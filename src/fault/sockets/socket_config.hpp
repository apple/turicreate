/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef LIBFAULT_SOCKET_CONFIG_HPP
#define LIBFAULT_SOCKET_CONFIG_HPP
#include <string>
namespace libfault {
extern int SEND_TIMEOUT;
extern int RECV_TIMEOUT;

void set_send_timeout(int ms);
void set_recv_timeout(int ms);

void set_conservative_socket_parameters(void* socket);

extern int64_t FORCE_IPC_TO_TCP_FALLBACK;

/**
 * Normalizes an zeromq address. 
 * On windows, this converts IPc addresses to localhost address.
 */
std::string normalize_address(const std::string& address);
};

#endif
