/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

/*
 * Error types to distinguish Turi Create errors from arbitrary C++ exceptions
 */

#include <core/logging/error.hpp>

using namespace turi::error;

#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
io_error::io_error(const std::string &message)
: std::ios_base::failure(message, std::error_code()), m_message(message) {}
#else
io_error::io_error(const std::string &message)
: std::ios_base::failure(message), m_message(message) {}
#endif


#ifdef COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS
const char *io_error::what() const noexcept {
#else
const char *io_error::what() const {
#endif
  return m_message.c_str();
}
