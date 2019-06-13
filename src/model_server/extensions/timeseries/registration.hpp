/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TIMESERIES_REGISTRATIONS_H_
#define TIMESERIES_REGISTRATIONS_H_

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_class_specification.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>

#include <model_server/extensions/timeseries/grouped_timeseries.hpp>
#include <model_server/extensions/timeseries/timeseries.hpp>

namespace turi {
namespace timeseries {

std::vector<turi::toolkit_class_specification>
                          get_toolkit_class_registration();
std::vector<turi::toolkit_function_specification>
                          get_toolkit_function_registration();

}
}
#endif
