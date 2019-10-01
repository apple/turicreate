/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <fstream>
#include <functional>
#include <string>

extern "C" {
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
}

namespace vega_renderer {
    namespace test_utils {
        struct vega_js_test_fixture {
        protected:
            std::string make_format_string(unsigned char *raw_format_str_ptr,
                                                    size_t raw_format_str_len);
            void run_test_case_with_js(const std::string& js_test, const std::string& name);
        };
    }
}