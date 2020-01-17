#include <model_server/lib/toolkit_function_response.hpp>
#include <iostream>
#include <future>

namespace turi {

toolkit_function_response_future::toolkit_function_response_future(
    std::function<toolkit_function_response_type()> exec_function) 
  : m_response(new std::shared_future<toolkit_function_response_type>(
        std::async(std::launch::async, std::move(exec_function)).share()))
{
  ASSERT_TRUE(m_response->valid());
}

}
