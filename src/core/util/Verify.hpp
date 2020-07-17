/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <core/system/exceptions/TuriException.hpp>

#define VerifyImpl(x, error_code, msg)      \
  do {                                      \
    if (!(x)) {                             \
      throw TuriException(error_code, msg); \
    }                                       \
  } while (0)

#if defined(NDEBUG)
#define VerifyDebugImpl(x, error_code, msg)
#else
#define VerifyDebugImpl(x, error_code, msg) VerifyImpl(x, error_code, msg)
#endif

// VerifyIsTrue - verifies expression x is true in all runtime environments.
// If failure, error_code (TuriErrorCode) will compose the thrown TuriException.
#define VerifyIsTrue(x, error_code) VerifyImpl(x, error_code, std::string())

// VerifyIsTrue - verifies expression x is true in all runtime environments.
// If failure, error_code (TuriErrorCode) and the detailed error message will compose the thrown
// TuriException.
#define VerifyIsTrueWithMessage(x, error_code, msg) VerifyImpl(x, error_code, msg)

// VerifyIsTrue - verifies expression x is true in debug builds only.
// If failure, error_code (TuriErrorCode) will compose the thrown TuriException.
#define VerifyDebugIsTrue(x, error_code) VerifyDebugImpl(x, error_code, std::string())

// VerifyIsTrue - verifies expression x is true in debug builds only.
// If failure, error_code (TuriErrorCode) and the detailed error message will compose the thrown
// TuriException.
#define VerifyDebugIsTrueWithMessage(x, error_code, msg) VerifyDebugImpl(x, error_code, msg)
