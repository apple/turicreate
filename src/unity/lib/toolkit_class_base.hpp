/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_CLASS_BASE_HPP
#define TURI_UNITY_TOOLKIT_CLASS_BASE_HPP
#include <vector>
#include <map>
#include <string>
#include <utility>
#include <functional>
#include <unity/lib/api/model_interface.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <export.hpp>
namespace turi {
/**
 * The base class for which all new toolkit classes inherit from.
 *
 * This class implements the class member registration and dispatcher for 
 * class member functions and properties.
 *
 * Basically, the class exposes to Python 6 keys:
 * "list_functions" --> returns a dictionary of 
 *                      function_name --> array of arg names containing
 *                      all the functions and input keyword arguments of each
 *                      member function.
 *
 * "list_get_properties" --> returns an array of strings, where each string
 *                           is a property which can be read.
 *
 * "list_set_properties" --> returns an array of strings, where each string
 *                           is a property which can be written to.
 *
 * "call_function" --> the argument must contain the key "__function_name__"
 *                     which is the function to call. The remaining keys
 *                     must match the keyword arguments of the function as
 *                     set on registration by REGISTER_CLASS_MEMBER_FUNCTION
 *
 * "get_property" --> The argument must contain the key "__property_name__"
 *                    which is the property to retrieve.
 *
 * "set_property" --> The argument must contain the key "__property_name__"
 *                    which is the property to set. The key "value" must
 *                    contain the value to set to.
 *
 * "get_docstring" --> The argument must contain the key "__symbol__",
 *                     and this returns a docstring
 *
 * "__uid__" --> Returns a class specific string. This is used to bypass 
 *               type erasure to model_base.
 */
class EXPORT toolkit_class_base: public model_base {
 public:
  toolkit_class_base() { }
  virtual ~toolkit_class_base();
  /**
   * The internal keys.
   */
  std::vector<std::string> list_keys();

  /**
   * The main dispatcher.
   */
  variant_type get_value(std::string key, variant_map_type& arg);

  /**
   * Returns the name of the toolkit class.
   */
  virtual std::string name() = 0; 

  /**
   * Returns a unique identifier for the toolkit class. It can be *any* unique
   * ID. The UID is only used at runtime and is never stored.
   */
  virtual std::string uid() = 0; 

  /**
   * Serializes the toolkit class. Must save the class to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const;

  /**
   * Loads a toolkit class previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  virtual void load_version(iarchive& iarc, size_t version);


  /**
   * Returns the current toolkit class version
   */
  virtual size_t get_version() const;

  /**
   * Lists all the registered functions.
   * Returns a map of function name to array of arguments of the function.
   */
  virtual std::map<std::string, std::vector<std::string>> list_functions();

  /**
   * Lists all the get-table properties of the class.
   */
  virtual std::vector<std::string> list_get_properties();

  /**
   * Lists all the set-table properties of the class.
   */
  virtual std::vector<std::string> list_set_properties();

  /**
   * Calls a user defined function
   */
  virtual variant_type call_function(std::string function, 
                                     variant_map_type argument);

  /**
   * Reads a property
   */
  virtual variant_type get_property(std::string property,
                                    variant_map_type argument);
 
  /**
   * Sets a property.
   */ 
  virtual variant_type set_property(std::string property, 
                                    variant_map_type argument);

  virtual std::string get_docstring(std::string symbol);
 
  /**
   * Function implemented by BEGIN_CLASS_MEMBER_REGISTRATION
   */ 
  virtual void perform_registration() = 0;
 protected:
  // whether perform registration has been called
  bool registered = false;
  // a description of all the function arguments. This is returned by 
  // list_functions()
  std::map<std::string, std::vector<std::string>> m_function_args;

  // The implementation of each function
  std::map<std::string, 
      std::function<variant_type(toolkit_class_base*, 
                                 variant_map_type)> > m_function_list;
  // The implementation of each setter function
  std::map<std::string, 
      std::function<variant_type(toolkit_class_base*, 
                                 variant_map_type)> > m_set_property_list;
  // The implementation of each getter function
  std::map<std::string, 
      std::function<variant_type(toolkit_class_base*, 
                                 variant_map_type)> > m_get_property_list;
  // The docstring for each symbol
  std::map<std::string, std::string> m_docstring;

  /**
   * Adds a function with the specified name, and argument list.
   */
  void register_function(std::string fnname, 
                         std::vector<std::string> arguments,
                         std::function<variant_type(toolkit_class_base*, variant_map_type)> fn);

  /**
   * Adds a property setter with the specified name.
   */
  void register_setter(std::string propname, 
                       std::function<variant_type(toolkit_class_base*, variant_map_type)> setfn);

  /**
   * Adds a property getter with the specified name.
   */
  void register_getter(std::string propname, 
                       std::function<variant_type(toolkit_class_base*, variant_map_type)> getfn);

  void register_docstring(std::pair<std::string, std::string> fnname_docstring);
};

} // namespace turi
#endif // TURI_UNITY_TOOLKIT_CLASS_BASE_HPP

