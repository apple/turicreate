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
#if defined(_OPENMP) && !defined(DISABLE_OPENMP)
// start a team of threads
#pragma omp parallel
  {
    const size_t nworkers = omp_get_num_threads();
#pragma omp for schedule(guided)
    for (size_t ii = 0; ii < nworkers; ii++) {
      fn(ii, nworkers);
    }
  }

#else

  size_t nworkers = thread_pool::get_instance().size();

  if (thread::get_tls_data().is_in_thread() || nworkers <= 1) {
    fn(0, 1);
    return;

  } else {
    parallel_task_queue threads(thread_pool::get_instance());

    for (unsigned int i = 0; i < nworkers; ++i) {
      threads.launch([&fn, i, nworkers]() { fn(i, nworkers); }, i);
    }
    threads.join();
  }
#endif
}

};  // namespace turi
