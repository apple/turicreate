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
 * The base class from which all new toolkit classes must inherit.
 *
 * This class defines a generic object interface, listing properties and
 * callable methods, so that instances can be naturally wrapped and exposed to
 * other languages, such as Python.
 *
 * Subclasses should use the macros defined in toolkit_class_macros.hpp to
 * declared the desired properties and methods, and to define their
 * implementations. See that file for details and examples.
 *
 * Subclasses that wish to support saving and loading should also override the
 * save_impl, load_version, and get_version functions below.
 */
// TODO: Clean up the relationship between the save/load interface defined here
//       and re-declared in ml_model_base.
// TODO: Remove the inheritance from ipc_object_base once the cppipc code has
//       been disentangled and removed.
class EXPORT model_base: public cppipc::ipc_object_base {
 public:
  // TODO: Remove this type alias once this class stops inheriting from
  // ipc_object_base.
  using proxy_object_type = model_proxy;

  virtual ~model_base();

  // These public member functions define the communication between model_base
  // instances and the unity runtime. Subclasses define the behavior of their
  // instances using the protected interface below.

  /**
   * Returns the name of the toolkit class, as exposed to client code. For
   * example, the Python proxy for this instance will have a type with this
   * name.
   *
   * Note: this function is typically overridden using the
   * BEGIN_CLASS_MEMBER_REGISTRATION macro.
   */
  virtual const char* name() = 0;

  /**
   * Returns a unique identifier for the toolkit class. It can be *any* unique
   * ID. The UID is only used at runtime (to determine the concrete type of an
   * arbitrary model_base instance) and is never stored.
   *
   * Note: this function is typically overridden using the
   * BEGIN_CLASS_MEMBER_REGISTRATION macro.
   */
  virtual const std::string& uid() = 0;

  void save(oarchive& oarc) const {
    oarc << get_version();
    save_impl(oarc);
  }

  /**
   * Serializes the toolkit class.
   * Must save the class to the file format
   * version matching that of get_version().
   */
  virtual void save_impl(oarchive& oarc) const {
    // A subclass needs to override these methods if it has any data that needs to be serialized. 
    // Otherwise this is a valid serialization of an empty model. 
  };

  /**
   * Loads a toolkit class previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  virtual void load_version(iarchive& iarc, size_t version) {
    // A subclass needs to override these methods if it has any data that needs to be serialized. 
    // Otherwise this is a valid serialization of an empty model. 
  } ;



  void load(iarchive& iarc) {
    size_t version = 0;
    iarc >> version;
    load_version(iarc, version);
  }



  /**
   * Save a toolkit class to disk.
   *
   * \param sidedata Any additional side information
   * \param url The destination url to store the class.
   */
  void save_model_to_file(const variant_map_type& side_data,
                          const std::string& url);

  /**
   * Save a toolkit class to a data stream.
   */
  void save_model_to_data(std::ostream& out);

  /**
   * Returns the current version of the toolkit class for this instance, for
   * serialization purposes.
   */
  virtual size_t get_version() const { return 0; }

  /**
   * Lists all the registered functions.
   * Returns a map of function name to array of argument names for the function.
   */
  const std::map<std::string, std::vector<std::string> >& list_functions();

  /**
   * Lists all the get-table properties of the class.
   */
  const std::vector<std::string>& list_get_properties();

  /**
   * Lists all the set-table properties of the class.
   */
  const std::vector<std::string>& list_set_properties();

  /**
   * Calls a user defined function.
   */
  variant_type call_function(const std::string& function, variant_map_type argument);

  /**
   * Reads a property.
   */
  variant_type get_property(const std::string& property);
 
  /**
   * Sets a property. The new value of the property should appear in the
   * argument map under the key "value".
   */ 
  variant_type set_property(const std::string& property, variant_map_type argument);

  /**
   * Returns the toolkit documentation for a function or property.
   */
  const std::string& get_docstring(const std::string& symbol);

  // TODO: Remove this vestigial macro invocation once the dependency on cppipc
  // has been removed.
  REGISTRATION_BEGIN(model_base);
  REGISTRATION_END

 protected:
  using impl_fn = std::function<variant_type(model_base*, variant_map_type)>;

  // The macros defined in toolkit_class_macros.h use these functions to
  // conveniently define this instance's collection of client-level methods and
  // properties.

  /**
   * Function implemented by BEGIN_CLASS_MEMBER_REGISTRATION and invoked the
   * first time a public member function executes.
   */ 
  virtual void perform_registration() = 0;

  // Used to ensure that perform_registration is called once for each instance.
  bool is_registered() const { return m_registered; }
  void set_registered() { m_registered = true; }

  /**
   * Adds a function with the specified name, and argument list.
   */
  void register_function(std::string fnname,
                         const std::vector<std::string>& arguments, impl_fn fn);

  /**
   * Registers default argument values
   */
  void register_defaults(const std::string& fnname,
                         const variant_map_type& arguments);

  /**
   * Adds a property setter with the specified name.
   */
  void register_setter(const std::string& propname, impl_fn setfn);

  /**
   * Adds a property getter with the specified name.
   */
  void register_getter(const std::string& propname, impl_fn getfn);

  /**
   * Adds a docstring for the specified function or property name.
   */
  void register_docstring(const std::pair<std::string, std::string>& fnname_docstring);

 private:
  // whether perform registration has been called
  bool m_registered = false;
  // a description of all the function arguments. This is returned by 
  // list_functions().
  std::map<std::string, std::vector<std::string>> m_function_args;

  // default arguments, if any
  std::map<std::string, variant_map_type> m_function_default_args;
  // The implementation of each function
  std::map<std::string, impl_fn> m_function_list;
  // The implementation of each setter function
  std::map<std::string, impl_fn> m_set_property_list;
  mutable std::vector<std::string> m_set_property_cache;

  // The implementation of each getter function
  std::map<std::string, impl_fn > m_get_property_list;
  mutable std::vector<std::string> m_get_property_cache;

  // The docstring for each symbol
  std::map<std::string, std::string> m_docstring;

  // Internal helper functions
  inline void _check_registration();

  template <typename T>
  GL_COLD_NOINLINE_ERROR
  void _raise_not_found_error(const std::string& name,
                             const std::map<std::string, T>& m);

  std::string _make_method_name(const std::string& function);


};

// TODO: Remove this proxy subclass once the dependency on cppipc has been
// removed.
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
    std_log_and_throw(std::runtime_error,"Calling Unreachable Function");
  }

  const std::string& uid() {
    std_log_and_throw(std::runtime_error, "Calling Unreachable Function");
  }
  void perform_registration() {
    std_log_and_throw(std::runtime_error,"Calling Unreachable Function");
  }

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const {
    std_log_and_throw(std::runtime_error, "Calling Unreachable Function");
  }

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  void load_version(iarchive& iarc, size_t version) {
    std_log_and_throw(std::runtime_error, "Calling Unreachable Function");
  }

  BOOST_PP_SEQ_FOR_EACH(__GENERATE_PROXY_CALLS__, model_base, 
                        __ADD_PARENS__(
                            (const char*, name, )
                            ))
};
#endif
} // namespace turi

#endif // TURI_UNITY_MODEL_BASE_HPP
