/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>

#include <memory>

#include <core/export.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>

extern "C" {

  struct __tc_wrapper_base {
    capi_struct_type_info* type_info;
  };

 // Destructor function -- universal way to call the destructor for any object.
EXPORT void tc_release(void* dt) {

  if(dt) {
    // Not sure how to make sure this won't segfault on bad input.
    reinterpret_cast<const __tc_wrapper_base*>(dt)->type_info->free(dt);
  }
}

}  // extern "C"
