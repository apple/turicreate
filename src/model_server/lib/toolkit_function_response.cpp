#include <model_server/lib/toolkit_function_response.hpp>
#include <iostream>

namespace turi {

toolkit_function_response_future::toolkit_function_response_future(
    std::function<toolkit_function_response_type()> exec_function) 
  : m_info(new response_info) {

  // Capture the m_info by value.  Capturing `this` could cause segfaults,
  // as it may not be valid after a copy.  Capturing a shared pointer to
  // the info by value, 
  auto captured_info = this->m_info; 

  m_info->response_future = std::async(std::launch::async, 
   [=]() {
     captured_info->response = exec_function();
     captured_info->is_completed = true;
     return true;
  });

 ASSERT_TRUE(m_info->response_future.valid());
}

const toolkit_function_response_type& toolkit_function_response_future::wait() const {
  ASSERT_TRUE(m_info != nullptr); 

  // The response future is invalid 
  if(!m_info->is_completed) { 
     ASSERT_TRUE(m_info->response_future.valid()); 
     m_info->response_future.wait(); 
     ASSERT_EQ(m_info->response_future.get(), true); 
  }

  ASSERT_TRUE(m_info->is_completed);

  return m_info->response;
}

}
