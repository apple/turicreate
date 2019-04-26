/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/toolkits/util/precision_recall.hpp>

// ML-Data
#include <ml_data/column_indexer.hpp>

// Evaluation
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/evaluation/evaluation_constants.hpp>
#include <unity/toolkits/evaluation/metrics.hpp>
#include <unity/toolkits/object_detection/od_data_iterator.hpp>
#include <unity/toolkits/object_detection/od_evaluation.hpp>
#include <unity/toolkits/supervised_learning/classifier_evaluations.hpp>

#include <map>
#include <algorithm>


namespace turi {
namespace evaluation {

namespace {

std::vector<std::string> get_default_classifier_metrics() {
  return {"accuracy", "auc", "precision", "recall", "f1_score", "log_loss",
          "confusion_matrix", "roc_curve"};
}

std::vector<std::string> get_classifier_metrics(const std::string& metric) {
  // Validate metric and determine list of metrics to compute.
  std::vector<std::string> metrics = get_default_classifier_metrics();
  if (metric != "auto") {
    // If the caller didn't request "auto", then adjust the list of metrics.
    if (metric == "report") {
      // Add the per-class report to the standard list of metrics.
      metrics.push_back("report_by_class");
    } else {
      // Just compute the requested metric, if valid.
      if (std::find(metrics.begin(), metrics.end(), metric) == metrics.end()) {
        log_and_throw("Unsupported metric " + metric);
      } else {
        metrics = {metric};
      }
    }
  }
  return metrics;
}

gl_sarray get_prediction_probability_vectors(const gl_sarray& predictions,
                                             const flex_list& class_labels) {
  // Canonicalize predictions into flex_vec.
  gl_sarray result;
  switch (predictions.dtype()) {
  case flex_type_enum::VECTOR:
    result = predictions;
    break;
  case flex_type_enum::ND_VECTOR:
    result = predictions.astype(flex_type_enum::VECTOR);
    break;
  case flex_type_enum::DICT: {
    // Require/assume the dictionary to map class labels to probabilities.

    // Build a fast lookup table from label to flex_vec index.
    auto class_to_index =
        std::make_shared<std::unordered_map<flexible_type, size_t>>();
    for (size_t i = 0; i < class_labels.size(); ++i) {
      class_to_index->emplace(class_labels[i], i);
    }

    // Use the lookup table to write a flex_vec for each flex_dict.
    auto flatten = [class_to_index](const flexible_type& ft) {
      const flex_dict& dict = ft.get<flex_dict>();
      flex_vec vec(dict.size());
      for (const auto& kv : dict) {
        vec[class_to_index->at(kv.first)] = kv.second;
      }
      return vec;
    };
    result = predictions.apply(flatten, flex_type_enum::VECTOR);
    break;
  }
  default:
    log_and_throw("Could not convert predictions to probability vectors for "
                  "classifier evaluation");
  }
  return result;
}

}

/*
 * Utility function to get the index map for an SArray. 
 * \param[in] targets      SArray of ground-truth
 * \param[in] predictions  SArray of predictions.
 */
std::unordered_map<flexible_type, size_t> get_index_map(
             const std::shared_ptr<unity_sarray>& unity_targets, 
             const std::shared_ptr<unity_sarray>& unity_predictions) {

  gl_sarray targets(unity_targets);
  gl_sarray predictions(unity_predictions);
  
  // Get the unique labels and put them into a map.
  gl_sarray labels = targets.unique().sort();
  std::unordered_map<flexible_type, size_t> index_map;
  size_t idx = 0;
  for (auto label : labels.range_iterator()) {
    index_map[label] = idx;
    idx++;
  }

  // Check that the number of classes is 2 for the binary classfication mode.
  if ((predictions.dtype() == flex_type_enum::FLOAT) 
       && (index_map.size() != 2)) {
    std::stringstream ss;
    ss << "For this evaluation metric, the input SArray `predictions` " 
       << "can be of type float only when the number of classes is 2" 
       << " (i.e binary classification). This dataset has " << index_map.size()
       << " classes." << std::endl; 
    log_and_throw(ss.str());
  }
  return index_map;
}



/**
 * Evaluation using the streaming evaluation interface.
   TODO: Switch to gl_sarray and the new SDK implementation. 
 */
static constexpr size_t MBSIZE = 5000;
variant_type _supervised_streaming_evaluator(
                           std::shared_ptr<unity_sarray> unity_targets, 
                           std::shared_ptr<unity_sarray> unity_predictions, 
                           std::string metric, 
                           std::map<std::string, flexible_type> kwargs) {

  // Convert to the native types
  std::shared_ptr<sarray<flexible_type>> targets = 
                                  unity_targets->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type>> predictions = 
                                  unity_predictions->get_underlying_sarray();
  DASSERT_EQ(targets->size(), predictions->size());
  DASSERT_TRUE(predictions->size() > 0);

  // Compute the index map if needed.
  std::map<std::string, variant_type> opts;
  for (const auto& kvp: kwargs) {
    opts[kvp.first] = to_variant(kvp.second);
  } 
  const bool needs_index_map = (metric == "auc") || (metric == "roc_curve") ||
      (metric == "binary_logloss") || (metric == "multiclass_logloss");
  if (needs_index_map && opts.count("index_map") == 0) {
    opts["index_map"] =  to_variant(
        get_index_map(unity_targets, unity_predictions));
  }

  // Get the evaluator metrics.
  std::shared_ptr<supervised_evaluation_interface> evaluator = 
                             get_evaluator_metric(metric, opts);

  // Iterate
  auto true_reader = targets->get_reader();
  auto pred_reader = predictions->get_reader();
  size_t current_row = 0;
  size_t nrows_y = MBSIZE;
  TURI_ATTRIBUTE_UNUSED_NDEBUG size_t nrows_yhat = MBSIZE;

  while (nrows_y == MBSIZE) {
    std::vector<flexible_type> current_yhat;
    std::vector<flexible_type> current_y;
    nrows_y = true_reader->read_rows(current_row, current_row + MBSIZE,
        current_y);
    nrows_yhat = pred_reader->read_rows(current_row, current_row + MBSIZE,
        current_yhat);
    DASSERT_EQ(nrows_y, nrows_yhat);

    for (size_t i = 0; i < nrows_y; ++i) {
      evaluator->register_example(current_y[i], current_yhat[i]);
    }
    current_row += MBSIZE;
  }
  return evaluator->get_metric();
}

variant_map_type compute_classifier_metrics_from_probability_vectors(
    std::vector<std::string> metrics, gl_sframe input,
    std::string target_column_name, std::string prediction_probs_column_name,
    flex_list class_labels)
{
  variant_map_type result;

  // Configure an SFrame `data` such that column 0 is the target, column 1 is
  // the probability vector, and column 2 is the most likely class label.
  gl_sframe data =
      input.select_columns({target_column_name, prediction_probs_column_name});
  auto max_prob_label = [&class_labels](const flexible_type& ft) {
    const flex_vec& prob_vec = ft.get<flex_vec>();
    auto max_it = std::max_element(prob_vec.begin(), prob_vec.end());
    return class_labels[max_it - prob_vec.begin()];
  };
  data.add_column(data[prediction_probs_column_name].apply(
      max_prob_label, class_labels.front().get_type()));

  // Handle "confusion_matrix" separately.
  // TODO: Unify with the logic in supervised_learning_model_base::api_evaluate,
  // and with the "confusion_matrix" evaluator returned by get_evaluator_metric.
  auto new_metrics_end = std::remove(metrics.begin(), metrics.end(),
                                     "confusion_matrix");
  if (new_metrics_end != metrics.end()) {
    // Don't expose get_evaluator_metric() to "confusion_matrix".
    metrics.erase(new_metrics_end, metrics.end());

    // Borrow the implementation from the supervised_learning toolkit.
    // Use the column names consistent with that toolkit's api_evaluate.
    result["confusion_matrix"] = supervised::confusion_matrix(
        gl_sframe({ {"class",           data[target_column_name]         },
                    {"predicted_class", data[data.column_names().back()] }  }),
        "class", "predicted_class");
  }

  // Handle "report_by_class" separately.
  // TODO: Implement "report_by_class" using the standard evaluation framework.
  new_metrics_end = std::remove(metrics.begin(), metrics.end(),
                                "report_by_class");
  if (new_metrics_end != metrics.end()) {
    // Don't expose get_evaluator_metric() to "report_by_class".
    metrics.erase(new_metrics_end, metrics.end());

    // Borrow the implementation from the supervised_learning toolkit.
    result["report_by_class"] = supervised::classifier_report_by_class(
        data, target_column_name, data.column_names().back());
  }

  // Construct the class-to-index map.
  std::unordered_map<flexible_type, size_t> class_to_index;
  for (size_t i = 0; i < class_labels.size(); ++i) {
    class_to_index[class_labels[i]] = i;
  }

  // Initialize the evaluators. Note that we always use the "multiclass" version
  // since we have full probability vectors. (The binary versions only expect
  // a single probability, for the "positive" class.)
  using evaluator_shared_ptr = std::shared_ptr<supervised_evaluation_interface>;
  std::map<std::string, variant_type> opts =
      { {"index_map", to_variant(class_to_index)},
        {"binary",    false                     } };
  std::map<std::string, evaluator_shared_ptr> evaluators;
  for (const std::string& metric : metrics) {
    // Apply the default options and tweaks to metric implementations defined
    // by the Python API.
    // TODO: Should the metric implementations themselves (or
    // get_evaluator_metric) define the defaults?
    std::string metric_impl = metric;
    if (metric == "accuracy") {
      opts["average"] = "micro";
      metric_impl = "flexible_accuracy";
    } else if (metric == "auc") {
      opts["average"] = "macro";
    } else if (metric == "f1_score") {
      opts["average"] = "macro";
      opts["beta"] = 1.0;
      metric_impl = "fbeta_score";
    } else if (metric == "log_loss") {
      metric_impl = "multiclass_logloss";
    } else if (metric == "precision") {
      opts["average"] = "macro";
    } else if (metric == "recall") {
      opts["average"] = "macro";
    } else if (metric == "roc_curve") {
      opts["average"] = "default";
    }
    evaluators[metric] = get_evaluator_metric(metric_impl, opts);
  }

  // Traverse the predictions and labels in parallel.
  auto callback = [&evaluators](size_t thread_idx,
                                const std::shared_ptr<sframe_rows>& rows) {
    for (const auto& row : *rows) {
      for (const auto& metric_and_evaluator : evaluators) {
        const evaluator_shared_ptr& evaluator = metric_and_evaluator.second;

        // Feed the probability vector (index 1) or the maximum-probability
        // label (index 2) into the evaluator.
        size_t pred_col_idx = evaluator->is_prob_evaluator() ? 1 : 2;
        evaluator->register_example(row[0], row[pred_col_idx], thread_idx);
      }
    }
    return false;  // Never stop before all rows have been tallied.
  };
  data.materialize_to_callback(callback, turi::thread::cpu_count());

  // Finalize each metric.
  for (const auto& metric_and_evaluator : evaluators) {
    result[metric_and_evaluator.first] =
        metric_and_evaluator.second->get_metric();
  }

  return result;
}

variant_map_type compute_classifier_metrics(
    gl_sframe data, std::string target_column_name, std::string metric,
    gl_sarray predictions, std::map<std::string, flexible_type> opts)
{
  // Expand requested metric into list of actual metrics to compute.
  std::vector<std::string> metrics = get_classifier_metrics(metric);

  // Retrieve the list of classes.
  // Note that "classes" is an "option" to guard against future alternate
  // options, such as inferring labels from the target column or from dictionary
  // keys in the prediction column.
  auto opts_it = opts.find("classes");
  if (opts_it == opts.end()) {
    log_and_throw("Cannot compute classifier metrics without class labels.");
  }
  flex_list class_labels = opts_it->second;

  // Convert predictions if necessary to canonical form: probability vectors.
  predictions = get_prediction_probability_vectors(predictions, class_labels);

  // Construct SFrame with just the targets and predicted probability vectors.
  gl_sframe input = gl_sframe({
      {"target", data[target_column_name]},
      {"class_probs", predictions},
  });

  return compute_classifier_metrics_from_probability_vectors(
      std::move(metrics), std::move(input), "target", "class_probs",
      std::move(class_labels));
}

variant_map_type compute_object_detection_metrics(
    gl_sframe data, std::string annotations_column_name,
    std::string image_column_name, gl_sarray predictions,
    std::map<std::string, flexible_type> opts)
{
  // Retrieve the list of classes.
  // Note that "classes" is an "option" to guard against future alternate
  // options, such as inferring labels from the target column or from dictionary
  // keys in the prediction column.
  auto opts_it = opts.find("classes");
  if (opts_it == opts.end()) {
    log_and_throw("Cannot compute classifier metrics without class labels.");
  }
  flex_list class_labels = opts_it->second;

  // Create a data iterator.
  object_detection::data_iterator::parameters iter_params;
  iter_params.data =
      data.select_columns({annotations_column_name, image_column_name});
  iter_params.data.add_column(predictions);
  iter_params.annotations_column_name = annotations_column_name;
  iter_params.predictions_column_name = iter_params.data.column_names().back();
  iter_params.image_column_name = image_column_name;
  iter_params.class_labels = std::vector<std::string>(class_labels.begin(),
                                                      class_labels.end());
  iter_params.repeat = false;
  object_detection::simple_data_iterator iter(iter_params);

  // Create the evaluator.
  object_detection::average_precision_calculator evaluator(class_labels);

  // Iterate through the labeled data and predictions.
  std::vector<neural_net::labeled_image> batch = iter.next_batch(32);
  while (!batch.empty()) {
    for (const neural_net::labeled_image& instance : batch) {
      evaluator.add_row(instance.predictions, instance.annotations);
    }

    batch = iter.next_batch(32);
  }

  return evaluator.evaluate();
}

//////////////////////////////////////////////////////////////////////////////////////

sframe precision_recall_by_user(
    const sframe& _validation_data,
    const sframe& _recommend_output,
    const std::vector<size_t>& cutoffs) {

  timer timer;
  timer.start();

  gl_sframe recommend_output(_recommend_output);
  gl_sframe validation_data(_validation_data);

  const auto& column_names = recommend_output.column_names();

  if (column_names != recommend_output.column_names()) {
    log_and_throw("Column names in test SFrame do not match recommender SFrame.");
  }

  const std::string& user_column = column_names[0]; 
  const std::string& item_column = column_names[1]; 

  // First, index the columns.
  gl_sframe indexed_validation_data_1, indexed_recommend_output_1;

  auto user_indexer = std::make_shared<ml_data_internal::column_indexer>(
      user_column, ml_column_mode::CATEGORICAL,
      _recommend_output.column_type(user_column));

  auto item_indexer = std::make_shared<ml_data_internal::column_indexer>(
      item_column, ml_column_mode::CATEGORICAL,
      _recommend_output.column_type(item_column));

  user_indexer->initialize();
  item_indexer->initialize();

  std::function<flexible_type(const flexible_type&)> user_index_f =
      [=](const flexible_type& f) -> flexible_type {
    return user_indexer->map_value_to_index(thread::thread_id(), f);
  };
  std::function<flexible_type(const flexible_type&)> item_index_f =
      [=](const flexible_type& f) -> flexible_type {
    return item_indexer->map_value_to_index(thread::thread_id(), f);
  };

  indexed_validation_data_1[user_column] =
      validation_data[user_column].apply(user_index_f, flex_type_enum::INTEGER);
  indexed_validation_data_1[item_column] =
      validation_data[item_column].apply(item_index_f, flex_type_enum::INTEGER);

  sframe indexed_validation_data =
      indexed_validation_data_1.materialize_to_sframe();

  indexed_recommend_output_1[user_column] = recommend_output[user_column].apply(
      user_index_f, flex_type_enum::INTEGER);
  indexed_recommend_output_1[item_column] = recommend_output[item_column].apply(
      item_index_f, flex_type_enum::INTEGER);

  sframe indexed_recommend_output =
      indexed_recommend_output_1.materialize_to_sframe();

  user_indexer->finalize();
  item_indexer->finalize();

  size_t num_users = user_indexer->indexed_column_size();

  // should preserve the order
  indexed_column_groupby pred_ranks(
      indexed_recommend_output.select_column(USER_COLUMN_INDEX),
      indexed_recommend_output.select_column(ITEM_COLUMN_INDEX), false, false);

  indexed_column_groupby val_ranks(
      indexed_validation_data.select_column(USER_COLUMN_INDEX),
      indexed_validation_data.select_column(ITEM_COLUMN_INDEX), false, false);

  sframe ret;
  ret.open_for_write(
      {user_column, "cutoff", "precision", "recall", "count"},
      {user_indexer->column_type(), flex_type_enum::INTEGER,
       flex_type_enum::FLOAT, flex_type_enum::FLOAT, flex_type_enum::INTEGER});

  size_t num_segments = ret.num_segments();


  parallel_for(0, num_segments, [&](size_t sidx) {

    size_t start_idx = (sidx * num_users) / num_segments;
    size_t end_idx = ((sidx + 1) * num_users) / num_segments;

    auto it_out = ret.get_output_iterator(sidx);
    std::vector<flexible_type> out_v;

    for (size_t i = start_idx; i < end_idx; ++i) {
      const std::vector<size_t>& vr = val_ranks.dest_group(i);
      const std::vector<size_t>& pr = pred_ranks.dest_group(i);

      const std::vector<std::pair<double, double>> prv =
          recsys::precision_and_recall(vr, pr, cutoffs);

      for (size_t j = 0; j < cutoffs.size(); ++j, ++it_out) {
        out_v = {user_indexer->map_index_to_value(i), cutoffs[j], prv[j].first,
                 prv[j].second, vr.size()};
        *it_out = out_v;
      }
    }
  });

  ret.close();

  return ret;  
}

}
}
