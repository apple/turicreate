/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_MODEL_INTERFACE_HPP
#define TURI_UNITY_MODEL_INTERFACE_HPP
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <unity/lib/variant.hpp>
#include <cppipc/magic_macros.hpp>
#include <export.hpp>
namespace turi {

class model_proxy;

/**
 * \ingroup unity
 * The base class for all model objects. The model object is an object which 
 * conceptually contains a machine learning model, is implemented on the server,
 * and exposed to the client via the cppipc system.
 *
 * The model object is designed to be highly generic, and exposes a simple
 * interface which is abstractly that of a map from a string to a variant 
 * object (\ref variant_type). Implementations of the model must inherit from
 * model_base and implement all functions.
 *
 * \ref simple_model is the most basic implementation of this model object.
 *
 * \see simple_model
 */
class EXPORT model_base : public cppipc::ipc_object_base {
 public:
  typedef model_proxy proxy_object_type;

  virtual ~model_base() {}
  /**
   * Lists all the keys accessible in the map. All these keys are queryable
   * via get_value.
   */
  virtual std::vector<std::string> list_keys() = 0;

  /**
   * Returns the value of a particular key. To permit arbitrary queryability,
   * an argument can be passed along with the query request. (For instance,
   * the "key" may reference a vector of numbers, and the argument is used to
   * identify the vector offset.). The returned object must be a copy/clone 
   * and should not contain pointers/references back into this model object.
   * In other words, if the model object is destroyed, the returned object
   * should not be invalidated.
   */
  virtual variant_type get_value(std::string key, variant_map_type& arg) = 0;

  inline virtual void save(oarchive& oarc) const {
    oarc << get_version();
    save_impl(oarc);
  }


  inline virtual void load(iarchive& iarc) {
    size_t version = 0;
    iarc >> version;
    load_version(iarc, version);
  }
  /**
   * Returns the name of the model.
   */
  virtual std::string name() = 0; 

  /**
   * Returns the current model version
   */
  virtual size_t get_version() const = 0;

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  virtual void save_impl(oarchive& oarc) const = 0;

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  virtual void load_version(iarchive& iarc, size_t version) = 0;

  REGISTRATION_BEGIN(model_base);
  REGISTER(model_base::list_keys)
  REGISTER(model_base::get_value)
  REGISTER(model_base::name)
  REGISTRATION_END
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
#endif // TURI_UNITY_MODEL_INTERFACE_HPP
