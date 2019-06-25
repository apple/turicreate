/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>

#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/export.hpp>
#include <model_server/lib/unity_global.hpp>
#include <model_server/lib/unity_global_singleton.hpp>

EXPORT tc_variant* tc_function_call(
    const char* function_name, const tc_parameters* arguments,
    tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  turi::variant_map_type params = arguments->value;
  turi::toolkit_function_response_type response =
      turi::get_unity_global_singleton()->run_toolkit(function_name, params);
  if (!response.success) {
    set_error(error, response.message);
    return nullptr;
  } else {
    turi::variant_type result = std::move(response.params["return_value"]);
    tc_variant* ret = new_tc_variant();
    ret->value = std::move(result);
    return ret;
  }

  ERROR_HANDLE_END(error, nullptr);
}




