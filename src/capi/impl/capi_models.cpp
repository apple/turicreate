#include <capi/impl/capi_models.hpp>
#include <capi/impl/capi_parameters.hpp>
#include <capi/impl/capi_variant.hpp>
#include <capi/impl/capi_error_handling.hpp>

#include <unity/server/unity_server_control.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/unity_global.hpp>


/******************************************************************************/
/*                                                                            */
/*   Models                                                                   */
/*                                                                            */
/******************************************************************************/

struct tc_model_struct; 
typedef struct tc_model_struct tc_model;

EXPORT void tc_initialize(const char* log_file, tc_error** error) {
  // Set up the unity server
  ERROR_HANDLE_START();

  // Initialize the server -- set up default environment values
  turi::unity_server_options s_opts;

  s_opts.log_file = log_file;
  s_opts.root_path = "";
  s_opts.daemon = false;
  s_opts.log_rotation_interval = 0;
  s_opts.log_rotation_truncate = 0;

  turi::start_server(s_opts);

  ERROR_HANDLE_END(error);
}

EXPORT tc_model* tc_model_new(const char* model_name, tc_error** error) { 
  ERROR_HANDLE_START();

  std::shared_ptr<turi::model_base> model 
    = turi::get_unity_global_singleton()->create_toolkit_class(model_name); 

  return new_tc_model(std::move(model));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_model* tc_model_load(const char* url, tc_error** error) {
  ERROR_HANDLE_START();
  CHECK_NOT_NULL(error, url, "url", nullptr);

  turi::variant_map_type result =
      turi::get_unity_global_singleton()->load_model(url);
  turi::flex_int version =
      turi::safe_varmap_get<turi::flexible_type>(result, "archive_version");
  switch (version) {
  case 0:
    return new_tc_model(
        turi::safe_varmap_get<std::shared_ptr<turi::model_base>>(
            result, "model_base"));
  case 1:
    return new_tc_model(
        turi::safe_varmap_get<std::shared_ptr<turi::model_base>>(
            result, "model"));
  default:
    set_error(error, "Unknown model archive version");
    return nullptr;
  }

  ERROR_HANDLE_END(error, nullptr);
}

EXPORT void tc_model_save(
    const tc_model* model, const char* url, tc_error** error) {
  ERROR_HANDLE_START();
  CHECK_NOT_NULL(error, model, "model");
  CHECK_NOT_NULL(error, url, "url");

  turi::get_unity_global_singleton()->save_model(model->value, {}, url);

  ERROR_HANDLE_END(error);
}

EXPORT const char* tc_model_name(const tc_model* model, tc_error **error) { 
  ERROR_HANDLE_START();

  return model->value->name().c_str();
  ERROR_HANDLE_END(error, "");
}

EXPORT tc_variant* tc_model_call_method(const tc_model* model, const char* method, 
                                           const tc_parameters* arguments, tc_error** error) {
  
  ERROR_HANDLE_START();

  turi::variant_type result = model->value->call_function(method, arguments->value); 

  return new_tc_variant(result);
  
  ERROR_HANDLE_END(error, NULL);
}


EXPORT void tc_model_destroy(tc_model* model) {
  delete model; 
}




