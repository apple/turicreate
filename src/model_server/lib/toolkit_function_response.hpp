/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP
#define TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP
#include <string>
#include <model_server/lib/variant.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <future>

namespace turi {

/**
 * \ingroup unity
 * The response from a toolkit
 */
struct toolkit_function_response_type {
  /**
   * Whether the toolkit was executed successfully
   */
  bool success = true;

  /**
   * Any other messages to be printed.
   */
  std::string message;

  /**
   * The returned parameters. (Details will vary from toolkit to toolkit)
   */
  variant_map_type params;

  void save(oarchive& oarc) const {
    log_func_entry();
    oarc << success << message << params;
  }

  void load(iarchive& iarc) {
    log_func_entry();
    iarc >> success >> message >> params;
  }
};

/**
 * \ingroup unity
 * The response from a toolkit executed in the background.
 */
class toolkit_function_response_future {
 public:
   toolkit_function_response_future() {}

   toolkit_function_response_future(std::function<toolkit_function_response_type()> exec_function);

  const toolkit_function_response_type& wait() const;

 private:
  struct response_info {
    toolkit_function_response_type response;
    std::future<bool> response_future;
    volatile bool is_completed = false;
    bool future_finished = false;
  };

  // This field becomes valid and non-null when the execution has
  // finished.
  std::shared_ptr<response_info> m_info;
};


} // turicreate

#endif // TURI_UNITY_TOOLKIT_RESPONSE_TYPE_HPP
