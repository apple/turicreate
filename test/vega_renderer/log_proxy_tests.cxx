/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE log_proxy_tests
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include "log_proxy_tests.hpp"

BOOST_AUTO_TEST_SUITE(log_proxy_tests );

BOOST_AUTO_TEST_CASE( test_wrap_unwrap ) {
    LogProxyTests::test_wrap_unwrap();
}

BOOST_AUTO_TEST_CASE( test_expected_property_access ) {
    LogProxyTests::test_expected_property_access();
}

BOOST_AUTO_TEST_CASE( test_unexpected_property_access ) {
    LogProxyTests::test_unexpected_property_access();
}

BOOST_AUTO_TEST_SUITE_END();