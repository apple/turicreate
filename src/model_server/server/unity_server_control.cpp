/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <model_server/server/unity_server.hpp>
#include <model_server/server/unity_server_options.hpp>
#include <model_server/server/unity_server_init.hpp>
#include <model_server/server/unity_server_control.hpp>
#include <core/parallel/mutex.hpp>
#include <mutex>

namespace turi {

static mutex _server_start_lock;

// Global embeded server and client object
static unity_server* SERVER = nullptr;

/**
 * Starts the server in the same process.
 *
 * \param root_path directory of the turicreate installation
 * \param server_address the inproc address of the server, could be anything like "inproc://test_server"
 * \param log_file local file for logging
 */
EXPORT void start_server(const unity_server_options& server_options,
                         const unity_server_initializer& server_initializer) {

  std::lock_guard<mutex> server_start_lg(_server_start_lock);

  namespace fs = boost::filesystem;
  global_logger().set_log_level(LOG_PROGRESS);
  global_logger().set_log_to_console(false);

  if(SERVER) {
    logstream(LOG_ERROR) << "Unity server initialized twice." << std::endl;
    return;
  }

  SERVER = new unity_server(server_options);
  SERVER->start(server_initializer);
}


EXPORT void stop_server() {
  logstream(LOG_EMPH) << "Stopping server" << std::endl;
  if (SERVER) {
    SERVER->stop();
    delete SERVER;
    SERVER = NULL;
  }
}

EXPORT void set_log_progress_callback( void (*callback)(const std::string&) ) {
  if (SERVER) {
    SERVER->set_log_progress_callback(callback);
  }
}

/**
 * Enable or disable log progress stream.
 */
EXPORT void set_log_progress(bool enable) {
  if (SERVER) {
    SERVER->set_log_progress(enable);
  }
}

} // end of turicreate
