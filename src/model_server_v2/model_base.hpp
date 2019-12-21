/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_MODEL_BASE_V2_HPP
#define TURI_MODEL_BASE_V2_HPP

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <core/export.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server_v2/method_wrapper.hpp>
#include <model_server_v2/method_registry.hpp>

namespace turi { 
  namespace v2 {  


/**
 * The base class from which all new models must inherit.
 *
 * This class defines a generic object interface, listing properties and
 * callable methods, so that instances can be naturally wrapped and exposed to
 * other languages, such as Python.
 *
 * Subclasses that wish to support saving and loading should also override the
 * save_impl, load_version, and get_version functions below.
 */
class EXPORT model_base { 
 public:
 
  model_base();

  virtual ~model_base();

  // These public member functions define the communication between model_base
  // instances and the unity runtime. Subclasses define the behavior of their
  // instances using the protected interface below.

  /**
   * Returns the name of the toolkit class, as exposed to client code. For
   * example, the Python proxy for this instance will have a type with this
   * name.
   *
   */
  virtual const char* name() const = 0;
  
  /** Sets up the class given the options present.  
   *  
   *  TODO: implement all of this. 
   */
  virtual void setup(const variant_map_type& options) {
  //   option_manager.update(options); 
  } 

  /** Call one of the const methods registered using the configure() method above.
   *
   *  `args` may either be explicit arguments or an instance of 
   *  the argument_pack class.
   */ 
  template <typename... Args> 
  variant_type call_method(const std::string& name, const Args&... args) const;


  /** Call one of the methods registered using the configure() method above.  
   *
   *  `args` may either be explicit arguments or an instance of 
   *  the argument_pack class.
   */
  template <typename... Args> 
  variant_type call_method(const std::string& name, const Args&... args);
 

  /** Register a method that can be called by name using the registry above.
   *
   *  The format for calling this is the function name, the pointer to the method,
   *  then a list of names or Parameter class instances giving the names of the 
   *  parameters. 
   *
   *  Example: 
   *
   *    // For a method "add" in class C that derives from model_base.
   *    register_method("add", &C::add, "x", "y");
   *
   *    // For a method "inc" in class C that derives from model_base, 
   *    // with a default parameter of 1.
   *    register_method("inc", &C::inc, Parameter("delta", 1) );
   *
   *  
   *  See the documentation on method_wrapper<...>::create to see the format of args.
   */
  template <typename Method, typename... Args>
  void register_method(const std::string& name, Method&&, const Args&... args);  


   // TODO: add back in load and save routines.

 private: 
   std::shared_ptr<method_registry<model_base> > m_registry;

};

///////////////////////////////////////////////////////////////////////
//
//  Implementation of above template functions.

/** Call one of the methods registered using the configure() method above.  
 *
 */
template <typename... Args> 
variant_type model_base::call_method(
    const std::string& name, const Args&... args) {

  return m_registry->call_method(this, name, args...);
}

/** Const overload of the above.
 *
 */ 
template <typename... Args> 
variant_type model_base::call_method(
    const std::string& name, const Args&... args) const {

  return m_registry->call_method(this, name, args...);
}

/** Register a method that can be called by name using the registry above.
 */
template <typename Method, typename... Args>
void model_base::register_method(
    const std::string& name, Method&& m, const Args&... args) { 

  m_registry->register_method(name, m, args...);
} 

}
}

#endif
