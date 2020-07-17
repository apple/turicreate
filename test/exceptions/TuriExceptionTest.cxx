/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE TuriExceptionTest

#include <core/system/exceptions/TuriException.hpp>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

using turi::TuriErrorCode;
using turi::TuriException;

BOOST_AUTO_TEST_CASE(TestErrorCode)
{
  TuriException e(TuriErrorCode::LogicError);
  TS_ASSERT(TuriErrorCode::LogicError == e.ErrorCode());
  TS_ASSERT_EQUALS("Logic error", std::string(e.what()));
  TS_ASSERT_EQUALS(std::string(e.what()), e.Message());
  TS_ASSERT(e.ErrorDetail().empty());
}

BOOST_AUTO_TEST_CASE(TestErrorCodeWithMessage)
{
  TuriException e(TuriErrorCode::NotImplemented, "le canard dit quack");
  TS_ASSERT(TuriErrorCode::NotImplemented == e.ErrorCode());
  TS_ASSERT_EQUALS("Not implemented: le canard dit quack", std::string(e.what()));
  TS_ASSERT_EQUALS(std::string(e.what()), e.Message());
  TS_ASSERT_EQUALS("le canard dit quack", e.ErrorDetail());
}
