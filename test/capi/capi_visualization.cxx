/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

#include <capi/TuriCreate.h>
#include "capi_utils.hpp"
#include <visualization/server/plot.hpp>
#include <visualization/server/show.hpp>

#include <random>

using namespace turi;
using namespace turi::visualization;

/*
 * Test streaming visualization C API
 * High level goal: ensure the same output between the C API
 *                  and the equivalent C++ Plot object method(s).
 * Non-goal (for this test): ensure that the resulting values are correct.
 * C++ unit tests should cover the correctness of the results.
 */
class capi_test_visualization {
    private:
        tc_sarray* m_sa_int;
        tc_sarray* m_sa_float;
        tc_sarray* m_sa_float_10k;
        tc_sarray* m_sa_str;
        tc_sframe* m_sf_float;

    public:
        capi_test_visualization() {
            tc_error* error = NULL;
            tc_flex_list* fl = NULL;

            std::vector<int64_t> v = {0,1,2,3,4,5};
            fl = make_flex_list_int(v);
            m_sa_int = tc_sarray_create_from_list(fl, &error);
            CAPI_CHECK_ERROR(error);

            error = NULL;
            std::vector<double> v2 = {0.0, 0.8, -1.0, -0.4, 1.0, 0.2};
            fl = make_flex_list_double(v2);
            m_sa_float = tc_sarray_create_from_list(fl, &error);
            CAPI_CHECK_ERROR(error);

            error = NULL;
            std::random_device rd{};
            std::mt19937 gen{rd()};
            std::normal_distribution<> d{0,10};
            std::vector<double> float_10k(10000);
            for (size_t i=0; i<10000; i++) {
                float_10k[i] = d(gen);
            }
            fl = make_flex_list_double(float_10k);
            m_sa_float_10k = tc_sarray_create_from_list(fl, &error);
            CAPI_CHECK_ERROR(error);

            error = NULL;
            std::vector<std::string> v3 = {"foo", "bar", "baz", "qux", "baz", "baz"};
            fl = make_flex_list_string(v3);
            m_sa_str = tc_sarray_create_from_list(fl, &error);
            CAPI_CHECK_ERROR(error);

            std::vector<std::pair<std::string, std::vector<double>>> sf_data;
            sf_data.push_back(std::make_pair<std::string, std::vector<double>>("x", {0.0, 1.0, 2.0, 3.5, 12.7}));
            sf_data.push_back(std::make_pair<std::string, std::vector<double>>("y", {3.2, -9.7, 2.1, 3.8, 2.2}));
            m_sf_float = make_sframe_double(sf_data);
        }

        ~capi_test_visualization() {
            tc_release(m_sa_int);
            tc_release(m_sa_float);
            tc_release(m_sa_float_10k);
            tc_release(m_sa_str);
            tc_release(m_sf_float);
        }

