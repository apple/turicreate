/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FAULT_SOCKETS_GET_NEXT_PORT_NUMBER_HPP
#define FAULT_SOCKETS_GET_NEXT_PORT_NUMBER_HPP
#include <stdint.h>
#include <cstdlib>
namespace turi {
namespace nanosockets {

size_t get_next_port_number();

} // namespace nanosockets
}
#endif
