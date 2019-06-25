/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SFRAME_UNITY_SERVER_HPP
#define SFRAME_UNITY_SERVER_HPP

#include <core/parallel/pthread_tools.hpp>
#include <core/util/blocking_queue.hpp>
#include "unity_server_options.hpp"
#include "unity_server_init.hpp"

namespace turi {
// Forward declaration of classes
class toolkit_function_registry;
class toolkit_class_registry;

class unity_server {
 public:
  typedef void(*progress_callback_type)(const std::string&);

  /**
   * Constructor
   */
  unity_server(unity_server_options options);

  /**
   * Start the server object
   */
  void start(const unity_server_initializer& server_initializer);

  /**
   * Stop the server and cleanup state
   */
  void stop();

  /**
   * Enable or disable log progress stream.
   */
  void set_log_progress(bool enable);
  void set_log_progress_callback(progress_callback_type callback);

 private:
  unity_server_options options;
  toolkit_function_registry* toolkit_functions;
  toolkit_class_registry* toolkit_classes;

  volatile progress_callback_type log_progress_callback = nullptr;

  turi::thread log_thread;
  blocking_queue<std::string> log_queue;

}; // end of class usenity_server
} // end of namespace turi

#endif
