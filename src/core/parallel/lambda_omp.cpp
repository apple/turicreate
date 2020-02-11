/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#if defined(_OPENMP) && !defined(DISABLE_OPENMP)
#include <omp.h>
#endif

#include <core/parallel/lambda_omp.hpp>
namespace turi {

void in_parallel(
    const std::function<void(size_t thread_id, size_t num_threads)>& fn) {
#pragma omp parallel
  {
    size_t nworkers = omp_get_num_threads();
#pragma omp parallel for
    for (size_t ii = 0; ii < nworkers; ii++) {
      fn(ii, nworkers);
    }
  }
}

};  // namespace turi
