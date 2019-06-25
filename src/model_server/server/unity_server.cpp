/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <time.h>
#include <unistd.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/system/cppipc/common/authentication_token_method.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/log_rotate.hpp>
#include <model_server/lib/unity_global.hpp>
#include <model_server/lib/unity_global_singleton.hpp>
#include <model_server/lib/toolkit_class_registry.hpp>
#include <model_server/lib/toolkit_function_registry.hpp>
#include <core/system/startup_teardown/startup_teardown.hpp>
#ifdef TC_HAS_PYTHON
#include <core/system/lambda/lambda_master.hpp>
#endif

#include "unity_server.hpp"

namespace turi {

unity_server::unity_server(unity_server_options options) : options(options) {
  toolkit_functions = new toolkit_function_registry();
  toolkit_classes = new toolkit_class_registry();
}

void unity_server::start(const unity_server_initializer& server_initializer) {

  // log files
  if (!options.log_file.empty()) {
    if (options.log_rotation_interval) {
      turi::begin_log_rotation(options.log_file,
                                   options.log_rotation_interval,
                                   options.log_rotation_truncate);
    } else {
      global_logger().set_log_file(options.log_file);
    }
  }

  turi::configure_global_environment(options.root_path);
  turi::global_startup::get_instance().perform_startup();

  // initialize built-in data structures, toolkits and models,
  // defined in registration.cpp
  server_initializer.init_toolkits(*toolkit_functions);
  server_initializer.init_models(*toolkit_classes);

  create_unity_global_singleton(toolkit_functions,
                                toolkit_classes);

  auto unity_global_ptr = get_unity_global_singleton();

  // initialize extension modules and lambda workers
  server_initializer.init_extensions(options.root_path, unity_global_ptr);

#ifdef TC_HAS_PYTHON
  lambda::set_pylambda_worker_binary_from_environment_variables();
#endif

  log_thread.launch([=]() {
                      do {
                        std::pair<std::string, bool> queueelem = this->log_queue.dequeue();
                        if (queueelem.second == false) {
                          break;
                        } else {
                          // we need to read it before trying to do the callback
                          // Otherwise we might accidentally call a null pointer
                          volatile progress_callback_type cback = this->log_progress_callback;
                          if (cback != nullptr) cback(queueelem.first);
                        }
                      } while(1);
                    });
}

/**
 * Cleanup the server state
 */
void unity_server::stop() {
  set_log_progress(false);
  log_queue.stop_blocking();
  turi::global_teardown::get_instance().perform_teardown();
}

EXPORT void unity_server::set_log_progress(bool enable) {
  global_logger().add_observer(LOG_PROGRESS, NULL);
  if (enable == true) {
    // set the progress observer
    global_logger().add_observer(
        LOG_PROGRESS,
        [=](int lineloglevel, const char* buf, size_t len){
          std::cout << "PROGRESS: " << std::string(buf, len);
        });
  }
}

void unity_server::set_log_progress_callback(progress_callback_type callback) {
  if (callback == nullptr) {
    log_progress_callback = nullptr;
    global_logger().add_observer(LOG_PROGRESS, NULL);
  } else {
    log_progress_callback = callback;
    global_logger().add_observer(
        LOG_PROGRESS,
        [=](int lineloglevel, const char* buf, size_t len){
          this->log_queue.enqueue(std::string(buf, len));
        });
  }
}

} // end of turicreate
