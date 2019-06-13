/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_COLUMN_SUMMARY
#define __TC_COLUMN_SUMMARY

#include <visualization/server/process_wrapper.hpp>
#include <visualization/server/histogram.hpp>
#include <visualization/server/item_frequency.hpp>
#include <visualization/server/transformation.hpp>
#include <visualization/server/thread.hpp>
#include <visualization/server/summary_view.hpp>
#include <visualization/server/vega_data.hpp>
#include <visualization/server/vega_spec.hpp>

namespace turi {
  class unity_sframe_base;

  namespace visualization {
    std::shared_ptr<Plot> plot_columnwise_summary(std::shared_ptr<unity_sframe_base> sf);
  }
}

#endif // __TC_ITEM_FREQUENCY
