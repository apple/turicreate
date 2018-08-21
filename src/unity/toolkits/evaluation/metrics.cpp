/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/toolkits/util/precision_recall.hpp>

// ML-Data
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>


// Evaluation
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/evaluation/evaluation_constants.hpp>
#include <unity/toolkits/evaluation/metrics.hpp>

#include <map>
#include <algorithm>


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
    const sframe& validation_data,
    const sframe& recommend_output,
    const std::vector<size_t>& cutoffs) {

  turi::timer timer;
  timer.start();
  
  std::map<std::string, flexible_type> opts 
    = { {"sort_by_first_two_columns", true} };  
  
  const std::string& user_column = recommend_output.column_name(0);
  const std::string& item_column = recommend_output.column_name(1);

  std::map<std::string, v2::ml_column_mode> col_modes =
  { {user_column, v2::ml_column_mode::CATEGORICAL}, 
    {item_column, v2::ml_column_mode::CATEGORICAL} };

  std::vector<std::pair<size_t, size_t> > recommendations;
  std::shared_ptr<v2::ml_metadata> metadata; 

  {
    // Map the recommend output first
    v2::ml_data md_rec(opts);
    md_rec.set_data(recommend_output.select_columns({user_column,item_column}), 
                    "", {}, col_modes);

    md_rec.fill(); 
    metadata = md_rec.metadata();

    // Dump this into a vector. 
    recommendations.resize(md_rec.num_rows());

    in_parallel([&](size_t thread_idx, size_t num_threads) { 
      
      for(auto it = md_rec.get_iterator(thread_idx, num_threads); !it.done(); ++it) { 
        std::vector<v2::ml_data_entry> v; 
        it.fill_observation(v); 
      
        recommendations[it.row_index()] = {v[0].index, v[1].index};
      }
    }); 

    DASSERT_TRUE(std::is_sorted(recommendations.begin(), recommendations.end()));
  }

  v2::ml_data md_val(metadata);
  md_val.fill(validation_data.select_columns({user_column,item_column}));
  
  size_t num_threads = thread::cpu_count();

  sframe ret;
  ret.open_for_write({user_column, "cutoff", "precision", "recall", "count"},
                     {metadata->column_type(user_column), 
                     flex_type_enum::INTEGER, 
                     flex_type_enum::FLOAT,
                     flex_type_enum::FLOAT, 
                     flex_type_enum::INTEGER},
                     "", num_threads);


  typedef decltype(ret.get_output_iterator(0)) out_iter_type; 
  std::vector<out_iter_type> output_iterators(num_threads);
  
  std::vector<std::vector<flexible_type> > out_vv(num_threads);   

  auto add_to_output = [&](
    size_t thread_idx,
    size_t user, 
    const std::vector<size_t>& pr, const std::vector<size_t>& vr) { 

    // Only record ones that are non-empty in the predictions.
    if(pr.empty()) {
      return; 
    }
  
    auto& it_out = output_iterators[thread_idx]; 
    auto& out_v = out_vv[thread_idx];

    const std::vector<std::pair<double, double> > prv =
        turi::recsys::precision_and_recall(vr, pr, cutoffs);

    for(size_t j = 0; j < cutoffs.size(); ++j, ++it_out) {
      out_v = {metadata->indexer(0)->map_index_to_value(user), cutoffs[j], 
        prv[j].first, prv[j].second, vr.size()};
      *it_out = out_v;
    }
  };
  
  // We need to record all the recommendations that were given that have 
  // no presence in 
  std::vector<int> recommendations_processed(metadata->index_size(0), 0);
  atomic<size_t> num_users_processed = 0; 

  in_parallel([&](size_t thread_idx, size_t num_threads) { 
  
    // Set the output iterator 
    output_iterators[thread_idx] = ret.get_output_iterator(thread_idx);

    // do squirrels barf nuts?
    auto val_it = md_val.get_block_iterator(thread_idx, num_threads);
     
    std::vector<v2::ml_data_entry> v;
    std::vector<size_t> vr, pr;  

    if(val_it.done()) { return; }
      
    val_it.fill_observation(v);
    size_t user = v[0].index;

    // Find the starting tracking iterator for this place
    auto rec_it = std::lower_bound(
       recommendations.begin(), 
       recommendations.end(), std::pair<size_t, size_t>{user, 0});

    while(!val_it.done()) { 
      
      vr.clear();

      val_it.fill_observation(v);
      user = v[0].index;
      vr.push_back(v[1].index); 
      ++val_it; 

      for(; !val_it.done() && !val_it.is_start_of_new_block(); ++val_it) { 
        val_it.fill_observation(v); 
        DASSERT_EQ(v[0].index, user); 
        vr.push_back(v[1].index); 
      }

      // Find the starting location of the recommendations for this user
      while(rec_it != recommendations.end() && rec_it->first < user) {
         ++rec_it; 
      }

      // Now, dump them into the predictions.
      pr.clear(); 

      // Copy the recommended items into a buffer. 
      while(rec_it != recommendations.end() && rec_it->first == user) { 
        pr.push_back(rec_it->second);
        ++rec_it;
      }

      // Record that it's been processed.
      DASSERT_LT(user, recommendations_processed.size()); 
      DASSERT_EQ(recommendations_processed[user], 0); 
      recommendations_processed[user] = 1;
      ++num_users_processed;
    
      add_to_output(thread_idx, user, pr, vr); 
    }
  });

  // Have we been able to process everything?  If not, add in the leftovers.
  if(num_users_processed < recommendations_processed.size()) { 
  
    in_parallel([&](size_t thread_idx, size_t num_threads) { 
 
       // Block it up by threads 
      size_t start_idx = (thread_idx * recommendations_processed.size()) / num_threads;
      size_t end_idx   = ((thread_idx+1) * recommendations_processed.size()) / num_threads;

      std::vector<size_t> pr; 

      for(size_t user = start_idx; user < end_idx; ++user) { 
        
        if(recommendations_processed[user]) {
          continue;
        }
        
        // Find these recommendations in the block.
        auto rec_it = std::lower_bound(
           recommendations.begin(), 
           recommendations.end(), std::pair<size_t, size_t>{user, 0});

        // The only way the above iterator would have missed it is if it's here, 
        // but not in the validation set.
        DASSERT_EQ(rec_it->first, user); 

        // Copy the recommended items into a buffer. 
        while(rec_it != recommendations.end() && rec_it->first == user) { 
          pr.push_back(rec_it->second);
          ++rec_it;
        }

        add_to_output(thread_idx, user, pr, {}); 
      }
    }); 
  }

  ret.close();

  return ret;
}


}
}
