/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

/*
 * Error types to distinguish Turi Create errors from arbitrary C++ exceptions
 */

#include <pch/pch.hpp>

namespace turi {
namespace error {

class io_error : public std::ios_base::failure {
  private:
    std::string m_message;

  public:
    explicit io_error(const std::string &message);
#ifdef COMPILER_HAS_IOS_BASE_FAILURE_WITH_ERROR_CODE
    io_error(const std::string &message, const std::error_code& code);
#endif

#ifdef COMPILER_HAS_NOEXCEPT_WHAT_ON_EXCEPTIONS
  virtual const char *what() const noexcept override;
#else
  virtual const char *what() const override;
#endif
};

} // namespace error
} // namespace turi
