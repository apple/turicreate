/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_MODEL_SERVER_HPP
#define TURI_MODEL_SERVER_HPP

#include <core/export.hpp>
#include <core/util/code_optimization.hpp>
#include <memory>
#include <string>
#include <vector>
#include <model_server/lib/variant.hpp>
#include <model_server_v2/method_registry.hpp>


namespace turi {
namespace v2 {

class model_server_impl;

/** Returns the singleton version of the model server. 
 *
 */
EXPORT model_server_impl& model_server();


EXPORT class model_server_impl { 
  private:

    // Disable instantiation outside of the global instance.
    model_server_impl();
    friend model_server_impl& model_server();

    // Explicitly disable copying, etc.
    model_server_impl(const model_server_impl&) = delete;
    model_server_impl(model_server_impl&&) = delete;

 public: 
   
    ////////////////////////////////////////////////////////////////////////////
    // Calling models. 

    /** Instantiate a previously registered model by name.
     *
     */
    std::shared_ptr<model_base> create_model(const std::string& model_name);
   

    /** Instantiate a model by type.
     */
    template <typename ModelType> 
      std::shared_ptr<ModelType> create_model(); 


    /** Call a previously registered function.
     */
    template <typename... FunctionArgs>
    variant_type call_function(
        const std::string& function_name, FunctionArgs&&... args); 

 public:

   /** Registration of a function.
    *
    *  Registers a new function that can be called through the call_function
    *  call above.   
    *   
    *  The format of the call is name of the function, the function, and then
    *  a list of 0 or more parameter specs.  
    *
    *  Example: 
    *
    *    void f(int x, int y); 
    *    register_new_function("f", f, "x", "y"); 
    *
    *
    *
    *  \param name     The name of the function.
    *  \param function A pointer to the function itself.
    *  \param param_specs The parameter specs
    */
   template <typename Function, typename... ParamSpecs>
     void register_new_function(const std::string& name, Function&& function, ParamSpecs&&...);


   /** Registration of new models.
    *
    *  A model is registered through a call to register new model, which 
    *  instantiates it and populates the required options and method call 
    *  lookups.  Copies of these options and method call lookups are stored 
    *  internally in a registry here so new models can be instantiated quickly.
    *
    *   
    *  The new model's name() method provides the name of the model being 
    *  registered.
    *  
    *  This method can be called at any point.
    *
    */
   template <typename ModelClass> void register_new_model();

   /** Fast on-load model registration. 
    *
    *  The callbacks below provide a fast method for registering new models 
    *  on library load time.  This works by first registering a callback  
    *  using a simple callback function.  
    *
    */
  typedef void (*_registration_callback)(model_server_impl&);
   
  /** Register a callback function to be processed when a model is served.
   *
   *  Function is reentrant and fast enough to be called from a static initializer.
   */
  inline void add_registration_callback(_registration_callback callback) GL_HOT_INLINE_FLATTEN;
  
 private:


  //////////////////////////////////////////////////////////////////////////////
  // Registered model lookups. 
  typedef std::function<std::shared_ptr<model_base>()> model_creation_function; 
  std::unordered_map<std::string, model_creation_function> m_model_by_name;


  //////////////////////////////////////////////////////////////////////////////
  // Registered function lookups.
  
  std::unique_ptr<method_registry<void> > m_function_registry; 


  ///////////////////////////////////////////////////////////////////////////////
  // TODO: Registered function lookups.
  //

  /** Lock to ensure that model registration is queued correctly.
   */
  std::mutex m_model_registration_lock;
  std::type_index m_last_model_registered = std::type_index(typeid(void));

  /** An intermediate buffer of registration callbacks.  
   *
   *  These queues are used on library load to register callback functions, which
   *  are then processed when any model is requested to ensure that library loading
   *  is done efficiently.  check_registered_callback_queue() should be called 
   *  before any lookups are done to ensure that all possible lookups have been 
   *  registered.
   *
   */
  std::array<_registration_callback, 512> m_registration_callback_list; 
  std::atomic<size_t> m_callback_pushback_index;
  std::atomic<size_t> m_callback_last_processed_index;

  /** Process the registered callbacks.
   *
   *  First performs a fast inline check to see if it's needed, so 
   *  this function can be called easily.
   */
  inline void check_registered_callback_queue();

  /** Does the work of registering things with the callbacks. 
   */
  void _process_registered_callbacks_internal();
};

/////////////////////////////////////////////////////////////////////////////////
//
// Implementations of inline functions for the model server class
//

/** Fast inline check 
 */ 
inline void model_server_impl::check_registered_callback_queue() { 
  if(m_callback_last_processed_index < m_callback_pushback_index) { 
    _process_registered_callbacks_internal(); 
  }
}

/** Add the callback to the registration function. 
 *
 *  This works by putting the callback function into a round-robin queue to avoid
 *  potential allocations or deallocations during library load time and to 
 *  preserve thread safety.
 */
inline void model_server_impl::add_registration_callback(
  model_server_impl::_registration_callback callback) {
      

  size_t insert_index_raw = (m_callback_pushback_index++);
  
  do {
    // Check to make sure this can be safely inserted.
    size_t processed_index_raw = m_callback_last_processed_index;

    // Check to make sure we aren't so far behind the number of actually 
    // registered callbacks that we're out of space. 
    if(processed_index_raw + m_registration_callback_list.size() > insert_index_raw) { 
      break; 
    } else {
      // This will process the next block of insertions.
      _process_registered_callbacks_internal();
    }

  } while(true);

  size_t insert_index = insert_index_raw % m_registration_callback_list.size();

  ASSERT_TRUE(m_registration_callback_list[insert_index] == nullptr);
  m_registration_callback_list[insert_index] = callback;
}


/** Registration of new models.
*
*  A model is registered through a call to register new model, which 
*  instantiates it and populates the required options and method call 
*  lookups.  Copies of these options and method call lookups are stored 
*  internally in a registry here so new models can be instantiated quickly.
*
*   
*  The new model's name() method provides the name of the model being 
*  registered.
*  
*  This method can be called at any point.
*
*/
template <typename ModelClass> void model_server_impl::register_new_model() {

  // Quick check to cut out duplicate registrations.  This can 
  // happen, e.g. if the class or the function macros appear in a header,  
  // which is fine and something we are designed to handle.  
  // However, this means that multiple registration calls can occur for the same 
  // class, and this quickly filters those registrations out. 
  if(std::type_index(typeid(ModelClass)) == m_last_model_registered) { 
    return;
  }
  m_last_model_registered = std::type_index(typeid(ModelClass)); 

  // TODO: As the registration is now performed in the constructor, 
  // a base instantiated version of the class should be held, then 
  // subsequent model creations should simply use the copy constructor to 
  // instantiate them.  This means the entire method registry is not 
  // duplicated.  For now, just go through this way.  
  const std::string& name = ModelClass().name(); 

  model_creation_function mcf = [=](){ return this->create_model<ModelClass>(); };

  m_model_by_name.insert({name, mcf});
}


/** Instantiate a previously registered model by type.
 */
template <typename ModelType> 
  std::shared_ptr<ModelType> model_server_impl::create_model() {

  // Make sure there aren't new models waiting on the horizon.
  check_registered_callback_queue();

  return std::make_shared<ModelType>();
}


/**  Register a new function.
 */
template <typename Function, typename... ParamSpecs>
   void model_server_impl::register_new_function(
         const std::string& name, Function&& function, ParamSpecs&&... param_specs) { 
  m_function_registry->register_method(name, function, param_specs...);   
}
   
/**  Call the function.
 */
template <typename... FunctionArgs>
  variant_type model_server_impl::call_function(
      const std::string& function_name, FunctionArgs&&... args) {

     // Make sure there aren't new functions waiting on the horizon.
    check_registered_callback_queue();

    return m_function_registry->call_method<void>(nullptr, function_name, args...);
}

} 
} // End turi namespace
#endif
