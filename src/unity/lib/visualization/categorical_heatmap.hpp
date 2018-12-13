/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef __TC_CATEGORICAL_HEATMAP
#define __TC_CATEGORICAL_HEATMAP

#include <unity/lib/gl_sframe.hpp>
#include <sframe/groupby_aggregate_operators.hpp>
#include <unity/lib/visualization/plot.hpp>
#include "transformation.hpp"

namespace turi {
namespace visualization {

class categorical_heatmap_result: public transformation_output,
                                 public ::turi::groupby_operators::frequency_count {
  public:
    virtual std::string vega_column_data(bool sframe) const override;
};

// expects a gl_sframe of:
// "x": str,
// "y": float
typedef transformation<gl_sframe, categorical_heatmap_result> categorical_heatmap_parent;

class categorical_heatmap : public categorical_heatmap_parent {
  public:
    virtual std::vector<categorical_heatmap_result> split_input(size_t num_threads) override;
    virtual void merge_results(std::vector<categorical_heatmap_result>& transformers) override;
};

std::shared_ptr<Plot> plot_categorical_heatmap(
                              const gl_sarray& x,
                              const gl_sarray& y,
                              const std::string& xlabel,
                              const std::string& ylabel,
                              const std::string& title);

}} // turi::visualization

#endif // __TC_CATEGORICAL_HEATMAP
