#include <model_server/lib/toolkit_function_response.hpp>
#include <iostream>

namespace turi {

toolkit_function_response_future::toolkit_function_response_future(std::function<toolkit_function_response_type()> exec_function) :
  m_info(new response_info) {

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

  if(!m_info->is_completed) { 
     ASSERT_TRUE(m_info->response_future.valid()); 
     m_info->response_future.wait(); 
     ASSERT_EQ(m_info->response_future.get(), true); 
  }

  ASSERT_TRUE(m_info->is_completed);

  return m_info->response;
}

}
