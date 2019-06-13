/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>

#include <model_server/extensions/timeseries/timeseries.hpp>
#include <model_server/extensions/timeseries/grouped_timeseries.hpp>

using namespace turi;

// NOTE: Do not change how the export code is setup here. You must register the
// classes in the turi namespace otherwise the export code is not called.
BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(turi::timeseries::gl_timeseries)
REGISTER_CLASS(turi::timeseries::gl_grouped_timeseries)
END_CLASS_REGISTRATION

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(turi::timeseries::date_range, "start_time", "end_time",
    "period")
END_FUNCTION_REGISTRATION
