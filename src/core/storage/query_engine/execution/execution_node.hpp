/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_OPERATORS_EXECUTION_NODE_HPP
#define TURI_SFRAME_QUERY_ENGINE_OPERATORS_EXECUTION_NODE_HPP

#include <memory>
#include <vector>
#include <queue>


#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/util/broadcast_queue.hpp>

namespace turi {
class sframe_rows;

template <>
struct broadcast_queue_serializer<std::shared_ptr<sframe_rows>> {
  void save(oarchive& oarc, const std::shared_ptr<sframe_rows>& t) {
    oarc << (*t);
  }
  void load(iarchive& iarc, std::shared_ptr<sframe_rows>& t) {
    t = std::make_shared<sframe_rows>();
    iarc >> (*t);
  }
};

namespace query_eval {


class query_context;

/**
 * \ingroup sframe_query_engine
 * \addtogroup execution Execution
 * \{
 */

/**
 * The execution node provides a wrapper around an operator. It
 *  - manages the coroutine context for the operator
 *  - manages the connections between the operator and its inputs and outputs.
 *  - Manages the buffering and transfer of information between the operator,
 *
 * \subsection execution_node_coroutines Coroutines
 *
 * Essentially, calling a coroutine, causes a context switch to occur
 * starting the coroutine. Then within the coroutine, a "sink()" function can be
 * called which context switches and resumes execution *where the coroutine was
 * initially triggered*.
 *
 * The classical example is a producer-consumer queue
 * \code
 * void producer() {
 *   while(1) {
 *     a = new work
 *     consumer(a); // or sink(a) in the above syntax
 *   }
 * }
 *
 * void consumer() {
 *   while(1) {
 *     a = producer();
 *     // do work on a
 *   }
 * }
 *
 * \endcode
 *
 * Here, we are using coroutines to attach and communicate between query
 * operators, so for instance, here is a simple transform on a source.
 * \code
 * void data_source() {
 *   while(data_source_has_rows) {
 *     rows = read_rows
 *     sink(rows);
 *   }
 * }
 *
 * void transform() {
 *   while(1) {
 *     data = source()
 *     if (data == nullptr) {
 *       break;
 *     } else {
 *       transformed_data = apply_transform(data)
 *       sink(transformed_data);
 *     }
 *   }
 * }
 * \endcode
 *
 * while the context switch is relatively cheap (boost coroutines promise this
 * at < 100 cycles or so), we still want to avoid performing the context
 * switch for every row, so our unit of communication across coroutines
 * is an \ref sframe_rows object which represents a collection of rows, but
 * represented columnar-wise. Every communicated block must be of a constant
 * number of rows (i.e. SFRAME_READ_BATCH_SIZE. ex: 256), except for the last
 * block which may be smaller. Operators which perform filtering for instance,
 * must hence make sure to buffer accordingly.
 *
 * \subsection execution_node_rate_control Rate Control
 * One key issue with any of the pipeline models (whether pull-based: like here,
 * or push-based) is about rate-control. For instance, the operator graph
 * corresponding to the following expression has issues:
 * \code
 * logical_filter(source_A, selector_source) + logical_filter(source_B, selector_source)
 * \endcode
 *  - To compute the "+", the left logical filter operator is invoked
 *  - To compute the left logical filter, source_A and selector_source is read
 *    and they continue to be read until say... 256 rows are generated.
 *    This is then sent to the "+" operator which resumes execution.
 *  - The "+" operator then reads the right logical filter operator.
 *  - The right logical filter operator now needs to read source_B and
 *  selector_source.
 *  - However, selector_source has already advanced because it was partially
 *  consumed for the left logical filter.
 *
 * A solution to this requires the selector_source to buffer its reads
 * while feeding the left logical_filter. The solution to this is to either
 * assume that all connected operators operate at exactly the same rate
 * (we can guarantee this with some care as to how the operator graph is
 * constructed), or we allow buffering. This buffering has to be somewhat
 * intelligent because it may require unbounded buffers.
 *
 * An earlier version of this execution model used the former procedure
 * (assuming uniform rate), now we use the \ref broadcast_queue to provide
 * unbounfed buffering.
 *
 * \subsection execution_node_usage execution_node Usage
 *
 * The execution_node is not generally used directly (see the hierarchy of
 * materialize functions). However, usage is not very complicated.
 *
 * Given an execution_node graph, with a tip you will like to consume data from:
 * \code
 *  (tip is a shared_ptr<execution_node>)
 *  // register a new consumer (aka myself)
 *  size_t consumer_id = tip->register_consumer();
 *
 *  while(1) {
 *    auto rows = node->get_next(consumer_id);
 *    // do stuff. rows == nullptr on completion
 *  }
 * \endcode
 */
class execution_node  : public std::enable_shared_from_this<execution_node> {
 public:
  execution_node(){}

