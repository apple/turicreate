/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LOGGER_FAIL_METHOD
#include <string>

#ifdef TURI_LOGGER_THROW_ON_FAILURE
#define TURI_LOGGER_FAIL_METHOD(str) throw(str)
#define LOGGED_TURI_LOGGER_FAIL_METHOD(str) log_and_throw(str)
#else
#define TURI_LOGGER_FAIL_METHOD(str) abort()
#define LOGGED_TURI_LOGGER_FAIL_METHOD(str) abort()
#endif

#endif
