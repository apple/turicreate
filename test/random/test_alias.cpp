/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <timer/timer.hpp>
#include <core/random/random.hpp>
#include <core/random/alias.hpp>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <iomanip>

using namespace turi;

/**
 * Create a Small PMF by hand.
 */
std::vector<double> create_small_pmf() {
  return {0.05, 0.01, 0.03, 0.01, 0.05, 0.1, 0.07, 0.03, 0.04, 0.01, 
    0.08, 0.02, 0.1, 0.1, 0.2, 0.1};
}

/**
 * Create a PMF with K outcomes in its sample space.
 * The probability of each outcome is generated from a random uniform.
 */
std::vector<double> create_large_pmf(size_t K) {
  auto probs = std::vector<double>(K);
  double total = 0;
  for (size_t k = 0; k < probs.size(); ++k) {
    probs[k] = random::fast_uniform<double>(0, 100);
    total += probs[k];
  }
  for (size_t k = 0; k < probs.size(); ++k) {
    probs[k] = probs[k]/total;
  }
  return probs;
}

/**
 * Run a benchmark on a given pmf that draws num_samples using the 
 * alias method as well as random::multinomial. For comparison (and
 * a lower bound) the timing for just a uniformly distributed int
 * is included.
 *
 * \param num_samples The number of samples to draw.
 * \param probs an (possibly unnormalized) vector of nonzero values
 *              to use as a pmf.
 */
void run_alias_benchmark(size_t num_samples, std::vector<double> probs) {

  timer ti;
  ti.start();
  auto A = random::alias_sampler(probs);
  std::cout << std::setw(20) << "alias setup time: " 
            << ti.current_time() << std::endl;
  size_t k = 0;
  ti.start();
  for (size_t i=0; i < num_samples; ++i) {
    k += A.sample();
  }
  std::cout << std::setw(20) << "alias " 
            << ti.current_time() << std::endl;

  ti.start();
  for (size_t i=0; i < num_samples; ++i) {
    k += random::multinomial(probs);
  }
  std::cout << std::setw(20) << "multinomial " 
            << ti.current_time() << std::endl;

  size_t K = probs.size();
  ti.start();
  for (size_t i=0; i < num_samples; ++i) {
    k += random::fast_uniform<size_t>(0, K);
  }
  std::cout << std::setw(20) << "fast unif " 
            << ti.current_time() << std::endl;

}

std::vector<size_t> count_samples(size_t num_samples, std::vector<double> probs) {

  auto A = random::alias_sampler(probs);
  auto counts = std::vector<size_t>(probs.size());
  for (size_t i=0; i < num_samples; ++i) {
    ++counts[A.sample()];
  }
  return counts;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "format: " << argv[0] << " <num_samples>" 
              << std::endl;
    exit(1);
  }
  int N = atoi(argv[1]);

  turi::random::seed(1001);

  std::cout << "Performance on a small pmf:" << std::endl;
  auto p1 = create_small_pmf();
  run_alias_benchmark(N, p1);

  std::cout << "Performance on a large pmf with 1000 levels:" << std::endl;
  auto p2 = create_large_pmf(1000); 
  run_alias_benchmark(N, p2);

  std::cout << "Compare observed frequencies (left) with true probabilities"
            << std::endl;
  size_t num_samples = 100000;
  auto probs = create_small_pmf();
  auto counts = count_samples(num_samples, probs); 
  for (size_t k = 0; k < counts.size(); ++k) {
    std::cout << counts[k] / (double) num_samples << " " 
              << probs[k] 
              << std::endl;
  }
  return 0;
}


