/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_CLASS_MACROS_HPP
#define TURI_UNITY_TOOLKIT_CLASS_MACROS_HPP
#include <string>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_class_specification.hpp>
#include <unity/lib/toolkit_function_wrapper_impl.hpp>
#include <unity/lib/toolkit_class_wrapper_impl.hpp>
#include <unity/lib/extensions/model_base.hpp>

/** 
 * \defgroup group_gl_class_ffi Class Extension Interface
 * \ingroup group_gl_ffi
 *
 * The class Extension Interface provides a collection of macros that automate the 
 * process of exporting a class to Python. The macros are located in
 * sdk/toolkit_class_macros.hpp. 
 *
 * For detailed usage descriptions, see page_turicreate_extension_interface .
 *
 * Example:
 * \code
 *  // class must inherit from model_base
 *  #include <turicreate/sdk/toolkit_class_macros.hpp>
 *  using namespace turi;
 *
 *  class example: public model_base {
 *    std::string hello_world() {
 *      return "hello world";
 *    }
 *
 *    std::string concat(std::string a, std::string b) {
 *      return a + b;
 *    }
 *    // register class members.
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::hello_world);
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::concat, "a", "b");
 *    END_CLASS_MEMBER_REGISTRATION
 *  };
 *
 *  // register class
 *  BEGIN_CLASS_REGISTRATION
 *  REGISTER_CLASS(example)
 *  END_CLASS_REGISTRATION
 * \endcode
 *
 * \{
 */

/**
 * Begins a class member registration block.
 *
 * BEGIN_CLASS_MEMBER_REGISTRATION(python_facing_classname)
 *
 * The basic usage is to put this inside a class to be published, and go:
 * \code
 * BEGIN_CLASS_MEMBER_REGISTRATION("python_facing_classname")
 * REGISTER_ ... OTHER MEMBERS ...
 * END_CLASS_MEMBER_REGISTRATION
 * \endcode
 */
