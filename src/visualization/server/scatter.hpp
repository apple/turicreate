/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/sframe/gl_sarray.hpp>

#include "groupby.hpp"
#include "transformation.hpp"
#include "plot.hpp"

namespace turi {
namespace visualization {

  class scatter_result: public transformation_output {
    private:
      gl_sframe m_sf;

    public:
      scatter_result(gl_sframe sf);
      virtual std::string vega_column_data(bool) const override;
  };

  class scatter: public transformation_base {
    private:
      gl_sframe m_sf;

    public:
      void init(gl_sframe sf);
      virtual std::shared_ptr<transformation_output> get() override;
      virtual bool eof() const override;
      virtual size_t get_batch_size() const override;
      virtual flex_int get_rows_processed() const override;
      virtual flex_int get_total_rows() const override;
  };

  std::shared_ptr<Plot> plot_scatter(
                    const gl_sarray& x,
                    const gl_sarray& y,
                    const std::string& xlabel,
                    const std::string& ylabel,
                    const std::string& title);

}}
