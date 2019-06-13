/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_LAMBDA_UTILS_HPP
#define TURI_LAMBDA_LAMBDA_UTILS_HPP

#include<core/system/cppipc/common/message_types.hpp>

namespace turi {
namespace lambda {

/**
 * Helper function to convert an communication failure exception to a user
 * friendly message indicating a failure lambda execution.
 */
inline cppipc::ipcexception reinterpret_comm_failure(cppipc::ipcexception e) {
  const char* message = "Fail executing the lambda function. The lambda worker may have run out of memory or crashed because it captured objects that cannot be properly serialized.";
  if (e.get_reply_status() == cppipc::reply_status::COMM_FAILURE) {
    return cppipc::ipcexception(cppipc::reply_status::EXCEPTION,
                                e.get_zeromq_errorcode(),
                                message);
  } else {
    return e;
  }
}

}
}

#endif
