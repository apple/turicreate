/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

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


namespace turi {
namespace evaluation {

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
