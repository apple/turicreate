/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_COLUMN_SUMMARY
#define __TC_COLUMN_SUMMARY

#include <unity/lib/visualization/process_wrapper.hpp>
#include <unity/lib/visualization/histogram.hpp>
#include <unity/lib/visualization/item_frequency.hpp>
#include <unity/lib/visualization/transformation.hpp>
#include <unity/lib/visualization/thread.hpp>
#include <unity/lib/visualization/summary_view.hpp>
#include <unity/lib/visualization/vega_data.hpp>
#include <unity/lib/visualization/vega_spec.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/gl_sframe.hpp>

namespace turi {
  namespace visualization {
    std::shared_ptr<Plot> plot_columnwise_summary(
      const std::string& path_to_client, std::shared_ptr<unity_sframe_base> sf);
  }
}

#endif // __TC_ITEM_FREQUENCY
