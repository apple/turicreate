/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_GROUP_AGGREGATE_VALUE_HPP
#define TURI_SFRAME_GROUP_AGGREGATE_VALUE_HPP

#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup groupby_aggregate Groupby Aggregation
 * \{
 */

/**
 * The base class specification for representing the intermediate state as well
 * as the computation (aggregation, combining and output) for an aggregation
 * operation.
 *
 * Essentially, the group_aggregate_value must describe a parallel reduction in
 * the following form:
 *
 * \code
 * for streamid in data_stream:
 *   for data in stream[streamid]:
 *     value[streamid].add_element(data)
 *   value[streamid].partial_finalize()
 *
 * for streamid in data_stream:
 *   final_value.combine(value[streamid])
 *
 * output = final_value.emit()
 * \endcode
 *
 * Each value could have an arbitrary number of elements inserted into it.
 * When all insertions are complete, partial_finalize() is called on the value.
 * Values can be combined with each other to form a final value, which then
 * emits a response.
 */
class group_aggregate_value {
 public:
  /**
   * Creates a new instance of the aggregator. The new instance must
   * remember the input type (see set_input_type()) but have have a new
   * empty value.
   */
  virtual group_aggregate_value* new_instance() const = 0;

  /**
   * Adds an element to the aggregate. Elements to be added will be either
   * the input_type (as set by set_input_type()) or UNDEFINED.
   *
   * Operator that expects more than one input values need to overwrite this function
   */
  virtual void add_element(const std::vector<flexible_type>& values) {
    DASSERT_TRUE(values.size() == 1);
    add_element_simple(values[0]);
  }

  /**
   * Adds an element to the aggregate. Simple version of add_element
   * where there is only one input value for the operator
   */
  virtual void add_element_simple(const flexible_type& flex) = 0;

  /**
   * No more elements will be added to this value. However, this value
   * may still be combined with other values.
   */
  virtual void partial_finalize() { }

  /**
   * Combines two partial aggregates
   */
  virtual void combine(const group_aggregate_value& other) = 0;

  /**
   * Emits the result of a groupby operation
   */
  virtual flexible_type emit() const = 0;

  /**
   * Returns true if the the aggregate_value can consume a column of this type,
   * and false otherwise. (For instance, a sum aggregator can consume integers
   * and floats, and not anything else).
   */
  virtual bool support_type(flex_type_enum type) const = 0;

  /**
   * Sets the input types and returns the output type. For instance,
   * a sum aggregator when summing integers will return an integer, and when
   * summing doubles will return doubles.
   *
   * Default implementation assumes there is ony one input, and output
   * type is the same as input type.
   */
  virtual flex_type_enum set_input_types(const std::vector<flex_type_enum>& types) {
    DASSERT_TRUE(types.size() == 1);
    return set_input_type(types[0]);
  }

  /**
   * Returns a printable name of the operation.
   */
  virtual std::string name() const = 0;

  /**
   * Saves the state of the aggregation to an archive
   */
  virtual void save(oarchive& oarc) const = 0;

  /**
   * Loads the state of the aggregation from an archive
   */
  virtual void load(iarchive& iarc) = 0;

  /**
   * Destruction
   */
  virtual inline ~group_aggregate_value() { }

  /**
   * Override this function for allowing the operator to be easily printed.
   *
   * \code
   *   std::cout << aggregator <<s std::endl;
   * \endcode
   */
  virtual void print(std::ostream& os) const {
    os << this->name() << "(value = " << this->emit() << ")";
  }

  virtual flex_type_enum set_input_type(flex_type_enum type) {
    return type;
  }
};

inline std::ostream& operator<<(std::ostream& os, const group_aggregate_value& dt) {
  dt.print(os);
  return os;
}


/**
 * Helper function to convert string aggregator name into builtin aggregator value.
 *
 * Implementation is in groupby_operators.hpp
 */
std::shared_ptr<group_aggregate_value> get_builtin_group_aggregator(const std::string&);


/// \}
} // end of turi
#endif // TURI_SFRAME_GROUP_AGGREGATE_VALUE_HPP
