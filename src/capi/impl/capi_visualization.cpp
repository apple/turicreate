/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <capi/impl/capi_error_handling.hpp>
#include <capi/impl/capi_initialization_internal.hpp>
#include <flexible_type/flexible_type.hpp> 
#include <export.hpp>

extern "C" {

    tc_plot* tc_plot_create_1d(const tc_sarray* sa,
                            const char *title,
                            const char *x_axis_title,
                            const char *y_axis_title,
                            tc_error** error) {

        ERROR_HANDLE_START();
        turi::ensure_server_initialized();

        CHECK_NOT_NULL(error, sa, "sarray", NULL);

        std::shared_ptr<turi::model_base> plot = sa->value.plot("" /* path_to_client */, title, x_axis_title, y_axis_title);
        return new_tc_plot(std::dynamic_pointer_cast<turi::visualization::Plot>(plot));

        ERROR_HANDLE_END(error, NULL);
    }

    tc_flexible_type* tc_plot_get_vega_spec(const tc_plot* plot, tc_error** error) {
        ERROR_HANDLE_START();
        turi::ensure_server_initialized();

        CHECK_NOT_NULL(error, plot, "plot", NULL);

        std::string vega_spec = plot->value->get_spec();
        return tc_ft_create_from_string(vega_spec.data(), vega_spec.size(), error);

        ERROR_HANDLE_END(error, NULL);
    }

    tc_flexible_type* tc_plot_get_next_data(const tc_plot* plot, tc_error** error) {
        ERROR_HANDLE_START();
        turi::ensure_server_initialized();

        CHECK_NOT_NULL(error, plot, "plot", NULL);

        std::string vega_data = plot->value->get_data();
        return tc_ft_create_from_string(vega_data.data(), vega_data.size(), error);

        ERROR_HANDLE_END(error, NULL);
    }
}