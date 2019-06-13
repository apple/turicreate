/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_EVALUATION_H_
#define TURI_EVALUATION_H_

#include <string>
#include <vector>

#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
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
 * Convenience API for computing several classifier metrics simultaneously.
 *
 * This function assumes that the class labels are available and that the
 * default options for each metric suffice. It should be more efficient than
 * multiple calls to _supervised_streaming_evaluator, insofar as this function
 * computes the metrics in parallel and uses multiple threads.
 *
 * \param metrics The list of metrics to compute. Valid metrics include those
 *                supported by `get_evaluator_metric`, as well as
 *                "report_by_class".
 * \param input SFrame containing the ground-truth labels and the predicted
 *              class probabilities.
 * \param target_column_name The name of the column in `input` containing the
 *                           ground-truth labels.
 * \param prediction_probs_column_name The name of the column in `input`
 *                                     containing the predicted class
 *                                     probabilities.
 * \param class_labels The class labels used when training the model being
 *                     evaluated. Every prediction probability vector must have
 *                     the same length as this list.
 * \return A map from metric name to the output from the corresponding
 *         evaluation metric.
 *
 * \todo Factor out the logic shared with
 *       `supervised_learning_model_base::evaluate`. Compared to that version,
 *       this one requires that all the predictions have been written to an
 *       SArray.
 */
variant_map_type compute_classifier_metrics_from_probability_vectors(
    std::vector<std::string> metrics, gl_sframe input,
    std::string target_column_name, std::string prediction_probs_column_name,
    flex_list class_labels);

variant_map_type compute_classifier_metrics(
    gl_sframe data, std::string target_column_name, std::string metric,
    gl_sarray predictions, std::map<std::string, flexible_type> opts);

variant_map_type compute_object_detection_metrics(
    gl_sframe data, std::string annotations_column_name,
    std::string image_column_name, gl_sarray predictions,
    std::map<std::string, flexible_type> opts);

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
