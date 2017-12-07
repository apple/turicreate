/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef ZMQ_MSG_STANDARD_FREE_HPP
#define ZMQ_MSG_STANDARD_FREE_HPP

namespace libfault {

void zmq_msg_standard_free(void* buf, void* hint);

} // libfault

#endif
