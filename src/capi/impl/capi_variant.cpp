/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <model_server/lib/toolkit_util.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/export.hpp>

/******************************************************************************/
/*                                                                            */
/*   Variant List                                                             */
/*                                                                            */
/******************************************************************************/
extern "C" {

EXPORT tc_variant* tc_variant_create_from_int64(int64_t n, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_variant(n);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_variant* tc_variant_create_from_double(double n, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_variant(n);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_cstring(const char* str, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, str, "CString", NULL);

  return new_tc_variant(turi::flex_string(str));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_string(const char* str, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, str, "String", NULL);

  return new_tc_variant(turi::flex_string(str, size_t(n)));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_double_array(const double* data, uint64_t n, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, data, "Double Array", NULL);

  return new_tc_variant(turi::flex_vec(data, data + n));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_flex_list(const tc_flex_list* fl, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, fl, "Flex List", NULL);

  return new_tc_variant(fl->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_flex_dict(const tc_flex_dict* td, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, td, "Flex Dictionary", NULL);

  return new_tc_variant(td->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_variant* tc_variant_create_from_datetime(const tc_datetime* dt, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, turi::flexible_type(dt->value), "Flex Datetime", NULL);

  return new_tc_variant(turi::flexible_type(dt->value));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_variant* tc_variant_create_from_image(const tc_flex_image* fi, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, turi::flexible_type(fi->value), "Flex Image", NULL);

  return new_tc_variant(turi::flexible_type(fi->value));

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_flexible_type(const tc_flexible_type* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flex Type", NULL);

  return new_tc_variant(ft->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_sarray(const tc_sarray* sa, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sa, "SArray", NULL);

  return new_tc_variant(turi::to_variant(sa->value.get_proxy()));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_variant* tc_variant_create_from_sframe(const tc_sframe* sf, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, sf, "SFrame", NULL);

  return new_tc_variant(turi::to_variant(sf->value.get_proxy()));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_variant* tc_variant_create_from_parameters(const tc_parameters* tp, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, tp, "Parameters", NULL);

  return new_tc_variant(tp->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_from_model(const tc_model* tm, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, tm, "Model", NULL);

  return new_tc_variant(tm->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_variant* tc_variant_create_copy(const tc_variant* tv, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, tv, "Variant", NULL);

  return new_tc_variant(tv->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT bool tc_variant_is_int64(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::INTEGER;
}

EXPORT bool tc_variant_is_double(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::FLOAT;
}

EXPORT bool tc_variant_is_cstring(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::STRING;
}

EXPORT bool tc_variant_is_string(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::STRING;
}

EXPORT bool tc_variant_is_double_array(const tc_variant* ft){
  return  turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::VECTOR;
}

EXPORT bool tc_variant_is_flex_list(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::LIST;
}

EXPORT bool tc_variant_is_flex_dict(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::DICT;
}

EXPORT bool tc_variant_is_datetime(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::DATETIME;
}

EXPORT bool tc_variant_is_image(const tc_variant* ft){
  return turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() == turi::flex_type_enum::IMAGE;
}

EXPORT bool tc_variant_is_flexible_type(const tc_variant* ft){
  return (ft->value).which() == 0;
}

EXPORT bool tc_variant_is_sarray(const tc_variant* ft){
  return (ft->value).which() == 5;
}

EXPORT bool tc_variant_is_sframe(const tc_variant* ft){
  return (ft->value).which() == 4;
}

EXPORT bool tc_variant_is_parameters(const tc_variant* ft){
  return (ft->value).which() == 6;
}

EXPORT bool tc_variant_is_model(const tc_variant* ft){
  return (ft->value).which() == 3;
}

EXPORT int64_t tc_variant_int64(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_int>();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT double tc_variant_double(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_float>();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_variant_string_length(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(ft == NULL) {
    set_error(error, "Flexible type is null");
    return NULL;
  }

  if(turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::STRING) {
    set_error(error, "Flexible type not a string.");
    return 0;
  }

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_string>().size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const char* tc_variant_string_data(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::STRING) {
    set_error(error, "Flexible type not a string.");
    return NULL;
  }

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_string>().data();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_variant_array_length(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::VECTOR) {
    set_error(error, "Flexible type not an Array.");
    return NULL;
  }

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_vec>().size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const double* tc_variant_array_data(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::VECTOR) {
    set_error(error, "Flexible type not an Array.");
    return NULL;
  }

  return turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_vec>().data();

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_flex_list* tc_variant_flex_list(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::LIST) {
    set_error(error, "Flexible type not a list.");
    return NULL;
  }

  return new_tc_flex_list(turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_list>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_dict* tc_variant_flex_dict(const tc_variant* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::DICT) {
    set_error(error, "Flexible type not a dict.");
    return NULL;
  }

  return new_tc_flex_dict(turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_dict>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_image* tc_variant_flex_image(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::IMAGE) {
    set_error(error, "Flexible type not an image.");
    return NULL;
  }

  return new_tc_flex_image(turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_image>());

  ERROR_HANDLE_END(error, NULL);
}
EXPORT tc_datetime* tc_variant_datetime(const tc_variant* ft, tc_error** error){
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (turi::variant_get_ref<turi::flexible_type>(ft->value).get_type() != turi::flex_type_enum::DATETIME) {
    set_error(error, "Flexible type not a datetime.");
    return NULL;
  }

  return new_tc_datetime(turi::variant_get_ref<turi::flexible_type>(ft->value).get<turi::flex_date_time>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_variant_flexible_type(
    const tc_variant* var, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  CHECK_NOT_NULL(error, var, "Variant", nullptr);

  if (!tc_variant_is_flexible_type(var)) {
    set_error(error, "Variant does not contain a flexible type.");
    return nullptr;
  }

  return new_tc_flexible_type(
      turi::variant_get_ref<turi::flexible_type>(var->value));

  ERROR_HANDLE_END(error, nullptr);
}

EXPORT tc_sarray* tc_variant_sarray(const tc_variant* var, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  CHECK_NOT_NULL(error, var, "Variant", nullptr);

  if (!tc_variant_is_sarray(var)) {
    set_error(error, "Variant does not contain an SArray.");
    return nullptr;
  }

  return new_tc_sarray(
      turi::variant_get_ref<std::shared_ptr<turi::unity_sarray_base>>(
          var->value));

  ERROR_HANDLE_END(error, nullptr);
}

EXPORT tc_sframe* tc_variant_sframe(const tc_variant* var, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  CHECK_NOT_NULL(error, var, "Variant", nullptr);

  if (!tc_variant_is_sframe(var)) {
    set_error(error, "Variant does not contain an SFrame.");
    return nullptr;
  }

  return new_tc_sframe(
      turi::variant_get_ref<std::shared_ptr<turi::unity_sframe_base>>(
          var->value));

  ERROR_HANDLE_END(error, nullptr);
}

EXPORT tc_parameters* tc_variant_parameters(
    const tc_variant* var, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  CHECK_NOT_NULL(error, var, "Variant", nullptr);

  if (!tc_variant_is_parameters(var)) {
    set_error(error, "Variant does not contain parameters.");
    return nullptr;
  }

  return new_tc_parameters(
      turi::variant_get_ref<turi::variant_map_type>(var->value));

  ERROR_HANDLE_END(error, nullptr);
}

EXPORT tc_model* tc_variant_model(const tc_variant* var, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  CHECK_NOT_NULL(error, var, "Variant", nullptr);

  if (!tc_variant_is_model(var)) {
    set_error(error, "Variant does not contain a model.");
    return nullptr;
  }

  return new_tc_model(
      turi::variant_get_ref<std::shared_ptr<turi::model_base>>(var->value));

  ERROR_HANDLE_END(error, nullptr);
}

}
