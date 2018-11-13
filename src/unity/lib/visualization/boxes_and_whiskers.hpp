/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_BOXES_AND_WHISKERS
#define __TC_BOXES_AND_WHISKERS

#include <unity/lib/gl_sframe.hpp>
#include <sframe/groupby_aggregate_operators.hpp>
#include <unity/lib/visualization/plot.hpp>
#include "groupby.hpp"
#include "transformation.hpp"

namespace turi {
namespace visualization {

class boxes_and_whiskers_result: public transformation_output,
                                 public groupby_quantile_result {
  public:
    virtual std::string vega_column_data(bool sframe) const override;
};

// expects a gl_sframe of:
// "x": str,
// "y": int/float
class boxes_and_whiskers : public groupby<boxes_and_whiskers_result> {
};

// expects x to be str, y to be int/float
std::shared_ptr<Plot> plot_boxes_and_whiskers(
                              const gl_sarray& x,
                              const gl_sarray& y,
                              const flexible_type& xlabel,
                              const flexible_type& ylabel,
                              const flexible_type& title);

}} // turi::visualization

#endif // __TC_BOXES_AND_WHISKERS
