/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <timer/timer.hpp>
#include <core/util/fast_integer_power.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/random/random.hpp>

using namespace turi;

static constexpr size_t n_iterations = 10000000;

void _run_time_test(size_t max_value) GL_HOT_INLINE_FLATTEN;

void _run_time_test(size_t max_value) {

  double v = std::pow(0.000001, 1.0 / max_value);

  std::vector<size_t> powers(n_iterations);

  for(size_t i = 0; i < n_iterations; ++i)
    powers[i] = random::fast_uniform<size_t>(0, max_value);


  {
    timer tt;
    tt.start();

    double x = 0;
    for(size_t i = 0; i < n_iterations; ++i) {
      x += std::pow(v, powers[i]);
    }

    std::cout << "  Time with std power function (" << n_iterations << " iterations, x = " << x << "): "
              << tt.current_time() << 's' << std::endl;
  }

  {
    timer tt;
    tt.start();

    fast_integer_power fip(v);

    double x = 0;
    size_t n_times = 20;
    for(size_t iter = 0; iter < n_times; ++iter) {
      for(size_t i = 0; i < n_iterations; ++i) {
        x += fip.pow(powers[i]);
      }
    }

    std::cout << "  Time with new power function (" << n_iterations << " iterations, x = " << x / n_times << "): "
              << tt.current_time() / n_times << 's' << std::endl;
  }

}

int main(int argc, char **argv) {

  std::cout << "Small integers (0 - 65535): " << std::endl;
  _run_time_test( (1UL << 16) );

  std::cout << "Medium integers (0 - 2^32): " << std::endl;

  _run_time_test( (1UL << 32) );

  std::cout << "Large integers (0 - 2^48): " << std::endl;

  _run_time_test( (1UL << 48) );
}
