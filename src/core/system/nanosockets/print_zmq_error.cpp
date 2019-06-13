/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/nanosockets/print_zmq_error.hpp>
#include <core/logging/logger.hpp>
#include <iostream>

extern "C" {
#include <nanomsg/nn.h>
}
namespace turi {
namespace nanosockets {

void print_zmq_error(const char* prefix) {
  logstream(LOG_ERROR) << prefix << ": Unexpected socket error(" << nn_errno()
            << ") = " << nn_strerror(nn_errno()) << std::endl;
}

}
}
