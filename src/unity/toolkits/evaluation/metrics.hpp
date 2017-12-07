/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_EVALUATION_H_
#define TURI_EVALUATION_H_

#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/toolkits/evaluation/evaluation_constants.hpp>
#include <unity/toolkits/evaluation/evaluation_interface-inl.hpp>

namespace turi {
namespace evaluation {



/**
 * Evaluation using the streaming evaluation interface.
 *
 * \param[in] targets       True values
 * \param[in] prediction    Predicted values
 * \param[in] metric        Name of the metric
 * See unity/toolkit/evaluation/metrics.hpp.
 */
variant_type _supervised_streaming_evaluator(
                           std::shared_ptr<unity_sarray> unity_targets,
                           std::shared_ptr<unity_sarray> unity_predictions,
                           std::string metric,
                           std::map<std::string, flexible_type> kwargs =
                                  std::map<std::string, flexible_type>());

/**
 * Computes the precision and recall for each user.
 *
 * \param validation_data An sframe object containing a user column
 * and an item column.
 * \param recommend_output An sframe representing a set of
 * recommendations. The first column must contain user ids, the second must
 * contain item ids. For each user, the item ids are expected to be sorted
 * by importance. (Precision and recall values are sensitive to this ordering.
 * \param cutoffs A set of cutoffs at which precision and recall should be
 * computed.
 * \return An SFrame containing columns for the user, the cutoff, and
 * precision and recall values.
 */
sframe precision_recall_by_user(
    const sframe& validation_data,
    const sframe& recommend_output,
    const std::vector<size_t>& cutoffs);

} // evaluation
} // turicreate
#endif
