/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <fault/zmq/print_zmq_error.hpp>
#include <logger/logger.hpp>
namespace libfault {

void print_zmq_error(const char* prefix) {
  logstream(LOG_ERROR) << prefix << ": Unexpected socket error(" << zmq_errno()
            << ") = " << zmq_strerror(zmq_errno()) << std::endl;
}

}
