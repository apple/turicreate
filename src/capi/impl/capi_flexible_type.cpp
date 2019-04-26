/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <flexible_type/flexible_type.hpp>
#include <export.hpp>

extern "C" {

EXPORT tc_flexible_type* tc_ft_create_empty(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(turi::FLEX_UNDEFINED);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_copy(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(ft->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_cstring(const char* str, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(str);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_string(const char* str, uint64_t n, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(turi::flex_string(str, size_t(n)));

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_double(double v, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(v);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_int64(int64_t v, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(v);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_double_array(
    const double* data, uint64_t n, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flexible_type(turi::flex_vec(data, data + n));

  ERROR_HANDLE_END(error, NULL);
}

// Conversion to flexible type from flex list.
EXPORT tc_flexible_type* tc_ft_create_from_flex_list(const tc_flex_list* fl, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, fl, "Flex list", NULL);

  return new_tc_flexible_type(fl->value);

  ERROR_HANDLE_END(error, NULL);
}


EXPORT tc_flexible_type* tc_ft_create_from_datetime(const tc_datetime* dt, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();
  
  CHECK_NOT_NULL(error, dt, "tc_datetime", NULL);

  return new_tc_flexible_type(dt->value);

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_create_from_flex_dict(const tc_flex_dict* fd, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, fd, "tc_flex_dict", NULL);

  return new_tc_flexible_type(fd->value);

  ERROR_HANDLE_END(error, NULL);
}

// Create a flexible type from an image
EXPORT tc_flexible_type* tc_ft_create_from_image(const tc_flex_image* image, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, image, "tc_flex_image", NULL);

  return new_tc_flexible_type(image->value);

  ERROR_HANDLE_END(error, NULL);
}

// Create a flexible type from an ndarray
EXPORT tc_flexible_type* tc_ft_create_from_ndarray(const tc_ndarray* nda, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, nda, "tc_ndarray", NULL);

  return new_tc_flexible_type(nda->value);

  ERROR_HANDLE_END(error, NULL);
}

/****************************************************/

EXPORT tc_ft_type_enum tc_ft_type(const tc_flexible_type* ft) {
  return static_cast<tc_ft_type_enum>(ft->value.get_type());
}

EXPORT bool tc_ft_is_string(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::STRING);
}

EXPORT bool tc_ft_is_double(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::FLOAT);
}

EXPORT bool tc_ft_is_int64(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::INTEGER);
}

EXPORT bool tc_ft_is_image(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::IMAGE);
}

EXPORT bool tc_ft_is_array(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::VECTOR);
}

EXPORT bool tc_ft_is_datetime(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::DATETIME);
}

EXPORT bool tc_ft_is_dict(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::DICT);
}

EXPORT bool tc_ft_is_list(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::LIST);
}

EXPORT bool tc_ft_is_undefined(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::UNDEFINED);
}

EXPORT bool tc_ft_is_ndarray(const tc_flexible_type* ft) {
  return (ft != NULL) && (ft->value.get_type() == turi::flex_type_enum::ND_VECTOR);
}

EXPORT bool tc_ft_is_type(const tc_flexible_type* ft, tc_ft_type_enum t) {
  return (ft != NULL) && (ft->value.get_type() == static_cast<turi::flex_type_enum>(t));
}

/****************************************************/

EXPORT double tc_ft_double(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);
  return ft->value.to<double>(); 

  ERROR_HANDLE_END(error, NULL);
}

EXPORT int64_t tc_ft_int64(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  return ft->value.to<int64_t>();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_ft_string_length(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(ft == NULL) {
    set_error(error, "Flexible type is null");
    return NULL;
  }

  if(ft->value.get_type() != turi::flex_type_enum::STRING) {
    set_error(error, "Flexible type not a string.");
    return 0;
  }

  return ft->value.get<turi::flex_string>().size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const char* tc_ft_string_data(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(ft->value.get_type() != turi::flex_type_enum::STRING) {
    set_error(error, "Flexible type not a string.");
    return NULL;
  }

  return ft->value.get<turi::flex_string>().data();

  ERROR_HANDLE_END(error, NULL);
}


EXPORT uint64_t tc_ft_array_length(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(ft == NULL) {
    set_error(error, "Flexible type is null");
    return NULL;
  }

  if(ft->value.get_type() != turi::flex_type_enum::VECTOR) {
    set_error(error, "Flexible type not an array.");
    return 0;
  }

  return ft->value.get<turi::flex_vec>().size();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT const double* tc_ft_array_data(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(ft->value.get_type() != turi::flex_type_enum::VECTOR) {
    set_error(error, "Flexible type not an array.");
    return NULL;
  }

  return ft->value.get<turi::flex_vec>().data();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_ft_flex_list(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if(ft->value.get_type() != turi::flex_type_enum::LIST) {
    set_error(error, "Flexible type not a list.");
    return NULL;
  }

  return new_tc_flex_list(ft->value.get<turi::flex_list>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_dict* tc_ft_flex_dict(const tc_flexible_type* ft, tc_error **error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (ft->value.get_type() != turi::flex_type_enum::DICT) {
    set_error(error, "Flexible type not a dict.");
    return NULL;
  }

  return new_tc_flex_dict(ft->value.get<turi::flex_dict>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_image* tc_ft_flex_image(const tc_flexible_type* ft, tc_error **error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (ft->value.get_type() != turi::flex_type_enum::IMAGE) {
    set_error(error, "Flexible type not an image.");
    return NULL;
  }

  return new_tc_flex_image(ft->value.get<turi::flex_image>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_datetime* tc_ft_datetime(const tc_flexible_type* ft, tc_error **error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (ft->value.get_type() != turi::flex_type_enum::DATETIME) {
    set_error(error, "Flexible type not a datetime object.");
    return NULL;
  }

  return new_tc_datetime(ft->value.get<turi::flex_date_time>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_ndarray* tc_ft_ndarray(const tc_flexible_type* ft, tc_error **error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  if (ft->value.get_type() != turi::flex_type_enum::ND_VECTOR) {
    set_error(error, "Flexible type not an ndarray object.");
    return NULL;
  }

  return new_tc_ndarray(ft->value.get<turi::flex_nd_vec>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_to_string(const tc_flexible_type* ft, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  return new_tc_flexible_type(ft->value.to<turi::flex_string>());

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flexible_type* tc_ft_to_type(const tc_flexible_type* ft, tc_ft_type_enum t, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  CHECK_NOT_NULL(error, ft, "Flexible type", NULL);

  turi::flexible_type out(static_cast<turi::flex_type_enum>(t));
  out.soft_assign(ft->value);
  return new_tc_flexible_type(out);

  ERROR_HANDLE_END(error, NULL);
}

// close out the extern "C" condition.
}
