/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <string>

namespace turi {

// TuriErrorCodes describe specific error types that don't warrant the creation of custom
// TuriException types.
//
// When adding new error codes, be sure to add human-readable string versions to the
// implementation of ErrorCodeToString.cpp.
//
enum class TuriErrorCode {
  // generic codes
  NotImplemented,
  LogicError,

  // argument and extent codes
  IndexOutOfBounds,
  MissingArgument,
  InvalidBufferLength,
  InvalidSize,
  InvalidDimensionality,

  // graph codes
  GraphInputsError,
  GraphOutputsError,
  GraphCompilationError,

  // externally-mapped codes
  ImageConversionFailure,

  Unknown,
};

std::string ErrorCodeToString(TuriErrorCode ec);

}  // namespace turi
