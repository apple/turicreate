/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <core/export.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

extern "C" {

/******************************************************************************/
/*                                                                            */
/*    flex_enum_list                                                               */
/*                                                                            */
/******************************************************************************/


EXPORT tc_flex_enum_list* tc_flex_enum_list_create(tc_error** error) {
ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flex_enum_list();

ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_enum_list* tc_flex_enum_list_create_with_capacity(uint64_t capacity, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  tc_flex_enum_list* ret = new tc_flex_enum_list;
  ret->value.reserve(capacity);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}

/**
 */
EXPORT uint64_t tc_flex_enum_list_add_element(tc_flex_enum_list* fl, const tc_ft_type_enum ft,
   tc_error** error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(fl == NULL) {
    set_error(error, "tc_flex_enum_list instance null.");
    return uint64_t(-1);
  }

  uint64_t pos = fl->value.size();
  fl->value.push_back(static_cast<turi::flex_type_enum>(ft));

  return uint64_t(pos);

  ERROR_HANDLE_END(error, uint64_t(-1));
}

/** Extract an element at a specific index.
 *
 */
EXPORT tc_ft_type_enum tc_flex_enum_list_extract_element(
    const tc_flex_enum_list* fl, uint64_t index, tc_error **error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if(fl == NULL) {
    set_error(error, "tc_flex_enum_list instance null.");
    return FT_TYPE_UNDEFINED;
  }

  if(index >= fl->value.size()) {
    set_error(error, "tc_flex_enum_list index out of bounds.");
    return FT_TYPE_UNDEFINED;
  }

  return static_cast<tc_ft_type_enum>(fl->value[index]);

  ERROR_HANDLE_END(error, FT_TYPE_UNDEFINED);
}

EXPORT uint64_t tc_flex_enum_list_size(const tc_flex_enum_list* fl) {
  if(fl == NULL) { return 0; }
  return fl->value.size();
}

}
