/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_RECOMMENDER_EVALUATOR
#define TURI_UNITY_RECOMMENDER_EVALUATOR
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi { namespace recsys {

/**
 *
 * \ingroup toolkit_util
 *
 * Compute precision and recall at k.  This is faster than calculating
 * precision and recall seperately.  In information retrieval terms,
 * this represents the ratio of relevant, retrieved items to the
 * number of relevant items.
 *
 * Let \f$p_k\f$ be a vector of the first \f$k\f$ elements of the argument "predicted",
 * and let \f$a\f$ be the set of items in the "actual" argument. The "recall at K" is defined as
 *
 * \f[
 * P(k) = \frac{ | a \cap p_k | }{|a|}
 * \f]
 *
 * The order of the elements in predicted affects the returned score.
 * Only unique predicted values contribute to the score.
 * One of the provided vectors must be nonempty.
 * If actual is empty, return 1.0. If predicted is empty, returns 0.0.
 *
 * \param actual A vector of observed items
 * \param predicted A vector of predicted items
 * \param cutoffs A vector of positive integers for which recall should be calculated
 *
 * return A vector of pair(precision, recall) scores corresponding to
 * the values in cutoffs.
 *
 * Notes:
 * The corner cases that involve empty lists were chosen to be consistent with the feasible
 * set of precision-recall curves, which start at (precision, recall) = (1,0) and end at (0,1).
 * However, we do not believe there is a well-known concensus on this choice.
 */
std::vector<std::pair<double, double> > precision_and_recall(
    std::vector<size_t> actual,
    std::vector<size_t> predicted,
    const std::vector<size_t>& cutoffs);


/**
 *
 * \ingroup toolkit_util
 *
 * Compute recall at k.
 * In information retrieval terms, this represents the ratio of relevant, retrieved items
 * to the number of relevant items.
 *
 * Let \f$p_k\f$ be a vector of the first \f$k\f$ elements of the argument "predicted",
 * and let \f$a\f$ be the set of items in the "actual" argument. The "recall at K" is defined as
 *
 * \f[
 * P(k) = \frac{ | a \cap p_k | }{|a|}
 * \f]
 *
 * The order of the elements in predicted affects the returned score.
 * Only unique predicted values contribute to the score.
 * One of the provided vectors must be nonempty.
 * If actual is empty, return 1.0. If predicted is empty, returns 0.0.
 *
 * \param actual an unordered vector of observed items
 * \param predicted an vector of predicted items
 * \param cutoffs A vector of positive integers for which recall should be calculated
 *
 * return A vector of recall scores corresponding to the values in cutoffs
 *
 * Notes:
 * The corner cases that involve empty lists were chosen to be consistent with the feasible
 * set of precision-recall curves, which start at (precision, recall) = (1,0) and end at (0,1).
 * However, we do not believe there is a well-known concensus on this choice.
 */
std::vector<double> recall(const std::vector<size_t>& actual,
    const std::vector<size_t>& predicted,
    const std::vector<size_t>& cutoffs);

/**
 * \ingroup toolkit_util
 * Compute precision at k.
 * In information retrieval terms, this represents the ratio of relevant, retrieved items
 * to the number of retrieved items.
 *
 * Let \f$p_k\f$ be a vector of the first \f$k\f$ elements of the argument "predicted",
 * and let \f$a\f$ be the set of items in the "actual" argument. The "precision at K" is defined as
 *
 * \f[
 * P(k) = \frac{ | a \cap p_k | }{|p_k|}
 * \f]
 *
 * The order of the elements in predicted affects the returned score.
 * Only unique predicted values contribute to the score.
 * One of the provided vectors must be nonempty.
 * If actual is empty, return 0.0. If predicted is empty, returns 1.0.
 *
 * \param actual an unordered vector observed items
 * \param predicted an vector of predicted items
 * \param cutoffs A vector of positive integers for which recall should be calculated
 *
 * return A vector of precision scores corresponding to the values in cutoffs
 *
 * Notes:
 * The corner cases that involve empty lists were chosen to be consistent with the feasible
 * set of precision-recall curves, which start at (precision, recall) = (1,0) and end at (0,1).
 * However, we do not believe there is a well-known concensus on this choice.
 */
/** Other versions of the above code
 */

std::vector<double> precision(const std::vector<size_t>& actual,
    const std::vector<size_t>& predicted,
    const std::vector<size_t>& cutoffs);

/**
 * \ingroup toolkit_util
 *
 * Compute the average precision at k.
 * This combines precision values at values up to k, where lower ranks are less important.
 *
 * Let \f$p_k\f$ be a vector of the first \f$k\f$ elements of the argument "predicted",
 * and let \f$a\f$ be the set of items in the "actual" argument.
 * If \f$P(k)\f$ is the precision at \f$k\f$, then the average precision at \f$k\f$ is defined as
 *
 * \f[
 * AP(k) = \frac{1}{\min(k, |a|)}\sum_{k: p_k \in a} \frac{P(k)}{k}
 * \f]
 *
 * \param actual an unordered set of observed items
 * \param predicted an vector of predicted items
 * \param k the maximum number of predicted elements
 *
 * \return The average precision at k for the provided lists.
 */

float average_precision(const std::unordered_set<flexible_type> &actual,
    const std::vector<flexible_type> &predicted,
    const int k);

/**
 * \ingroup toolkit_util
 *
 * Compute mean average precision across all of the elements of the provided vectors.
 * The two vectors must have the same length.
 *
 * actual: a vector of unordered sets of observed items.
 * predicted: a vector of vectors of observed items.
 *
 */

float mean_average_precision(const std::vector<std::unordered_set<flexible_type>> &actual,
    const std::vector<std::vector<flexible_type>> &predicted,
    const int k);

}
}
#endif
