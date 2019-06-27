/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <capi/TuriCreate.h>
#include <model_server/server/unity_server_control.hpp>
#include <model_server/server/unity_server_options.hpp>
#include <model_server/server/unity_server.hpp>
#include <capi/impl/capi_initialization.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <core/globals/globals.hpp>

// All the functions related to the initialization of the server.
namespace turi {

// Create and return the server options.
static unity_server_options& _get_server_options() {
  static std::unique_ptr<turi::unity_server_options> _server_options;
  if (_server_options == nullptr) {

    _server_options.reset(new unity_server_options);

    _server_options->log_file = "/var/log/";
    _server_options->root_path = "";
    _server_options->daemon = false;
    _server_options->log_rotation_interval = 0;
    _server_options->log_rotation_truncate = 0;
  }

  return *_server_options;
}

///////////////////////////////////////////////////////////////////////////////
//
//  The server is initialized on demand.

bool capi_server_initialized = false;

static std::mutex _capi_server_initializer_lock;

EXPORT void _tc_initialize() {

  std::lock_guard<std::mutex> lg(_capi_server_initializer_lock);

  if (capi_server_initialized) {
    return;
  }

  turi::start_server(_get_server_options(), *capi_server_initializer());
  capi_server_initialized = true;

  // Set up the progress logger to log progress to stdout.
  global_logger().add_observer(LOG_PROGRESS,
                               [](int, const char* buf, size_t len) {
                                 for (; len != 0; --len) {
                                   std::cout << *buf;
                                   ++buf;
                                 }
                                 std::cout << std::flush;
                               });
}

}  // namespace turi

// The user facing components of the server initialization

extern "C" {

EXPORT void tc_init_set_log_location(const char* log_file, tc_error** error) {
  ERROR_HANDLE_START();

  std::lock_guard<std::mutex> lg(turi::_capi_server_initializer_lock);

  if (turi::capi_server_initialized) {
    set_error(error, "CAPI server is already initialized; call setup functions before all other functions.");
    return;
  }

  turi::_get_server_options().log_file = log_file;

  ERROR_HANDLE_END(error);
}

EXPORT void tc_init_set_log_callback_function(  //
    tc_log_level log_level,
    void (*callback)(tc_log_level, const char*, uint64_t n), tc_error** error) {
  ERROR_HANDLE_START();

  global_logger().add_observer(
      static_cast<int>(log_level),
      [callback](int log_level, const char* buf, uint64_t len) {
        callback(static_cast<tc_log_level>(log_level), buf, len);
      });

  ERROR_HANDLE_END(error);
}

EXPORT void tc_init_set_config_parameter(const char* parameter,
                                  tc_flexible_type* value, tc_error** error) {

  ERROR_HANDLE_START();

  turi::globals::set_global_error_codes err = turi::globals::set_global(parameter, value->value);

  switch(err) {
    case turi::globals::set_global_error_codes::SUCCESS:
      return;
    case turi::globals::set_global_error_codes::NO_NAME:
      throw std::invalid_argument(std::string("Unknown config parameter ") + parameter);
    case turi::globals::set_global_error_codes::NOT_RUNTIME_MODIFIABLE:
      {
      set_error(error,
                "CAPI server is already initialized; call setup functions "
                "before all other functions.");
      return;
      }
    case turi::globals::set_global_error_codes::INVALID_VAL:
      throw std::invalid_argument(std::string("Invalid value for config parameter ") + parameter);
  }

  ERROR_HANDLE_END(error);
}



}