#define BEGIN_CLASS_MEMBER_REGISTRATION(python_facing_classname) \
 public: \
   virtual inline std::string name() { \
     return python_facing_classname; \
   } \
   virtual inline std::string uid() { \
     const char* file = __FILE__; \
     file = ((strrchr(file, '/') ? : file- 1) + 1); \
     return std::string(file) + ":" + \
         std::to_string(__LINE__) +  " " + \
         std::string(__DATE__) + " " + \
         std::string(__TIME__); \
   } \
   virtual inline void perform_registration() { \
     if (is_registered()) return;

/**
 * Registers a single class member function.
 *
 *  REGISTER_CLASS_MEMBER_FUNCTION(function, ...var args of input argument names ...)
 *
 *  Registers a function with no arguments.
 *  REGISTER_CLASS_MEMBER_FUNCTION(class::function) 
 *
 *  Registers a function with 2 input arguments. The first input argument is
 *  named "a" and the 2nd input argument is named "b"
 *  REGISTER_CLASS_MEMBER_FUNCTION(class::function, "a", "b") 
 *
 * Example:
 *
 * \code
 *  class example: public model_base {
 *    std::string hello_world() {
 *      return "hello world";
 *    }
 *
 *    std::string concat(std::string a, std::string b) {
 *      return a + b;
 *    }
 *
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::hello_world);
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::concat, "a", "b");
 *    END_CLASS_MEMBER_REGISTRATION
 *  }
 * \endcode
 *
 * The return value of the function will be returned to Python. The function
 * can return void. If the function fails, it should throw an exception which 
 * will be forward back to Python as RuntimeError.
 */
#define REGISTER_CLASS_MEMBER_FUNCTION(function, ...) \
  register_function(#function,  \
                    std::vector<std::string>{__VA_ARGS__}, \
                    toolkit_class_wrapper_impl::generate_member_function_wrapper_indirect( \
                    &function, ##__VA_ARGS__));

/**
 * Like REGISTER_CLASS_MEMBER_FUNCTION but allows the python-facing name of
 * the function to be redefined.
 *
 *  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(python_name, function, 
 *                          ...var args of input argument names ...)
 *
 *  Registers a function with 2 input arguments. The function shall be called
 *  "hello" in Python. The first input argument is named "a" and the 2nd input
 *  argument is named "b". 
 *
 *  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("hello", class::function, "a", "b") 
 */
#define REGISTER_NAMED_CLASS_MEMBER_FUNCTION(name, function, ...) \
  register_function(name,  \
                    std::vector<std::string>{__VA_ARGS__}, \
                    toolkit_class_wrapper_impl::generate_member_function_wrapper_indirect( \
                    &function, ##__VA_ARGS__));

// /*
//  * Like REGISTER_CLASS_MEMBER_FUNCTION but to be used when the function to
//  * be exposed is overloaded. 
//  *
//  *  REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION(function_type, function, 
//  *                          ...var args of input argument names ...)
//  *
//  * When a function is overloaded, the function pointer classname::function is
//  * ill-defined and one particular type of the function has to be selected.
//  *
//  * Example:
//  * \code
//  *  class example: public model_base {
//  *
//  *    std::string concat(std::string a, std::string b) {
//  *      return a + b;
//  *    }
//  *    std::string concat(std::string a, std::string b, std::string c) {
//  *      return a + b + c;
//  *    }
//  *
//  *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
//  *    // the type of the concat function we want
//  *    typedef std::string (example::* two_arg_concat_type)(std::string ,std::string);
//  *    REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION(two_arg_concat_type, example::concat, "a", "b");
//  *    END_CLASS_MEMBER_REGISTRATION
//  *  }
//  *  \endcode
//  */
/* #define REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION(overload_type, function, ...) \
 *   register_function(#function,  \
 *                     std::vector<std::string>{__VA_ARGS__}, \
 *                     toolkit_class_wrapper_impl::generate_member_function_wrapper_indirect( \
 *                     static_cast<overload_type>(&function), ##__VA_ARGS__));
 */
// /*
//  * Like REGISTER_CLASS_MEMBER_FUNCTION but to be used when the function to
//  * be exposed is overloaded, AND the python facing function is to be redefined.
//  *
//  *  REGISTER_NAMED_OVERLOADED_CLASS_MEMBER_FUNCTION(python_name,
//  *                          function_type, function, 
//  *                          ...var args of input argument names ...)
//  *
//  * When a function is overloaded, the function pointer classname::function is
//  * ill-defined and one particular type of the function has to be selected.
//  *
//  * Example:
//  * \code
//  *  class example: public model_base {
//  *
//  *    std::string concat(std::string a, std::string b) {
//  *      return a + b;
//  *    }
//  *    std::string concat(std::string a, std::string b, std::string c) {
//  *      return a + b + c;
//  *    }
//  *
//  *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
//  *    // the type of the concat function we want
//  *    typedef std::string (example::* two_arg_concat_type)(std::string ,std::string);
//  *    REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION("concat_two", two_arg_concat_type, 
//  *                                              example::concat, "a", "b");
//  *    typedef std::string (example::* three_arg_concat_type)(std::string ,std::string);
//  *    REGISTER_OVERLOADED_CLASS_MEMBER_FUNCTION("concat_three", three_arg_concat_type, 
//  *                                              example::concat, "a", "b");
//  *    END_CLASS_MEMBER_REGISTRATION
//  *  }
//  *  \endcode
//  */
/* #define REGISTER_OVERLOADED_NAMED_CLASS_MEMBER_FUNCTION(name, overload_type, function, ...) \
 *   register_function(name,  \
 *                     std::vector<std::string>{__VA_ARGS__}, \
 *                     toolkit_class_wrapper_impl::generate_member_function_wrapper_indirect( \
 *                     static_cast<overload_type>(&function), ##__VA_ARGS__));
 */

/// \internal
namespace docstring_macro_impl {
  /*
   * Registers a docstring for a function name. This in combination with 
   * the other overload for register_docstring allows for the macro
   * \code
   * register_docstring(name, #name, docstring)
   * \endcode
   * And that will work regardless of whether name is a string, or symbol.
   */
  inline std::pair<std::string, std::string> 
      get_docstring(const char** fnname,
                    std::string __unused__,
                    std::string docstring) {
    return {*fnname, docstring};
  }

  template <typename T>
  inline std::pair<std::string, std::string> 
      get_docstring(T t,
                    std::string fnname,
                    std::string docstring) {
    return {fnname, docstring};
  }
} // end namespacedocstring_macro_impl

/**
 * Registers a docstring of a function or property previously registered with
 * any of the registration functions. 
 *
 * Name can be a function, or a string. 
 * (Generally for the name, you put the first argument of any of 
 * the REGISTER macros and it should work fine)
 *
 * \code
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::hello_world);
 *    REGISTER_CLASS_MEMBER_DOCSTRING(example::hello_world, "prints hello world")
 *
 *    REGISTER_NAMED_CLASS_MEMBER_FUNCTION("hello", example::say_hello, "a", "b") 
 *    REGISTER_CLASS_MEMBER_DOCSTRING("hello", "says hello")
 *
 *    REGISTER_PROPERTY(abc)
 *    REGISTER_CLASS_MEMBER_DOCSTRING(abc, "contains the abc property")
 *    REGISTER_CLASS_DOCSTRING("example class")
 *    END_CLASS_MEMBER_REGISTRATION
 * \endcode
 */
#define REGISTER_CLASS_MEMBER_DOCSTRING(name, docstring) \
     this->register_docstring(docstring_macro_impl::get_docstring(&name, #name, docstring));

/**
 * Registers a docstring of a class 
 * \code
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_CLASS_MEMBER_FUNCTION(example::hello_world);
 *    REGISTER_CLASS_MEMBER_DOCSTRING(example::hello_world, "prints hello world")
 *
 *    REGISTER_NAMED_CLASS_MEMBER_FUNCTION("hello", example::say_hello, "a", "b") 
 *    REGISTER_CLASS_MEMBER_DOCSTRING("hello", "says hello")
 *
 *    REGISTER_PROPERTY(abc)
 *    REGISTER_CLASS_MEMBER_DOCSTRING(abc, "contains the abc property")
 *    REGISTER_CLASS_DOCSTRING("example class")
 *    END_CLASS_MEMBER_REGISTRATION
 * \endcode
 */
#define REGISTER_CLASS_DOCSTRING(docstring) \
     this->register_docstring({"__doc__", docstring});

/**
 * Registers a function as a getter for a python property.
 *
 * REGISTER_GETTER(python_property_name, getter_function)
 *
 * The getter_function must return a single value and take no arguments.
 *
 * \code
 *  class example: public model_base {
 *
 *    std::string get_value() {
 *      return value;
 *    }
 *    void set_value(std::string a) {
 *      a = value;
 *    }
 *    std::string value;
 *
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_GETTER("value", example::get_value)
 *    REGISTER_SETTER("value", example::set_value)
 *    END_CLASS_MEMBER_REGISTRATION
 * \endcode
 */
#define REGISTER_GETTER(propname, function) \
  register_getter(propname,  \
                  toolkit_class_wrapper_impl::generate_getter( \
                  &function));

/**
 * Registers a function as a setter for a python property.
 *
 * REGISTER_SETTER(python_property_name, setter_function)
 *
 * The setter_function must have a single argument, and return no values.
 *
 * \code
 *  class example: public model_base {
 *
 *    std::string get_value() {
 *      return value;
 *    }
 *    void set_value(std::string a) {
 *      a = value;
 *    }
 *    std::string value;
 *
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_GETTER("value", example::get_value)
 *    REGISTER_SETTER("value", example::set_value)
 *    END_CLASS_MEMBER_REGISTRATION
 * \endcode
 */
#define REGISTER_SETTER(propname, function) \
  register_setter(propname,  \
                  toolkit_class_wrapper_impl::generate_setter( \
                  &function, "value"));

/**
 * Registers a member variable, automatically generating a getter and a setter.
 *
 * REGISTER_PROPERTY(member_variable)
 *
 * \code
 *  class example: public model_base {
 *    std::string value;
 *
 *    BEGIN_CLASS_MEMBER_REGISTRATION("example")
 *    REGISTER_PROPERTY(value)
 *    END_CLASS_MEMBER_REGISTRATION
 * \endcode
 *
 */
#define REGISTER_PROPERTY(propname) \
  register_getter(#propname, [=](model_base* curthis, variant_map_type)->variant_type { \
                              return to_variant((dynamic_cast<std::decay<decltype(this)>::type>(curthis))->propname); \
                            }); \
  register_setter(#propname, [=](model_base* curthis, variant_map_type in)->variant_type { \
                              (dynamic_cast<std::decay<decltype(this)>::type>(curthis))->propname =      \
                                        variant_get_value<decltype(propname)>(in["value"]); \
                              return to_variant(0); \
                            });

/**
 * Begins a class registration block.
 * Basic usage:
 *
 * \code
 * BEGIN_CLASS_REGISTRATION
 * REGISTER_CLASS(example)
 * END_CLASS_REGISTRATION
 * \endcode
 *
 */
#define BEGIN_CLASS_REGISTRATION \
   __attribute__((visibility("default"))) std::vector<::turi::toolkit_class_specification> get_toolkit_class_registration() { \
     std::vector<::turi::toolkit_class_specification> specs;



/**
 * Ends a class member registration block.
 * See BEGIN_CLASS_MEMBER_REGISTRATION
 */
#define END_CLASS_MEMBER_REGISTRATION \
     set_registered(); }


/**
 * Begins a class registration block.
 *
 * Basic usage:
 * \code
 * BEGIN_CLASS_REGISTRATION
 * REGISTER_CLASS(example)
 * END_CLASS_REGISTRATION
 * \endcode
 *
 */
#define REGISTER_CLASS(class_name) \
    { \
       ::turi::toolkit_class_specification spec;  \
       class_name c; \
       spec.name = c.name(); \
       spec.constructor = +[]() { return (::turi::model_base*)(new class_name); }; \
       spec.description["functions"] =  \
          flexible_type_converter<std::map<std::string, \
                                  std::vector<std::string>>>().set(c.list_functions());  \
       spec.description["get_properties"] =  \
          flexible_type_converter<std::vector<std::string>>().set(c.list_get_properties());  \
       spec.description["set_properties"] =  \
          flexible_type_converter<std::vector<std::string>>().set(c.list_set_properties());  \
       spec.description["uid"] =  \
          flexible_type_converter<std::string>().set(c.uid());  \
       specs.push_back(spec); \
    }
/**
 * Ends a class registration block.
 *
 * Basic usage:
 * \code
 * BEGIN_CLASS_REGISTRATION
 * REGISTER_CLASS(example)
 * END_CLASS_REGISTRATION
 * \endcode
 *
 */
#define END_CLASS_REGISTRATION \
     return specs;     \
  }

/// \}
#endif // TURI_UNITY_TOOLKIT_MAGIC_MACROS_HPP
