/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "transformation.hpp"

namespace turi {
namespace visualization {

double transformation_base::get_percent_complete() const {
  double ret = static_cast<double>(get_rows_processed()) /
                static_cast<double>(get_total_rows());
  DASSERT_GE(ret, 0.0);
  DASSERT_LE(ret, 1.0);
  return ret;
}

}} // turi::visualization
