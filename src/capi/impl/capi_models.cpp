#include <capi/impl/capi_models.hpp>
#include <capi/impl/capi_parameters.hpp>
#include <capi/impl/capi_variant.hpp>
#include <capi/impl/capi_error_handling.hpp>

#include <unity/server/unity_server_control.hpp>
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

EXPORT tc_model* tc_model_load(const char* file_name, tc_error** error) {
  

  return NULL;
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




