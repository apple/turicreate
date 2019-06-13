/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstring>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <core/export.hpp>

// these are implementation of xgboost error hanlding
// so that xgboost's Error, Check, Assert, Printf
// will be redirected to turicreate's engine
namespace xgboost {
namespace utils {
/*!
 * \brief handling of Assert error, caused by in-apropriate input
 * \param msg error message
 */
EXPORT void HandleAssertError(const char *msg) {
  ASSERT_MSG(false, "%s", msg);
}
/*!
 * \brief handling of Check error, caused by in-apropriate input
 * this need to go back to user
 * \param msg error message
 */
EXPORT void HandleCheckError(const char *msg) {
  std::string errmsg = msg;
  const char *errlabel = "label must be in [0,1]";
  if (!strncmp(msg, errlabel, strlen(errlabel))) {
    errmsg = "The 'target_column' has to contain only [0, 1] value";
  }
  log_and_throw(errmsg);
}

EXPORT void HandlePrint(const char *msg) {
  logprogress_stream << msg << std::endl;
}
}  // namespace utils
}  // namespace xgboost
