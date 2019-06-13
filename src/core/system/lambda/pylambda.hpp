/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_PYLAMBDA_EVALUATOR_HPP
#define TURI_LAMBDA_PYLAMBDA_EVALUATOR_HPP
#include <core/system/lambda/lambda_interface.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/system/lambda/python_callbacks.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <string>

namespace turi {

namespace shmipc {
class server;
}

class sframe_rows;

namespace lambda {

/** The data used in the common call type.
 */
struct lambda_call_data {
  flex_type_enum output_enum_type = flex_type_enum::UNDEFINED;
  bool skip_undefined = false;

  // It's the responsibility of the calling class to make sure these
  // are valid.  input_values and output_values must point to storage
  // of at least n_inputs values.
  const flexible_type* input_values = nullptr;
  flexible_type* output_values = nullptr;
  size_t n_inputs = 0;
};

/** The data used in the call by dict call type.
 */
struct lambda_call_by_dict_data {
  flex_type_enum output_enum_type = flex_type_enum::UNDEFINED;
  bool skip_undefined = false;

  // It's the responsibility of the calling class to make sure these
  // are valid.  output_values must point to storage
  // of at least input_rows->size() values.
  const std::vector<std::string>* input_keys = nullptr;
  const std::vector<std::vector<flexible_type> >* input_rows = nullptr;
  flexible_type* output_values = nullptr;
};

/** The data used in the call by sframe rows call type.
 */
struct lambda_call_by_sframe_rows_data {
  flex_type_enum output_enum_type = flex_type_enum::UNDEFINED;
  bool skip_undefined = false;

  const std::vector<std::string>* input_keys = nullptr;
  const sframe_rows* input_rows = nullptr;
  flexible_type* output_values = nullptr;
};

/** The data used in applying a graph triple apply.
 */
struct lambda_graph_triple_apply_data {

  const std::vector<std::vector<flexible_type> >* all_edge_data;
  std::vector<std::vector<flexible_type> >* out_edge_data;

  std::vector<std::vector<flexible_type> >* source_partition;
  std::vector<std::vector<flexible_type> >* target_partition;


  const std::vector<std::string>* vertex_keys;
  const std::vector<std::string>* edge_keys;
  const std::vector<std::string>* mutated_edge_keys;
  size_t srcid_column, dstid_column;
};


struct pylambda_evaluation_functions {
  void (*set_random_seed)(size_t seed);
  size_t (*init_lambda)(const std::string&);
  void (*release_lambda)(size_t);
  void (*eval_lambda)(size_t, lambda_call_data*);
  void (*eval_lambda_by_dict)(size_t, lambda_call_by_dict_data*);
  void (*eval_lambda_by_sframe_rows)(size_t, lambda_call_by_sframe_rows_data*);
  void (*eval_graph_triple_apply)(size_t, lambda_graph_triple_apply_data*);
};

/** This is called through the cython functions to set up the
 *  evaluation function interface.
 */
void set_pylambda_evaluation_functions(pylambda_evaluation_functions* eval_function_struct);

extern pylambda_evaluation_functions evaluation_functions;

/**
 * Creates a lambda from a pickled lambda string.
 *
 * Throws an exception if the construction failed.
 */
size_t make_lambda(const std::string& pylambda_str);

/**
 * Release the cached lambda object
 */
void release_lambda(size_t lambda_hash);


/**
 * \ingroup lambda
 *
 * A functor class wrapping a pickled python lambda string.
 *
 * The lambda type is assumed to be either: S -> T or or List -> T.
 * where all types should be compatible  with flexible_type.
 *
 * \note: currently only support basic flexible_types: flex_string, flex_int, flex_float
 *
 * \internal
 * All public member functions including the constructors are guarded by the
 * global mutex, preventing simultanious access to the python's GIL.
 *
 * Internally, the class stores a a python lambda object which is created from the
 * pickled lambda string upon construction. The lambda object is equivalent
 * to a python lambda object (with proper reference counting), and therefore, the class is copiable.
 */
class pylambda_evaluator : public lambda_evaluator_interface {

 public:
  /**
   * Construct an empty evaluator.
   */
  inline pylambda_evaluator(turi::shmipc::server* shared_memory_server = nullptr) {
    m_shared_memory_server = shared_memory_server;
  };

  ~pylambda_evaluator();


  /**
   * Initializes shared memory communication via SHMIPC.
   * Returns the shared memory address to connect to.
   */
  std::string initialize_shared_memory_comm();

 private:

  // Set the lambda object for the next evaluation.
  void set_lambda(size_t lambda_hash);

  /**
   * Creates a lambda from a pickled lambda string.
   *
   * Throws an exception if the construction failed.
   */
  size_t make_lambda(const std::string& pylambda_str);

  /**
   * Release the cached lambda object
   */
  void release_lambda(size_t lambda_hash);


  /**
   * Apply as a function: flexible_type -> flexible_type,
   *
   * \note: this function does not perform type check and exception could be thrown
   * when applying of the function. As a subroutine, this function does not
   * try to acquire GIL and assumes it's already been acquired.
   */
  flexible_type eval(size_t lambda_hash, const flexible_type& arg);

  /**
   * Evaluate the lambda function on each argument separately in the args list.
   */
  std::vector<flexible_type> bulk_eval(size_t lambda_hash, const std::vector<flexible_type>& args,
                                       bool skip_undefined, int seed);

  /**
   * \overload
   *
   * We have to use different function name because
   * the cppipc interface doesn't support true overload
   */
  std::vector<flexible_type> bulk_eval_rows(size_t lambda_hash,
                                            const sframe_rows& values, bool skip_undefined, int seed);

  /**
   * Evaluate the lambda function on each element separately in the values.
   * The value element is combined with the keys to form a dictionary argument.
   */
  std::vector<flexible_type> bulk_eval_dict(size_t lambda_hash,
                                            const std::vector<std::string>& keys,
                                            const std::vector<std::vector<flexible_type>>& values,
                                            bool skip_undefined, int seed);

  /**
   * We have to use different function name because
   * the cppipc interface doesn't support true overload
   */
  std::vector<flexible_type> bulk_eval_dict_rows(size_t lambda_hash,
                                                 const std::vector<std::string>& keys,
                                                 const sframe_rows& values,
                                                 bool skip_undefined, int seed);


  /**
   * Redirects to either bulk_eval_rows or bulk_eval_dict_rows.
   * First byte in the string is a bulk_eval_serialized_tag byte to denote
   * whether this call is going to bulk_eval_rows or bulk_eval_dict_rows.
   *
   * Deserializes the remaining parameters from the string
   * and calls the function accordingly.
   */
  std::vector<flexible_type> bulk_eval_rows_serialized(const char* ptr, size_t len);

  turi::shmipc::server* m_shared_memory_server;
  turi::thread m_shared_memory_listener;
  volatile bool m_shared_memory_thread_terminating = false;
};
} // end of lambda namespace
} // end of turi namespace




#endif
