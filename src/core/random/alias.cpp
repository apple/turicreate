/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <timer/timer.hpp>
#include <core/random/random.hpp>
#include <vector>
#include <string>
#include <utility>
#include <core/random/alias.hpp>

namespace turi {
namespace random {

alias_sampler::alias_sampler(const std::vector<double>& p) {
  auto S = std::vector<size_t>();
  auto L = std::vector<size_t>();
  K = p.size();
  J = std::vector<size_t>(K);
  q = std::vector<double>(K);
  double sum_p = 0;
  for (auto pi : p) sum_p += pi;
  for (size_t i = 0; i < K; ++i) {
    q[i] = K * p[i] / sum_p;
    if (q[i] < 1) {
      S.emplace_back(i);
    } else {
      L.emplace_back(i);
    }
  }
  while (S.size() != 0 && L.size() != 0) {
    size_t s = S.back();
    S.pop_back();
    size_t l = L.back();
    /* assert(q[s] <= 1); assert(1 <= q[l]); */
    J[s] = l;
    q[l] -= (1 - q[s]);

    if (q[l] < 1) {
      S.emplace_back(l);
      L.pop_back();
    }
  }
  /** For testing purposes
    for (size_t i = 0; i < K; ++i) {
      double qi = q[i];
      for (size_t m = 0; m < K; ++m) {
        if (J[m] == i) {
          qi += 1 - q[m];
        }
      }
      assert(std::abs(qi - p[i] * K) < 0.000001);
    }
    */
}

size_t alias_sampler::sample() {
  size_t k = random::fast_uniform<size_t>(0, K - 1);
  if (q[k] > random::fast_uniform<double>(0, 1)) {
    return k;
  } else {
    return J[k];
  }
}

};
};
