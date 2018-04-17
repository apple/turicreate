/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_CAPI_INITIALIZATION_INTERNAL
#define TURI_CAPI_INITIALIZATION_INTERNAL

namespace turi {
void _tc_initialize();

extern bool capi_server_initialized;

static inline void ensure_server_initialized() {
  if (!capi_server_initialized) {
    _tc_initialize();
  }
}

}  // namespace turi
#endif