        void test_1d_plots() {

            // numeric histogram (int)
            std::shared_ptr<model_base> expected_obj_base = m_sa_int->value.plot("foo", "bar", "baz");
            std::shared_ptr<Plot> expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            std::string expected_spec = expected_obj->get_spec();
            tc_error *error = nullptr;
            tc_plot *actual_obj = tc_plot_create_1d(m_sa_int, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            tc_flexible_type *actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            const char *actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            size_t actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            std::string actual_spec(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);

            // numeric histogram (float)
            expected_obj_base = m_sa_float->value.plot(FLEX_UNDEFINED, FLEX_UNDEFINED, FLEX_UNDEFINED);
            expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            expected_spec = expected_obj->get_spec();
            error = nullptr;
            actual_obj = tc_plot_create_1d(m_sa_float, nullptr, nullptr, nullptr, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec = std::string(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);

            // categorical histogram (str)
            expected_obj_base = m_sa_str->value.plot("foo", "bar", "baz");
            expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            expected_spec = expected_obj->get_spec();
            error = nullptr;
            actual_obj = tc_plot_create_1d(m_sa_str, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec = std::string(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);
        }

        void test_2d_plots() {
            // numeric x numeric small (scatter plot)
            std::shared_ptr<model_base> expected_obj_base = plot(m_sa_int->value, m_sa_float->value, "bar", "baz", "foo");
            std::shared_ptr<Plot> expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            std::string expected_spec = expected_obj->get_spec();
            tc_error *error = nullptr;
            tc_plot *actual_obj = tc_plot_create_2d(m_sa_int, m_sa_float, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            tc_flexible_type *actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            const char *actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            size_t actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            std::string actual_spec(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);

            // numeric x numeric large (continuous heat map)
            expected_obj_base = plot(m_sa_float_10k->value, m_sa_float_10k->value, "bar", "baz", "foo");
            expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            expected_spec = expected_obj->get_spec();
            error = nullptr;
            actual_obj = tc_plot_create_2d(m_sa_float_10k, m_sa_float_10k, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec = std::string(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);

            // numeric x categorical (boxes and whiskers)
            expected_obj_base = plot(m_sa_float->value, m_sa_str->value, "bar", "baz", "foo");
            expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            expected_spec = expected_obj->get_spec();
            error = nullptr;
            actual_obj = tc_plot_create_2d(m_sa_float, m_sa_str, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec = std::string(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);

            // categorical x categorical (discrete heat map)
            expected_obj_base = plot(m_sa_str->value, m_sa_str->value, "bar", "baz", "foo");
            expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            expected_spec = expected_obj->get_spec();
            error = nullptr;
            actual_obj = tc_plot_create_2d(m_sa_str, m_sa_str, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            actual_spec = std::string(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);
        }

        void test_sframe_summary_plot() {
            std::shared_ptr<model_base> expected_obj_base = m_sf_float->value.plot();
            std::shared_ptr<Plot> expected_obj = std::dynamic_pointer_cast<Plot>(expected_obj_base);
            std::string expected_spec = expected_obj->get_spec();
            tc_error *error = nullptr;
            tc_plot *actual_obj = tc_plot_create_sframe_summary(m_sf_float, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            tc_flexible_type *actual_spec_ft = tc_plot_get_vega_spec(actual_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            const char *actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            size_t actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            std::string actual_spec(actual_spec_data, actual_spec_length);
            TS_ASSERT_EQUALS(actual_spec, expected_spec);
            while (!expected_obj->finished_streaming()) {
                TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), false);
                CAPI_CHECK_ERROR(error);
                std::string expected_data = expected_obj->get_next_data();
                tc_flexible_type *actual_data_ft = tc_plot_get_next_data(actual_obj, nullptr, &error);
                CAPI_CHECK_ERROR(error);
                const char *actual_data_data = tc_ft_string_data(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                size_t actual_data_length = tc_ft_string_length(actual_data_ft, &error);
                CAPI_CHECK_ERROR(error);
                std::string actual_data(actual_data_data, actual_data_length);
                TS_ASSERT_EQUALS(actual_data, expected_data);
            }
            TS_ASSERT_EQUALS(tc_plot_finished_streaming(actual_obj, nullptr, &error), true);
            CAPI_CHECK_ERROR(error);
        }

        void test_plot_get_url() {
            // For a given plot, test that get_url returns the same value through C and C++ API
            tc_error *error = nullptr;
            tc_plot *actual_obj = tc_plot_create_1d(m_sa_int, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            std::shared_ptr<Plot> expected_obj = std::dynamic_pointer_cast<Plot>(actual_obj->value);
            std::string expected_url = expected_obj->get_url();
            tc_flexible_type *actual_url_ft = tc_plot_get_url(actual_obj, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            const char *actual_url_data = tc_ft_string_data(actual_url_ft, &error);
            CAPI_CHECK_ERROR(error);
            size_t actual_url_length = tc_ft_string_length(actual_url_ft, &error);
            CAPI_CHECK_ERROR(error);
            std::string actual_url(actual_url_data, actual_url_length);
            TS_ASSERT_EQUALS(actual_url, expected_url);

            // TODO - test web server page load
        }

#ifdef __APPLE__
#ifndef TC_BUILD_IOS

        static CGContextRef create_cgcontext(double width, double height) {
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef ctx = CGBitmapContextCreate(NULL, // Let CG allocate it for us
                                                    width,
                                                    height,
                                                    8,
                                                    0,
                                                    colorSpace,
                                                    kCGImageAlphaNoneSkipLast); // RGBA
            // draw a white background
            CGContextSaveGState(ctx);
            CGContextFillRect(ctx, CGRectMake(0, 0, width, height));
            CGContextRestoreGState(ctx);
            CGColorSpaceRelease(colorSpace);
            return ctx;
        }

        void test_rendering() {
            // numeric histogram (int)
            tc_error *error = nullptr;
            tc_plot *plot_obj = tc_plot_create_1d(m_sa_int, "foo", "bar", "baz", nullptr, &error);
            CAPI_CHECK_ERROR(error);
            tc_flexible_type *actual_spec_ft = tc_plot_get_vega_spec(plot_obj, tc_plot_variation_default, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            const char *actual_spec_data = tc_ft_string_data(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            size_t actual_spec_length = tc_ft_string_length(actual_spec_ft, &error);
            CAPI_CHECK_ERROR(error);
            std::string actual_spec(actual_spec_data, actual_spec_length);

            CGContextRef ctx = create_cgcontext(800, 600); // some random size - should be larger than the plot

            // render plot onto it and check for errors
            tc_plot_render_final_into_context(plot_obj, tc_plot_variation_default, ctx, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            CGContextRelease(ctx);

            // render spec onto it and check for errors
            ctx = create_cgcontext(800, 600); // some random size - should be larger than the plot
            tc_plot_render_vega_spec_into_context(actual_spec.c_str(), ctx, nullptr, &error);
            CAPI_CHECK_ERROR(error);
            CGContextRelease(ctx);
        }
#endif // TC_BUILD_IOS
#endif // __APPLE__

};

BOOST_FIXTURE_TEST_SUITE(_capi_test_visualization, capi_test_visualization)
BOOST_AUTO_TEST_CASE(test_1d_plots) {
  capi_test_visualization::test_1d_plots();
}
BOOST_AUTO_TEST_CASE(test_2d_plots) {
  capi_test_visualization::test_2d_plots();
}
BOOST_AUTO_TEST_CASE(test_sframe_summary_plot) {
  capi_test_visualization::test_sframe_summary_plot();
}
BOOST_AUTO_TEST_CASE(test_plot_get_url) {
  capi_test_visualization::test_plot_get_url();
}
#ifdef __APPLE__
#ifndef TC_BUILD_IOS
BOOST_AUTO_TEST_CASE(test_rendering) {
  capi_test_visualization::test_rendering();
}
#endif // TC_BUILD_IOS
#endif // __APPLE__
BOOST_AUTO_TEST_SUITE_END()
