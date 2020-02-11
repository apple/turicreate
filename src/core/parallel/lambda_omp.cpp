#include <omp.h>

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
