/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include "batch_size.hpp"

namespace turi {
namespace visualization {

size_t batch_size(const gl_sarray& x) {
  return 5000000;
}

size_t batch_size(const gl_sarray& x, const gl_sarray& y) {
  return 2500000;
}

size_t batch_size(const gl_sframe& sf) {
  return 5000000 / sf.column_names().size();
}

}} // turi::visualization
