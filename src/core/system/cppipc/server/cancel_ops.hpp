/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CPPIPC_SERVER_CANCEL_OPS_HPP
#define CPPIPC_SERVER_CANCEL_OPS_HPP
#include <atomic>
#include <iostream>

namespace cppipc {

std::atomic<unsigned long long>& get_srv_running_command();

std::atomic<bool>& get_cancel_bit_checked();

bool must_cancel();

} // cppipc

#endif // CPPIPC_SERVER_CANCEL_OPS_HPP
