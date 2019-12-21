/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server_v2/model_server.hpp>


namespace turi {
namespace v2 { 

EXPORT model_server_impl& model_server() { 
  static model_server_impl global_model_server;
  return global_model_server;
}


model_server_impl::model_server_impl() 
 : m_function_registry(new method_registry<void>)
{ 


}

/** Does the work of registering things with the callbacks. 
 */
void model_server_impl::_process_registered_callbacks_internal() {

  std::lock_guard<std::mutex> _lg(m_model_registration_lock);

  size_t cur_idx; 

  while( (cur_idx = m_callback_last_processed_index) != m_callback_pushback_index) { 

    
    // Call the callback function to perform the registration, simultaneously 
    // zeroing out the pointer. 
    _registration_callback reg_f = nullptr;
    size_t idx = cur_idx % m_registration_callback_list.size();
    std::swap(reg_f, m_registration_callback_list[idx]);
    reg_f(*this);

    // We're done here; advance.
    ++m_callback_last_processed_index; 
  }
}

/** Instantiate a previously registered model by name.
 */
std::shared_ptr<model_base> model_server_impl::create_model(const std::string& model_name) {

  // Make sure there aren't new models waiting on the horizon.
  check_registered_callback_queue();

  auto it = m_model_by_name.find(model_name); 

  if(it == m_model_by_name.end()) {
    // TODO: make this more informative.
    throw std::invalid_argument("Model not recognized.");
  }

  return it->second();
}


}
}
