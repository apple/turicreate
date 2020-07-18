/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <core/system/exceptions/TuriErrorCode.hpp>

namespace turi {

std::string ErrorCodeToString(TuriErrorCode ec)
{
  switch (ec) {
    //
    // generic codes
    //
    case TuriErrorCode::NotImplemented:
      return "Not implemented";

    case TuriErrorCode::LogicError:
      return "Logic error";

    //
    // argument and extent codes
    //
    case TuriErrorCode::IndexOutOfBounds:
      return "Index out of bounds";

    case TuriErrorCode::MissingArgument:
      return "Missing argument";

    case TuriErrorCode::InvalidBufferLength:
      return "Invalid buffer length";

    case TuriErrorCode::InvalidSize:
      return "Invalid size";

    case TuriErrorCode::InvalidDimensionality:
      return "Invalid dimensionality";

    //
    // graph codes
    //
    case TuriErrorCode::GraphInputsError:
      return "Graph inputs error";

    case TuriErrorCode::GraphOutputsError:
      return "Graph outputs error";

    case TuriErrorCode::GraphCompilationError:
      return "Graph compilation error";

    //
    // externally-mapped codes
    //
    case TuriErrorCode::ImageConversionFailure:
      return "Image conversion failure";

    case TuriErrorCode::Unknown:
    default:
      return "Unknown error";
  }
}

}  // namespace turi
