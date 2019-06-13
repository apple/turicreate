/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_VISUALIZATION_BATCH_SIZE_HPP
#define TURI_VISUALIZATION_BATCH_SIZE_HPP

#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>

namespace turi {
namespace visualization {

  // Computes batch sizes for a variety of input.
  // For now, very basic implementations (constant w.r.t. number of columns),
  // But could potentially use the types of columns or even the data itself.
  size_t batch_size(const gl_sarray& x);
  size_t batch_size(const gl_sarray& x, const gl_sarray& y);
  size_t batch_size(const gl_sframe& sf);

}
}

#endif
