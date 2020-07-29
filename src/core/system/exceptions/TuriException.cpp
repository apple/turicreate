/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <core/system/exceptions/TuriException.hpp>

namespace {

std::string MakeMessage(turi::TuriErrorCode error_code, const std::string& detail_message)
{
  auto base = turi::ErrorCodeToString(error_code);

  if (!detail_message.empty()) {
    base += ": " + detail_message;
  }

  return base;
}

}  // namespace

namespace turi {

TuriException::TuriException(const TuriException&) = default;
TuriException::TuriException(TuriException&&) = default;

TuriException::~TuriException() = default;

TuriException::TuriException(TuriErrorCode error_code)
  : TuriException(error_code, std::string())
{
}

TuriException::TuriException(TuriErrorCode error_code, std::string detail_message)
  : error_code_(error_code)
  , detail_message_(std::move(detail_message))
  , message_(MakeMessage(error_code_, detail_message_))
{
}

const char* TuriException::what() const throw() { return message_.c_str(); }

const std::string& TuriException::Message() const { return message_; }

TuriErrorCode TuriException::ErrorCode() const noexcept { return error_code_; }

const std::string& TuriException::ErrorDetail() const { return detail_message_; }

}  // namespace turi
