/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/nanosockets/get_next_port_number.hpp>
#include <cstdlib>
#include <core/export.hpp>
namespace turi {
namespace nanosockets {
#define ZSOCKET_DYNFROM     0xc000
#define ZSOCKET_DYNTO       0xffff
static size_t cur_port = ZSOCKET_DYNFROM;

EXPORT size_t get_next_port_number() {
  size_t ret = cur_port;
  cur_port = (cur_port + 1) <= ZSOCKET_DYNTO ?
                                (cur_port + 1) : ZSOCKET_DYNFROM;
  return ret;
}

} // nanosockets
}
