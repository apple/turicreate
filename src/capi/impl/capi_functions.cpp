#include <capi/TuriCore.h>

#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <platform/export.hpp>
#include <unity/lib/unity_global.hpp>
#include <unity/lib/unity_global_singleton.hpp>

EXPORT tc_variant* tc_function_call(
    const char* function_name, const tc_parameters* arguments,
    tc_error** error) {

  ERROR_HANDLE_START();

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




