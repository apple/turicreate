/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <exception>
#include <string>

#include <core/system/exceptions/TuriErrorCode.hpp>

namespace turi {

class TuriException : public std::exception {
 public:
  TuriException() = delete;
  TuriException& operator=(const TuriException&) = delete;
  TuriException& operator=(TuriException&&) = delete;

  ~TuriException() override;

  TuriException(const TuriException&);
  TuriException(TuriException&&);

  explicit TuriException(TuriErrorCode error_code);
  TuriException(TuriErrorCode error_code, std::string detail_message);

  const char* what() const throw() override;
  const std::string& Message() const;

  TuriErrorCode ErrorCode() const noexcept;
  const std::string& ErrorDetail() const;

 private:
  const TuriErrorCode error_code_;
  const std::string detail_message_;
  const std::string message_;
};

}  // namespace turi
