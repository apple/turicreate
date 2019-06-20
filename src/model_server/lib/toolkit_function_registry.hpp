/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TOOLKIT_FUNCTION_REGISTRY_HPP
#define TURI_TOOLKIT_FUNCTION_REGISTRY_HPP
#include <map>
#include <string>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/api/function_closure_info.hpp>
namespace turi {

/**
 * \ingroup unity
 * Defines a collection of toolkits. Has the ability to add/register new
 * toolkits, and get information about the toolkits.
 */
class toolkit_function_registry {
 private:
  std::map<std::string, toolkit_function_specification> registry;

 public:

  // default constructor. Does nothing
  toolkit_function_registry() { }

  /**
   * Registers a toolkit specification. See \ref toolkit_function_specification
   * for details. After registration, information about the toolkit will be
   * queryable via the other toolkit_function_registry function. \ref unregister_toolkit_function
   * will remove the toolkit from the registry.
   *
   * \returns Returns true on success. Returns false if some other toolkit with
   * the same name has already been registered.
   */
  bool register_toolkit_function(toolkit_function_specification spec,
                                 std::string prefix = "");

  /**
   * Registers a collection of toolkit specifications. See \ref
   * toolkit_function_specification for details. After registration, information about
   * the toolkits will be queryable via the other toolkit_function_registry function.
   * \ref unregister_toolkit_function will remove the toolkit from the registry.
   *
   * \returns Returns true on success. Returns false if some other toolkit with
   * the same name has already been registered, in which case, none of the
   * toolkits listed in spec will be registered.
   */
  bool register_toolkit_function(std::vector<toolkit_function_specification> spec,
                                 std::string prefix = "");


  /**
   * Unregisters a previously registered toolkit.
   *
   * \returns Returns true on success. Returns false if a toolkit with
   * the specified name has not been registered.
   */
  bool unregister_toolkit_function(std::string name);

  /**
   * Gets the complete specification infomation about a toolkit.
   *
   * \returns Returns a pointer to the toolkit_function_specification on success.
   * Returns NULL if a toolkit with the specified name has not been registered.
   *
   * \note Registering, or unregistering toolkits invalidates this pointer.
   */
  const toolkit_function_specification* get_toolkit_function_info(std::string toolkit_fn_name);

  /**
   * Returns the natively callable version of a toolkit function if available.
   * Raises an exception otherwise.
   */
  std::function<variant_type(const std::vector<variant_type>&)>
      get_native_function(std::string name);


  /**
   * Returns the natively callable version of a toolkit function with
   * closure information associated.
   * Raises an exception otherwise.
   */
  std::function<variant_type(const std::vector<variant_type>&)>
      get_native_function(const function_closure_info& name);

  /**
   * Returns a list of names of all registered toolkits.
   *
   * \returns A vector of string where each string is a toolkit name.
   */
  std::vector<std::string> available_toolkit_functions();
};

}

#endif
