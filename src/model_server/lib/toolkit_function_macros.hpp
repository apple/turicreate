/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_FUNCTION_MACROS_HPP
#define TURI_UNITY_TOOLKIT_FUNCTION_MACROS_HPP
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_function_wrapper_impl.hpp>
#include <model_server/lib/toolkit_class_wrapper_impl.hpp>


/**************************************************************************/
/*                                                                        */
/*                         Function Registration                          */
/*                                                                        */
/**************************************************************************/
/**
 * \defgroup group_gl_function_ffi Function Extension Interface
 * \ingroup group_gl_ffi
 *
 * The Function Extension Interface provides a collection of macros that automate
 * the process of exporting a function to Python. The macros are located in
 * sdk/toolkit_function_macros.hpp .
 *
 * For detailed usage descriptions, see page_turicreate_extension_interface .
 *
 * Example:
 * \code
 *  #include <string>
 *  #include <model_server/lib/toolkit_function_macros.hpp>
 *  using namespace turi;
 *
 *  std::string demo_to_string(int in) {
 *    return std::to_string(in);
 *  }
 *
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(demo_to_string, "in");
 *  END_FUNCTION_REGISTRATION
 * \endcode
 * \{
 */

/**
 * Begins a toolkit registration block.
 *
 * \see END_FUNCTION_REGISTRATION
 * \see REGISTER_FUNCTION
 *
 * The basic usage is:
 *  \code
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(my_function, inargname1, inargname2 ...)
 *  REGISTER_FUNCTION(my_function2, inargname1, ...)
 *  END_FUNCTION_REGISTRATION
 *  \endcode
 *
 */
#define BEGIN_FUNCTION_REGISTRATION                         \
  __attribute__((visibility("default")))                  \
      std::vector<::turi::toolkit_function_specification> \
      get_toolkit_function_registration() {               \
     std::vector<::turi::toolkit_function_specification> specs;

/**
 * Registers a function to make it callable from Python.
 * \see BEGIN_FUNCTION_REGISTRATION
 * \see END_FUNCTION_REGISTRATION
 *
 *  Registers a function with no arguments.
 *  \code
 *  REGISTER_FUNCTION(function)
 *  \endcode
 *
 *  Registers a function with 3 input arguments. The first input argument shall
 *  be named "a", the second named "b" and the 3rd named "c"
 *  \code
 *  REGISTER_FUNCTION(function, "a", "b", "c")
 *  \endcode
 *
 * Example:
 *
 * \code
 *  std::string demo_to_string(flexible_type in) {
 *    return std::string(in);
 *  }
 *
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(demo_to_string, "in");
 *  END_FUNCTION_REGISTRATION
 * \endcode
 *
 * Namespaces are permitted. For instance:
 * \code
 *  namespace example {
 *  std::string demo_to_string(flexible_type in) {
 *    return std::string(in);
 *  }
 *  }
 *
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(example::demo_to_string, "in");
 *  END_FUNCTION_REGISTRATION
 * \endcode
 *
 * Both will be published as "demo_to_string"; the namespacing is ignored.
 *
 * The return value of the function will be returned to Python. The function
 * can return void. If the function fails, it should throw an exception which
 * will be forward back to Python as RuntimeError.
 */
#define REGISTER_FUNCTION(function, ...) \
     specs.push_back(toolkit_function_wrapper_impl::make_spec_indirect(function, #function, \
                                                                      ##__VA_ARGS__));

/**
 * Register a function, assigning it a different name than the name of the function.
 *
 * \code
 *  std::string demo_to_string(flexible_type in) {
 *    return std::string(in);
 *  }
 *
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_NAMED_FUNCTION("module._demo_to_string", demo_to_string, "in");
 *  END_FUNCTION_REGISTRATION
 * \endcode
 */
#define REGISTER_NAMED_FUNCTION(name, function, ...) \
     specs.push_back(toolkit_function_wrapper_impl::make_spec_indirect(function, name, \
                                                                      ##__VA_ARGS__));
/**
 * Sets a docstring on the function.
 * If not provided, a default docstring describing the input arguments will be
 * used. Must be called only *after* the function is registered. i.e.
 * the matching REGISTER_FUNCTION must appear
 * before, and it must be called with exactly the same function.
 *
 * \code
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(demo_add_one, "in");
 *  REGISTER_DOCSTRING(demo_add_one, "Adds one to an integer/float")
 *  REGISTER_DOCSTRING(demo_to_string, "Converts an arbitrary value to a string")
 *  END_FUNCTION_REGISTRATION
 * \endcode
 */
#define REGISTER_DOCSTRING(function, docstring) \
  for (auto& i: specs) { \
    if (i.description.count("_raw_fn_pointer_") &&  \
        i.description.at("_raw_fn_pointer_") == reinterpret_cast<size_t>(function)) { \
      i.description["documentation"] = docstring; \
    } \
  }



/**
 * Ends a toolkit registration block.
 *
 * \see BEGIN_FUNCTION_REGISTRATION.
 * \see REGISTER_FUNCTION_FUNCTION
 *
 * The basic usage is:
 *  \code
 *  BEGIN_FUNCTION_REGISTRATION
 *  REGISTER_FUNCTION(my_function, inargname1, inargname2 ...)
 *  REGISTER_FUNCTION(my_function2, inargname1, ...)
 *  END_FUNCTION_REGISTRATION
 *  \endcode
 */
#define END_FUNCTION_REGISTRATION  \
     return specs; \
   }

/// \}

#endif // TURI_UNITY_TOOLKIT_MAGIC_MACROS_HPP
