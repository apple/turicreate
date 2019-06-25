/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/export.hpp>
#include <model_server/lib/toolkit_util.hpp>


/******************************************************************************/
/*                                                                            */
/*   Parameter List                                                           */
/*                                                                            */
/******************************************************************************/
extern "C" {

// Create a new set of parameters
EXPORT tc_parameters* tc_parameters_create_empty(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_parameters();

  ERROR_HANDLE_END(error, NULL); 
} 

// Add a new value to the set of parameters
EXPORT void tc_parameters_add(tc_parameters* params, const char* name, const tc_variant* variant, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, variant, "tc_variant");

  params->value[name] = variant->value;

  ERROR_HANDLE_END(error);
}

// Returns true if an entry exists, false otherwise   
EXPORT bool tc_parameters_entry_exists(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  return (params->value.find(name) != params->value.end());

  ERROR_HANDLE_END(error, false); 
} 

// Retrieves a variant from the set of parameters
EXPORT tc_variant* tc_parameters_retrieve(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  tc_variant* res = new_tc_variant();
  res->value = params->value.at(name);
  return res;

  ERROR_HANDLE_END(error, NULL);
}

// Convenient specializations of tc_parameters_add
EXPORT void tc_parameters_add_int64(tc_parameters* params, const char* name, int64_t value, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");

  params->value[name] = turi::to_variant(value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_double(tc_parameters* params, const char* name, double value, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");

  params->value[name] = turi::to_variant(value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_cstring(tc_parameters* params, const char* name, const char* str, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, str, "cstring");

  params->value[name] = turi::to_variant(turi::flex_string(str));

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_string(tc_parameters* params, const char* name, const char* str, uint64_t n, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, str, "string");

  params->value[name] = turi::to_variant(turi::flex_string(str, n));

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_double_array(tc_parameters* params, const char* name, const double* data, uint64_t n, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, data, "data");

  params->value[name] = turi::to_variant(turi::flex_vec(data, data + n));

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_flex_list(tc_parameters* params, const char* name, const tc_flex_list* fl, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, fl, "tc_flex_list");

  params->value[name] = turi::to_variant(fl->value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_flex_dict(tc_parameters* params, const char* name, const tc_flex_dict* fd, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, fd, "tc_flex_dict");

  params->value[name] = turi::to_variant(fd->value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_datetime(tc_parameters* params, const char* name, const tc_datetime* dt, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, dt, "tc_datetime");

  params->value[name] = turi::to_variant(dt->value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_image(tc_parameters* params, const char* name, const tc_flex_image* fi, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, fi, "tc_flex_image");

  params->value[name] = turi::to_variant(turi::flexible_type(fi->value));

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_flexible_type(tc_parameters* params, const char* name, const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, ft, "tc_flexible_type");

  params->value[name] = ft->value;

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_sarray(tc_parameters* params, const char* name, const tc_sarray* sa, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, sa, "tc_sarray");

  params->value[name] = turi::to_variant(sa->value.get_proxy());

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_sframe(tc_parameters* params, const char* name, const tc_sframe* sf, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, sf, "tc_sframe");

  params->value[name] = turi::to_variant(sf->value.get_proxy());

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_parameters(tc_parameters* params, const char* name, const tc_parameters* p, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, p, "tc_parameters");

  params->value[name] = turi::to_variant(p->value);

  ERROR_HANDLE_END(error); 
}

EXPORT void tc_parameters_add_model(tc_parameters* params, const char* name, const tc_model* m, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters");
  CHECK_NOT_NULL(error, m, "tc_model");

  params->value[name] = turi::to_variant(m->value);

  ERROR_HANDLE_END(error); 
}

EXPORT bool tc_parameters_is_int64(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::INTEGER;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_double(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::FLOAT;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_cstring(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::STRING;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_string(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::STRING;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_double_array(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::VECTOR;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_flex_list(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::LIST;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_flex_dict(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::DICT;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_datetime(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::DATETIME;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_image(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  const turi::variant_type& v = params->value.at(name);
  return v.which() == 0 && turi::variant_get_ref<turi::flexible_type>(v).get_type() == turi::flex_type_enum::IMAGE;

  ERROR_HANDLE_END(error, false); 
}


// Query the type of the parameter 
EXPORT bool tc_parameters_is_flexible_type(const tc_parameters* params,
    const char* name, tc_error** error) {  
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 0; 

  ERROR_HANDLE_END(error, false); 
} 

// Query the type of the parameter 
EXPORT bool tc_parameters_is_sarray(const tc_parameters* params,
      const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 5; 

  ERROR_HANDLE_END(error, false); 
}


// Query the type of the parameter
EXPORT bool tc_parameters_is_sframe(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 4; 

  ERROR_HANDLE_END(error, false); 
} 


EXPORT bool tc_parameters_is_parameters(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 6;

  ERROR_HANDLE_END(error, false); 
}


EXPORT bool tc_parameters_is_model(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", false);

  // TODO: Make less ugly!
  return params->value.at(name).which() == 3;

  ERROR_HANDLE_END(error, false); 
}


EXPORT int64_t tc_parameters_retrieve_int64(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_int>();

  ERROR_HANDLE_END(error, NULL); 
}
 
EXPORT double tc_parameters_retrieve_double(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_float>();

  ERROR_HANDLE_END(error, NULL); 
}
 
EXPORT tc_flexible_type* tc_parameters_retrieve_string(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flexible_type(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_string>());

  ERROR_HANDLE_END(error, NULL); 
}

EXPORT tc_flexible_type* tc_parameters_retrieve_array(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flexible_type(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_vec>());

  ERROR_HANDLE_END(error, NULL); 
}
 
EXPORT tc_flex_list* tc_parameters_retrieve_flex_list(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flex_list(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_list>());

  ERROR_HANDLE_END(error, NULL); 
}

EXPORT tc_flex_dict* tc_parameters_retrieve_flex_dict(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flex_dict(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_dict>());

  ERROR_HANDLE_END(error, NULL); 
}

EXPORT tc_datetime* tc_parameters_retrieve_datetime(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_datetime(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_date_time>());

  ERROR_HANDLE_END(error, NULL); 
}

EXPORT tc_flex_image* tc_parameters_retrieve_image(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flex_image(turi::variant_get_ref<turi::flexible_type>(v).get<turi::flex_image>());

  ERROR_HANDLE_END(error, NULL); 
}
 
// Retrieve the value of an sframe as returned parameter.
EXPORT tc_flexible_type* tc_parameters_retrieve_flexible_type(
    const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_flexible_type(turi::variant_get_ref<turi::flexible_type>(v));

  ERROR_HANDLE_END(error, NULL); 
}

// Retrieve the value of an sframe as returned parameter.
EXPORT tc_sarray* tc_parameters_retrieve_sarray(const tc_parameters* params, 
    const char* name, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
    
  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_sarray(turi::variant_get_ref<std::shared_ptr<turi::unity_sarray_base> >(v));

  ERROR_HANDLE_END(error, NULL); 
}

// Retrieve the value of an sframe as returned parameter.
EXPORT tc_sframe* tc_parameters_retrieve_sframe(
    const tc_parameters* params, 
    const char* name, tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_sframe(turi::variant_get_ref<std::shared_ptr<turi::unity_sframe_base> >(v));

  ERROR_HANDLE_END(error, NULL); 
}

EXPORT tc_parameters* tc_parameters_retrieve_parameters(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  return new_tc_parameters(turi::variant_get_ref<turi::variant_map_type>(v));

  ERROR_HANDLE_END(error, NULL); 
}
 
EXPORT tc_model* tc_parameters_retrieve_model(const tc_parameters* params, const char* name, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, params, "tc_parameters", NULL);

  const turi::variant_type& v = params->value.at(name);
  const std::shared_ptr<turi::model_base>& model = turi::variant_get_ref<std::shared_ptr<turi::model_base>>(v);
  return new_tc_model(model);

  ERROR_HANDLE_END(error, NULL); 
}

}
