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
/*    flex_list                                                               */
/*                                                                            */
/******************************************************************************/

EXPORT tc_flex_list* tc_flex_list_create(tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  return new_tc_flex_list();

  ERROR_HANDLE_END(error, NULL);
}

EXPORT tc_flex_list* tc_flex_list_create_with_capacity(uint64_t capacity, tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  tc_flex_list* ret = new_tc_flex_list();
  ret->value.reserve(capacity);
  return ret;

  ERROR_HANDLE_END(error, NULL);
}

/**
 */
EXPORT uint64_t tc_flex_list_add_element(tc_flex_list* fl, const tc_flexible_type* ft,
                                         tc_error** error) {
  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (fl == NULL) {
    set_error(error, "tc_flex_list instance null.");
    return uint64_t(-1);
  }

  if (ft == NULL) {
    set_error(error, "tc_flexible_type instance null.");
    return uint64_t(-1);
  }

  uint64_t pos = fl->value.size();
  fl->value.push_back(ft->value);
  return pos;

  ERROR_HANDLE_END(error, uint64_t(-1));
}

/** Extract an element at a specific index.
 *
 */
EXPORT tc_flexible_type* tc_flex_list_extract_element(
    const tc_flex_list* fl, uint64_t index, tc_error **error) {

  ERROR_HANDLE_START();
  turi::ensure_server_initialized();

  if (fl == NULL) {
    set_error(error, "tc_flex_list instance null.");
    return NULL;
  }

  tc_flexible_type* ft = tc_ft_create_empty(error);

  if(*error) { return NULL; }

  if (index >= fl->value.size()) {
    set_error(error, "tc_flex_list index out of bounds.");
    return NULL;
  }

  ft->value = fl->value[index];

  return ft;

  ERROR_HANDLE_END(error, NULL);
}

EXPORT uint64_t tc_flex_list_size(const tc_flex_list* fl) {
  if(fl == NULL) { return 0; }
  return fl->value.size();
}

}

