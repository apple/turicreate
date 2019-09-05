/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "vega_js_test_fixture.hpp"

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#import <visualization/vega_renderer/TCVegaRenderer.h>

using namespace vega_renderer::test_utils;

std::string vega_js_test_fixture::make_format_string(unsigned char *raw_format_str_ptr,
                                                    size_t raw_format_str_len) {
  auto raw_format_str = std::string(
    reinterpret_cast<char *>(raw_format_str_ptr),
    raw_format_str_len);

  return raw_format_str;
}

void vega_js_test_fixture::run_test_case_with_js(const std::string& js_test, const std::string& name) {
    @autoreleasepool {
        BOOST_TEST_MESSAGE("Running test case with name: " + name);
        TCVegaRenderer *view = [[TCVegaRenderer alloc] init];

        // analogous to BOOST_CHECK
        view.context[@"check"] = ^(BOOL value, NSString* msg) {
          if(!value && msg != nil) {
            NSLog(@"error: %@", msg);
          }
          BOOST_CHECK(value);
        };

        // analogous to BOOST_REQUIRE
        view.context[@"assert"] = ^(BOOL value, NSString* msg) {
          if(!value && msg != nil) {
            NSLog(@"error: %@", msg);
          }
          BOOST_REQUIRE(value);
        };

        [view.context evaluateScript:[NSString stringWithUTF8String:js_test.c_str()]];
    }
}
