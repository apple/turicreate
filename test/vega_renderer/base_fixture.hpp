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
        struct base_fixture {
        private:
            static void replace_url_with_values_in_data_entry(CFMutableDictionaryRef dataEntry);
            static std::string replace_urls_with_values_in_spec(const std::string& spec);
            static CGContextRef create_cgcontext(double width, double height);
            static double count_all_pixels(CGImageRef image);
            double compare_expected_with_actual(CGImageRef expected,
                                                CGImageRef actual,
                                                double acceptable_diff);
            void expected_rendering(const std::string& spec,
                                    std::function<void(CGImageRef image)> completion_handler);

        protected:
            static double acceptable_diff;
            std::string make_format_string(unsigned char *raw_format_str_ptr,
                                                    size_t raw_format_str_len);
            void run_test_case_with_spec(const std::string& test_spec, const std::string& name);
            void run_test_case_with_spec(const std::string& test_spec, const std::string& name,  double acceptable_diff);
        };
    }
}