  /**
   * Initializes the execution node with an operator and inputs.
   * Also resets the operator.
   */
  explicit execution_node(const std::shared_ptr<query_operator>& op,
                 const std::vector<std::shared_ptr<execution_node> >& inputs
                   = std::vector<std::shared_ptr<execution_node>>());

  execution_node(execution_node&&) = default;
  execution_node& operator=( execution_node&&) = default;

  execution_node(const execution_node&) = delete;
  execution_node& operator=(const execution_node&) = delete;

  /**
   * Initializes the execution node with an operator and inputs.
   * Also resets the operator.
   */
  void init(const std::shared_ptr<query_operator>& op,
            const std::vector<std::shared_ptr<execution_node> >& inputs
            = std::vector<std::shared_ptr<execution_node>>());


  /**
   * Adds an execution consumer. This function call then
   * returns an ID which the caller should use with get_next().
   */
  size_t register_consumer();


  /** Returns nullptr if there is no more data.
   */
  std::shared_ptr<sframe_rows> get_next(size_t consumer_id, bool skip=false);

  /**
   * Returns the number of inputs of the execution node
   */
  inline size_t num_inputs() const {
    return m_inputs.size();
  }

  inline std::shared_ptr<execution_node> get_input_node(size_t i) const {
    return m_inputs[i].m_node;
  }

  /**
   * resets the state of this execution node. Note that this does NOT
   * recursively reset all parents (since in a general graph this could cause
   * multiple resets of the same vertex). The caller must ensure that all
   * connected execution nodes are reset.
   */
  void reset();

  /**
   * Returns true if an exception occured while executing this node
   */
  bool exception_occurred() const {
    return m_exception_occured;
  }

  /**
   * If an exception occured while excecuting this node, this returns the
   * last exception exception. Otherwise returns an exception_ptr which
   * compares equal to the null pointer.
   */
  std::exception_ptr get_exception() const {
    return m_exception;
  }
 private:
  /**
   * Internal function used to add to the operator output
   */
  void add_operator_output(const std::shared_ptr<sframe_rows>& rows);

  /**
   * Internal utility function what pulls the next batch of rows from a input
   * to this node.
   * This reads *exactly* one block.
   * If skip is true, nullptr is always returned.
   */
  std::shared_ptr<sframe_rows> get_next_from_input(size_t input_id, bool skip);

  /**
   * Starts the coroutines
   */
  void start_coroutines();

  /// The operator implementation
  std::shared_ptr<query_operator> m_operator;

  std::shared_ptr<query_context> m_context;

  /**
   * The inputs to this execution node:
   *   what execution node they come from, and what is the consumer ID
   *   when trying to pull data from the execution node.
   */
  struct input_node {
    std::shared_ptr<execution_node> m_node;
    size_t m_consumer_id = 0;
  };
  std::vector<input_node> m_inputs;

  std::unique_ptr<broadcast_queue<std::shared_ptr<sframe_rows> > > m_output_queue;
  size_t m_head = 0;
  bool m_coroutines_started = false;
  bool m_skip_next_block = false;

  /// m_consumer_pos[i] is the ID which consumer i is consuming next.
  std::vector<size_t> m_consumer_pos;

  /// exception handling
  bool m_exception_occured = false;
  std::exception_ptr m_exception;
  bool supports_skipping;
  bool is_linear_operator;

  friend class query_context;
};

/// \}
}}

#endif /* TURI_SFRAME_QUERY_ENGINE_OPERATORS_EXECUTION_NODE_HPP */
