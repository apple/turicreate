/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef __TC_VIS_HEATMAP
#define __TC_VIS_HEATMAP

#include <unity/lib/gl_sframe.hpp>

#include "extrema.hpp"
#include "groupby.hpp"
#include <unity/lib/visualization/plot.hpp>

namespace turi {
namespace visualization {

  class heatmap_result: public transformation_output,
                        public group_aggregate_value {
    private:
      std::vector<std::vector<flex_int>> bins; // row-major order

      // heatmap methods
      void widen_x(double value);
      void widen_y(double value);

    public:
      bounding_box<double> extrema;

      heatmap_result();
      void init(double xMin, double xMax, double yMin, double yMax);

      // group_aggregate_value methods
      virtual group_aggregate_value* new_instance() const override;
      virtual void add_element_simple(const flexible_type& flex) override;
      virtual void combine(const group_aggregate_value& other) override;
      virtual flexible_type emit() const override;
      virtual bool support_type(flex_type_enum type) const override;
      virtual std::string name() const override;
      virtual void save(oarchive& oarc) const override;
      virtual void load(iarchive& iarc) override;

      // transformation_output methods
      virtual std::string vega_column_data(bool) const override;
  };

  /*
   * heatmap()
   * Uses an optimal streaming histogram in 2-d to avoid
   * the need for restarting when re-binning. Bins into a potentially wider
   * range than the data (not to exceed 2x the range), and re-bins into the
   * data range on get().
   *
   * dtype of sarrays can be flex_int or flex_float
   * Heatmap always gives bins as flex_ints (bin counts are positive integers).
   */
  class heatmap : public groupby<heatmap_result> {
    public:
      virtual void init(const gl_sframe& source) override;
      virtual std::vector<heatmap_result> split_input(size_t num_threads) override;
  };

  std::shared_ptr<Plot> plot_heatmap(const std::string& path_to_client,
                    const gl_sarray& x,
                    const gl_sarray& y,
                    const std::string& xlabel,
                    const std::string& ylabel,
                    const std::string& title);

}}

#endif
