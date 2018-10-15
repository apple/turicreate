/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

/*
 * Error types to distinguish Turi Create errors from arbitrary C++ exceptions
 */

#include <logger/error.hpp>

using namespace turi::error;

io_error::io_error(const std::string &message)
  : std::ios_base::failure(message), m_message(message) {}

const char *io_error::what() const noexcept {
  return m_message.c_str();
}