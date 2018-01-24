/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_MODEL_BASE_HPP
#define TURI_UNITY_MODEL_BASE_HPP

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <export.hpp>

#include <cppipc/ipc_object_base.hpp>
#include <cppipc/magic_macros.hpp>
#include <serialization/serialization_includes.hpp>
#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/variant.hpp>

namespace turi {

class model_proxy;

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

class EXPORT model_base: public cppipc::ipc_object_base {
 public:
  using proxy_object_type = model_proxy;

  virtual ~model_base();
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

  inline virtual void save(oarchive& oarc) const {
    oarc << get_version();
    save_impl(oarc);
  }

  /**
   * Serializes the toolkit class. Must save the class to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const;

  inline virtual void load(iarchive& iarc) {
    size_t version = 0;
    iarc >> version;
    load_version(iarc, version);
  }

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
  virtual std::map<std::string, std::vector<std::string> > list_functions();

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

  REGISTRATION_BEGIN(model_base);
  REGISTER(model_base::list_keys)
  REGISTER(model_base::get_value)
  REGISTER(model_base::name)
  REGISTRATION_END

 protected:
  using impl_fn = std::function<variant_type(model_base*, variant_map_type)>;

  // whether perform registration has been called
  bool registered = false;
  // a description of all the function arguments. This is returned by 
  // list_functions()
  std::map<std::string, std::vector<std::string>> m_function_args;

  // default arguments if any
  std::map<std::string, variant_map_type> m_function_default_args;

  // The implementation of each function
  std::map<std::string, impl_fn> m_function_list;
  // The implementation of each setter function
  std::map<std::string, impl_fn> m_set_property_list;
  // The implementation of each getter function
  std::map<std::string, impl_fn > m_get_property_list;
  // The docstring for each symbol
  std::map<std::string, std::string> m_docstring;

  /**
   * Adds a function with the specified name, and argument list.
   */
  void register_function(
      std::string fnname, std::vector<std::string> arguments, impl_fn fn);

  /**
   * Registers default argument values
   */
  void register_defaults(std::string fnname, 
                         const variant_map_type& arguments);

  /**
   * Adds a property setter with the specified name.
   */
  void register_setter(std::string propname, impl_fn setfn);

  /**
   * Adds a property getter with the specified name.
   */
  void register_getter(std::string propname, impl_fn getfn);

  void register_docstring(std::pair<std::string, std::string> fnname_docstring);
};

#ifndef DISABLE_TURI_CPPIPC_PROXY_GENERATION
/**
 * Explicitly implemented proxy object.
 *
 */
class model_proxy : public model_base {
 public:
  cppipc::object_proxy<model_base> proxy;

  inline model_proxy(cppipc::comm_client& comm, 
                    bool auto_create = true,
                    size_t object_id = (size_t)(-1)):
      proxy(comm, auto_create, object_id){ }

  inline void save(turi::oarchive& oarc) const {
    oarc << proxy.get_object_id();
  }

  inline size_t __get_object_id() const {
    return proxy.get_object_id();
  }

  inline void load(turi::iarchive& iarc) {
    size_t objid; iarc >> objid;
    proxy.set_object_id(objid);
  }

  virtual size_t get_version() const {
    throw("Calling Unreachable Function");
  }

  std::string uid() override {
    throw("Calling Unreachable Function");
  }
  void perform_registration() override {
    throw("Calling Unreachable Function");
  }

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const {
    throw("Calling Unreachable Function");
  }

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  void load_version(iarchive& iarc, size_t version) {
    throw("Calling Unreachable Function");
  }

  BOOST_PP_SEQ_FOR_EACH(__GENERATE_PROXY_CALLS__, model_base, 
                        __ADD_PARENS__(
                            (std::vector<std::string>, list_keys, )
                            (variant_type, get_value, (std::string)(variant_map_type&))
                            (std::string, name, )
                            ))
};
#endif

} // namespace turi
#endif // TURI_UNITY_MODEL_BASE_HPP

