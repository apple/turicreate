/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>
#include <toolkits/util/precision_recall.hpp>
#include <toolkits/util/algorithmic_utils.hpp>

namespace turi { namespace recsys {

////////////////////////////////////////////////////////////////////////////////

std::vector<std::pair<double, double> > precision_and_recall(
    std::vector<size_t> actual,
    std::vector<size_t> predicted,
    const std::vector<size_t>& cutoffs) {

  if(predicted.empty())
    return std::vector<std::pair<double, double> >(cutoffs.size(), std::make_pair(0.0, 0.0));

  // Set it up so things get mapped back to the original values.
  std::vector<std::pair<size_t, size_t> > cutoff_map(cutoffs.size());

  // Need to calculate things in order, cause we rearrange predicted
  // stuff.
  for(size_t i = 0; i < cutoffs.size(); ++i)
    cutoff_map[i] = {cutoffs[i], i};

  std::sort(cutoff_map.begin(), cutoff_map.end());

  std::vector<std::pair<double, double> > ret(cutoffs.size());

  if(!std::is_sorted(actual.begin(), actual.end()))
    std::sort(actual.begin(), actual.end());

  for(const auto& cutoff_pair : cutoff_map) {

    size_t k       = std::min(cutoff_pair.first, predicted.size());
    size_t out_idx = cutoff_pair.second;

    // TODO: optimize this, as some of the range is already sorted
    std::sort(predicted.begin(), predicted.begin() + k);

    double overlap = count_intersection(actual.begin(), actual.end(),
                                        predicted.begin(), predicted.begin() + k);

    double precision = k == 0 ? 0.0 : (overlap / k);
    double recall    = actual.empty() ? 1.0 : (overlap / actual.size());

    ret[out_idx] = std::make_pair(precision, recall);
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<double> recall(const std::vector<size_t>& actual,
                           const std::vector<size_t>& predicted,
                           const std::vector<size_t>& cutoffs) {

  std::vector<std::pair<double, double> > prv = precision_and_recall(actual, predicted, cutoffs);

  std::vector<double> ret(prv.size());

  for(size_t i = 0; i < prv.size(); ++i)
    ret[i] = prv[i].second;

  return ret;
}

std::vector<double> precision(const std::vector<size_t>& actual,
                              const std::vector<size_t>& predicted,
                              const std::vector<size_t>& cutoffs) {

  std::vector<std::pair<double, double> > prv = precision_and_recall(actual, predicted, cutoffs);

  std::vector<double> ret(prv.size());

  for(size_t i = 0; i < prv.size(); ++i)
    ret[i] = prv[i].first;

  return ret;
}

float average_precision(const std::unordered_set<flexible_type> &actual,
    const std::vector<flexible_type> &predicted,
    const int K) {

  if (actual.size() == 0)
    return 1.0;

  float score = 0.0;
  float num_hits = 0.0;

  std::unordered_set<flexible_type> seen_predictions;

  for (size_t k = 0; k < (size_t) K; ++k) {
    if ((actual.count(predicted[k]) > 0) &&
        (seen_predictions.count(predicted[k]) == 0)) {
      num_hits += 1.0;
      score += num_hits / (float) (k + 1.0);
    }
    seen_predictions.insert(predicted[k]);
  }

  int num_observations = actual.size();

  return (float) score / (float) std::min(num_observations, K);
}

float mean_average_precision(const std::vector<std::unordered_set<flexible_type>> &actual,
    const std::vector<std::vector<flexible_type>> &predicted,
    const int k) {

  ASSERT_EQ(actual.size(), predicted.size());
  ASSERT_GT(actual.size(), 0);

  float map = 0.0;
  for (size_t i = 0; i < (size_t) actual.size(); ++i) {
    map += average_precision(actual[i], predicted[i], k);
  }
  return map / (float) actual.size();
}

}
}
