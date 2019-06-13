/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_CLIENT_GLOBAL_HPP
#define TURI_UNITY_CLIENT_GLOBAL_HPP
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <unity/lib/api/unity_sarray_interface.hpp>
#include <unity/lib/api/unity_graph_interface.hpp>
#include <unity/lib/toolkit_function_response.hpp>
#include <unity/lib/api/client_base_types.hpp>
#include <cppipc/magic_macros.hpp>

namespace turi {


#if DOXYGEN_DOCUMENTATION
// Doxygen fake documentation

/**
 * \ingroup unity
 * The \ref turi::unity_global and \ref turi::unity_global_base classes
 * implement a singleton object on the server side which is exposed to the
 * client via the cppipc system. This singleton object provides other
 * miscellaneous uncategorized services (global functions) to the python layer.
 * See \ref turi::unity_global for detailed documentation of the functions.
 */
class unity_global_base {
 public:
  std::vector<std::string> list_toolkit_functions();
  std::string get_version();
  std::string get_graph_dag();

  toolkit_function_response_type run_toolkit(std::string name, variant_map_type& opts);

  std::shared_ptr<unity_sgraph_base> load_graph(std::string file);
};
#endif

typedef std::map<std::string, flexible_type> global_configuration_type;

  GENERATE_INTERFACE_AND_PROXY(unity_global_base, unity_global_proxy,
      (std::vector<std::string>, list_toolkit_functions, )
      (std::vector<std::string>, list_toolkit_classes, )
      (global_configuration_type, describe_toolkit_function, (std::string))
      (global_configuration_type, describe_toolkit_class, (std::string))
      (std::shared_ptr<model_base>, create_toolkit_class, (std::string))
      (std::string, get_version, )
      (std::string, get_graph_dag, )
      (toolkit_function_response_type, run_toolkit, (std::string)(variant_map_type&))
      (std::shared_ptr<unity_sgraph_base>, load_graph, (std::string))
      (variant_map_type, load_model, (const std::string&))
      (void, save_model, (std::shared_ptr<model_base>)(const variant_map_type&)(const std::string&))
      (void, save_model2, (const std::string&)(const variant_map_type&)(const std::string&))
      (flexible_type, eval_lambda, (const std::string&)(const flexible_type&))
      (flexible_type, eval_dict_lambda, (const std::string&)(const std::vector<std::string>&)(const std::vector<flexible_type>&))
      (std::vector<flexible_type>, parallel_eval_lambda, (const std::string&)(const std::vector<flexible_type>&))
      (std::string, __read__, (const std::string&))
      (void, __write__, (const std::string&)(const std::string&))
      (bool, __mkdir__, (const std::string&))
      (bool, __chmod__, (const std::string&)(short))
      (size_t, __get_heap_size__, )
      (size_t, __get_allocated_size__, )
      (void, set_log_level, (size_t))
      (global_configuration_type, list_globals, (bool))
      (std::string, set_global, (std::string)(flexible_type))
      (std::shared_ptr<unity_sarray_base>, create_sequential_sarray, (ssize_t)(ssize_t)(bool))
      (std::string, load_toolkit, (std::string)(std::string))
      (std::vector<std::string>, list_toolkit_functions_in_dynamic_module, (std::string))
      (std::vector<std::string>, list_toolkit_classes_in_dynamic_module, (std::string))
      (std::string, get_current_cache_file_location, )
      (std::string, get_turicreate_object_type, (const std::string&))
  )
} // namespace turi

#endif // TURI_UNITY_CLIENT_GLOBALS_HPP
