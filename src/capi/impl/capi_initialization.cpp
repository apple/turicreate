#include <unity/server/unity_server_control.hpp>
#include <unity/server/unity_server_options.hpp>
#include <unity/server/unity_server.hpp>
#include <capi/impl/capi_initialization.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <capi/impl/capi_error_handling.hpp>

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
}

}

// The user facing components of the server initialization

extern "C" {

EXPORT void tc_setup_log_location(const char* log_file, tc_error** error) {
  ERROR_HANDLE_START();

  std::lock_guard<std::mutex> lg(turi::_capi_server_initializer_lock);

  if (turi::capi_server_initialized) {
    set_error(error, "CAPI server is already initialized; call setup functions before all other functions.");
    return;
  }

  turi::_get_server_options().log_file = log_file;

  ERROR_HANDLE_END(error);
}

}
