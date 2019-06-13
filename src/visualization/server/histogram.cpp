/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "histogram.hpp"

#include <parallel/lambda_omp.hpp>

#include <string>
#include <cmath>

namespace turi {
namespace visualization {

std::shared_ptr<Plot> plot_histogram(
  const gl_sarray& sa, const flexible_type& xlabel, const flexible_type& ylabel,
  const flexible_type& title) {
    using namespace turi;
    using namespace turi::visualization;

    logprogress_stream << "Materializing SArray" << std::endl;
    sa.materialize();

    if (sa.size() == 0) {
      log_and_throw("Nothing to show; SArray is empty.");
    }

    std::shared_ptr<const gl_sarray> self = std::make_shared<const gl_sarray>(sa);

    std::shared_ptr<transformation_base> transformation = nullptr;
    switch (sa.dtype()) {
      case flex_type_enum::INTEGER:
      {
        std::shared_ptr<histogram<flex_int>> hist = std::make_shared<histogram<flex_int>>();
        hist->init(*self, batch_size(sa));
        transformation = hist;
        break;
      }
      case flex_type_enum::FLOAT:
      {
        std::shared_ptr<histogram<flex_float>> hist = std::make_shared<histogram<flex_float>>();
        hist->init(*self, batch_size(sa));
        transformation = hist;
        break;
      }
      default:
        DASSERT_MSG(false, "Expected integer or float SArray for histogram.");
        break;
    }

    std::string spec = histogram_spec(title, xlabel, ylabel, self->dtype());
    double size_array = static_cast<double>(self->size());
    return std::make_shared<Plot>(spec, transformation, size_array);
}

}}