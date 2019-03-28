/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/recsys/models/itemcf.hpp>
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/version_number.hpp>
#include <random/random.hpp>
#include <memory>
#include <perf/memory_info.hpp>
#include <unity/toolkits/util/algorithmic_utils.hpp>
#include <sgraph/sgraph_compute.hpp> 
#include <unity/toolkits/recsys/user_item_graph.hpp>
#include <sframe/sframe_iterators.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/unity_global.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/variant.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <generics/symmetric_2d_array.hpp>
#include <unity/toolkits/sparse_similarity/sparse_similarity_lookup.hpp>
#include <unity/toolkits/sparse_similarity/similarities.hpp>
#include <table_printer/table_printer.hpp>
#include <unity/toolkits/coreml_export/mlmodel_include.hpp>
#include <util/try_finally.hpp>

namespace turi { namespace recsys {

void recsys_itemcf::set_extra_data(const std::map<std::string, variant_type>& extra_data) {
  
  // Only try to load nearest_items if it exists.
  if (extra_data.count("nearest_items") == 0)
    return;

  sframe nearest_items = *(safe_varmap_get<std::shared_ptr<unity_sframe> >(extra_data, "nearest_items"))->get_underlying_sframe();

  // If empty, return.
  if (nearest_items.num_rows() == 0)
    return;

  // Check column names 
  const std::string& item_column = metadata->column_name(ITEM_COLUMN_INDEX);

  if (!(nearest_items.contains_column(item_column)) ||
      !(nearest_items.contains_column("similar")) ||
      !(nearest_items.contains_column("score"))) {

    log_and_throw((std::string("When providing an SFrame describing the item-to-item "
                               "similarity it must contain columns named '")
                   + item_column + "', 'similar', and 'score'.").c_str());
  }

  size_t item_id_col_idx = nearest_items.column_index(item_column);
  size_t similar_col_idx = nearest_items.column_index("similar");

  // Check column types
  if ((nearest_items.column_type(item_id_col_idx) != item_type()) ||
      (nearest_items.column_type(similar_col_idx) != item_type())) {

    log_and_throw("When providing an SFrame describing the item-to-item "
                  "similarity, the type of the 'item_id' and 'similar' "
                  "columns must match the type of the item column in the "
                  "observation data.");
  }

  // index nearest_items
  // allowing new categorical values 
  auto user_indexer = metadata->indexer(USER_COLUMN_INDEX);
  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);

  std::string user_column_name = metadata->column_name(USER_COLUMN_INDEX);
  std::string item_column_name = metadata->column_name(ITEM_COLUMN_INDEX);

  ////////////////////////////////////////////////////////////////////////////////
  // Now, go through and index all this data.

  bool allow_new_categorical_values = true;

  nearest_items = nearest_items.replace_column(
      v2::map_to_indexed_sarray(item_indexer,
                                nearest_items.select_column(item_column_name),
                                allow_new_categorical_values),
      item_column_name);

  nearest_items = nearest_items.replace_column(
      v2::map_to_indexed_sarray(item_indexer,
                                nearest_items.select_column("similar"),
                                allow_new_categorical_values),
      "similar");

  ////////////////////////////////////////////////////////////////////////////////
  // Save it for use during training.

  user_provided_data.reset(new user_provided_data_struct);
  user_provided_data->nearest_items = nearest_items;
}

void recsys_itemcf::load_user_provided_data() {

  if(user_provided_data == nullptr) {
    return;
  }

  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);
  std::string item_column_name = metadata->column_name(ITEM_COLUMN_INDEX);

  // Add them to the item similarity.
  item_sim->setup_by_raw_similarity(
      item_indexer->indexed_column_size(),
      flex_list(),
      user_provided_data->nearest_items,
      item_column_name, "similar", "score", false);
}

////////////////////////////////////////////////////////////////////////////////

void recsys_itemcf::init_options(const std::map<std::string, flexible_type>&_options) {

  option_handling::option_info opt;

  opt.name           = "user_id";
  opt.description    = "The name of the column for user ids.";
  opt.default_value  = "user_id";
  opt.parameter_type = option_handling::option_info::STRING;
  options.create_option(opt); 

  opt.name           = "item_id";
  opt.description    = "The name of the column for item ids.";
  opt.default_value  = "item_id";
  opt.parameter_type = option_handling::option_info::STRING;
  options.create_option(opt); 

  opt.name           = "target";
  opt.description    = "The name of the column of target ratings to be predicted.";
  opt.default_value  = "";
  opt.parameter_type = option_handling::option_info::STRING;
  options.create_option(opt); 

  opt.name = "similarity_type";
  opt.description = "Similarity function to use for comparing two items.";
  opt.default_value = "jaccard";
  opt.parameter_type = option_handling::option_info::STRING;
  opt.allowed_values = {"jaccard", "cosine", "pearson"};
  options.create_option(opt); 

  opt.name = "seed_item_set_size";
  opt.description = ("For users that have not yet rated any items, or have only "
                     "rated items with no co-occuring items and hence no similar items, "
                     "the model assumes the user given the most popular items "
                     "their mean rating. This parameter controls the size of this seed set.");
  opt.default_value = 50;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<flex_int>::max();
  opt.parameter_type = option_handling::option_info::INTEGER;
  options.create_option(opt);
  
  sparse_similarity_lookup::add_options(options);

  // Set user specified options
  options.set_options(_options);

  // Save options to state variable
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

std::map<std::string, flexible_type> recsys_itemcf::train(const v2::ml_data& data) {

  turi::timer training_timer;
  training_timer.start();

  auto _item_sim = create_similarity_lookup();
  
  std::map<std::string, flexible_type> ret;

  // If there is extra data present, then also add that in.
  item_sim = _item_sim;

  // If needed, load extra data.
  if(user_provided_data != nullptr) {
    logprogress_stream << "Loading user-provided nearest items." << std::endl;
    load_user_provided_data();
  } else {
    logprogress_stream << "Training model from provided data. " << std::endl;
    ret = item_sim->train_from_sparse_matrix_sarray(
        metadata->index_size(ITEM_COLUMN_INDEX), this->trained_user_items);
  }

  // Now, go through and populate a list of seed items for new users.
  // Assume that user is boring -- i.e. they simply rated the most
  // popular items at the average rating.

  {
    logprogress_stream << "Generating candidate set for working with new users." << std::endl;

    // Get the num_items.
    size_t num_items = data.metadata()->index_size(ITEM_COLUMN_INDEX);
    
    // Go through and calculate the means. 
    std::array<simple_spinlock, 1024> locks;
    
    item_mean_score.assign(num_items, 0); 

    // Now, do a pass thorugh the data to calculate all this.
    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<v2::ml_data_entry> x;
        
        for(auto it = data.get_iterator(thread_idx, num_threads); !it.done(); ++it) {
          it.fill_observation(x);

          size_t item = x[ITEM_COLUMN_INDEX].index;
          double value = it.target_value();

          DASSERT_LT(item, num_items);

          std::lock_guard<simple_spinlock> lg(locks[item % locks.size()]);
          item_mean_score[item] += value;
        }
      });

    // Normalize and gather the spread.
    item_mean_min = std::numeric_limits<double>::max();
    item_mean_max = std::numeric_limits<double>::lowest();
    for(size_t item = 0; item < num_items; ++item) {
      if(data.metadata()->has_target()) {
        item_mean_score[item]
            /= std::max<size_t>(1, data.metadata()->statistics(ITEM_COLUMN_INDEX)->count(item));
      }
            
      item_mean_min = std::min(item_mean_min, item_mean_score[item]);
      item_mean_max = std::max(item_mean_max, item_mean_score[item]);
    }
    
    // Now, choose the most popular items for the candidate seed set.
    size_t seed_count = options.value("seed_item_set_size");
    seed_count = std::min<size_t>(seed_count, num_items);
    new_user_seed_items.resize(seed_count);

    if(seed_count != 0) {     
      // Choose a bunch of the most frequent items to use as seed items.
      std::vector<std::pair<size_t, size_t> > item_counts(num_items);
    
      for(size_t i = 0; i < num_items; ++i) {
        item_counts[i] = {data.metadata()->statistics(ITEM_COLUMN_INDEX)->count(i), i};
      }

      std::nth_element(item_counts.begin(), item_counts.begin() + seed_count, item_counts.end(),
                       [](const std::pair<size_t, size_t>& p1, const std::pair<size_t, size_t>& p2) {
                         return p1.first > p2.first;
                       });

    
      for(size_t i = 0; i < seed_count; ++i) {
        size_t index = item_counts[i].second;
        double score = ((item_mean_score[index] - item_mean_min)
                        / (std::max<double>(1.0, item_mean_max - item_mean_min)));
        new_user_seed_items[i] = {index, score};
      }
      
      std::sort(new_user_seed_items.begin(), new_user_seed_items.end());
    }
  }

  add_or_update_state({{"training_time", training_timer.current_time()}});

  // This is important; otherwise, it gets calculated and it's
  // meaningless here.
  add_or_update_state({{"training_rmse", FLEX_UNDEFINED}});
  
  logprogress_stream << "Finished training in "
                     << training_timer.current_time() << "s" << std::endl;
  
  return ret;
}

sframe recsys_itemcf::predict(const v2::ml_data& test_data) const {
  
  std::shared_ptr<sarray<flexible_type> > ret(new sarray<flexible_type>);

  ret->open_for_write(1);
  ret->set_type(flex_type_enum::FLOAT);

  ////////////////////////////////////////////////////////////////////////////////
    
  turi::timer total_timer;
  total_timer.start();

  auto trained_user_items_reader = trained_user_items->get_reader();
  
  size_t num_users = metadata->index_size(USER_COLUMN_INDEX);
  size_t n = test_data.size();
  size_t n_left = n;

  // Something that can reasonbly fit in memory.
  const size_t block_size = 1024*1024;
  auto it_out = ret->get_output_iterator(0);
  auto it = test_data.get_iterator();
  
  std::vector<double> out_values;
  
  // A map from the user idx to the (item_index, out_index) values. 
  std::map<size_t, std::vector<std::pair<size_t, size_t> > > user_to_entry_map;
  std::vector<v2::ml_data_entry> x;
  std::vector<std::pair<size_t, double> > scores;

  std::vector<std::vector<std::pair<size_t, double> > > user_item_v;

  // Go through in blocks, with all the users in a block scored
  // together for efficiency.  It's basically the same cost to run
  // score_items, which takes one user, on 50 items as 1 or 500.
  // Thus, for each unique user, we run that function once.  The code
  // below is designed to aggregate all of this.
  for(size_t block_start_idx = 0; block_start_idx < n; block_start_idx += block_size) {
    user_to_entry_map.clear();
    
    size_t end_idx = std::min(n_left, block_size);
    size_t count = 0;

    // iterate through each row in the test set, and register that
    // value in the user_to_entry_map.  This map will be used to make
    // predictions.
    for(; !it.done() && count < end_idx; ++it) {

      it.fill_observation(x);
        
      // the (user, item) pair for which we need to make a prediction
      size_t user = x[USER_COLUMN_INDEX].index;
      size_t item = x[ITEM_COLUMN_INDEX].index;

      // Aggregate all the new users together.
      if(user >= num_users)
        user = num_users;

      user_to_entry_map[user].push_back( {item, count});
      ++count;
    }

    out_values.resize(count);
    n_left -= count;

    if(n <= block_size) {
      DASSERT_EQ(n_left, 0);
      DASSERT_EQ(count, n);
      DASSERT_TRUE(it.done());
    }

    for(const auto& p : user_to_entry_map) {
      size_t user = p.first;
      const auto& requested_items = p.second;
          
      scores.resize(requested_items.size());
      for(size_t i = 0; i < requested_items.size(); ++i) {
        scores[i].first = requested_items[i].first;
      }

      if(user >= num_users) {
        _score_items(scores, new_user_seed_items); 
      } else {
        trained_user_items_reader->read_rows(user, user + 1, user_item_v);
        DASSERT_EQ(user_item_v.size(), 1); 
        _score_items(scores, user_item_v.at(0));
      }

      for(size_t i = 0; i < requested_items.size(); ++i) {
        out_values[requested_items[i].second] = scores[i].second;
      }
    } // End loop over users.

    // Now, write it all out to the output sarray
    for(size_t i = 0; i < count; ++i, ++it_out) {
      *it_out = out_values[i];
    }
  }

  ret->close();

  DASSERT_EQ(ret->size(), test_data.size());

  return sframe(std::vector<std::shared_ptr<sarray<flexible_type> > >{ret},
                std::vector<std::string>{"prediction"});
}

void recsys_itemcf::_score_items(std::vector<std::pair<size_t, double> >& item_scores, 
                                 const std::vector<std::pair<size_t, double> >& user_scores) const {
  // Score the items.
  size_t n_scores_given = item_sim->score_items(item_scores, user_scores);

  // If, for some bizarre reason, the model actually doesn't score
  // anything -- which is possible for unique (user, item) pairs --
  // instead use the candidate set to generate such items.
  if(UNLIKELY(n_scores_given == 0) ) {
    if(&user_scores != &new_user_seed_items) {
      n_scores_given = item_sim->score_items(item_scores, new_user_seed_items);
    }
    
    // If this still didn't fix it, then put in the normalized average
    // ratings.
    if(n_scores_given == 0) {
      for(auto& p : item_scores) {
        size_t item = p.first;
        if(item >= item_mean_score.size()) {
          p.second = 0;
          continue;
        }
        
        p.second = (item_mean_score[item] - item_mean_min)
            / (std::max<double>(1, item_mean_max - item_mean_min));
        DASSERT_LE(p.second, 1);
        DASSERT_GE(p.second, -1);
      }
    }
  }
}

void recsys_itemcf::score_all_items(
    std::vector<std::pair<size_t, double> >& item_scores,
    const std::vector<v2::ml_data_entry>& query_row,
    size_t top_k,
    const std::vector<std::pair<size_t, double> >& trained_user_item_list,
    const std::vector<std::pair<size_t, double> >& new_user_item_interactions,
    const std::vector<v2::ml_data_row_reference>& new_observation_data,
    const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const {

  // Need to choose the appropriate data source
  const std::vector<std::pair<size_t, double> >* uiv_ptr;

  // We have both current data and new user item data.  For this
  // case, we need to merge both into a buffer, then use that buffer.
  size_t thread_idx = thread::thread_id();

  if(!user_item_buffers_setup) {
    std::lock_guard<mutex> lg(init_user_item_buffers_lock);
    if(!user_item_buffers_setup) {
      user_item_buffers_by_thread.resize(thread::cpu_count());
      user_item_buffers_setup = true;
    }
  }

  DASSERT_LT(thread_idx, user_item_buffers_by_thread.size());
  auto& user_item_buffer = user_item_buffers_by_thread[thread_idx];
  user_item_buffer.clear();

  if(!trained_user_item_list.empty()) {
    if(!new_user_item_interactions.empty()) {

      user_item_buffer.reserve(trained_user_item_list.size() + new_user_item_interactions.size());

      auto it_1 = trained_user_item_list.cbegin();
      auto it_1_end = trained_user_item_list.cend();
      auto it_2 = new_user_item_interactions.cbegin();
      auto it_2_end = new_user_item_interactions.cend();

      while(true) {
        if(UNLIKELY(it_1 == it_1_end)) {
          for(;it_2 != it_2_end; ++it_2) {
            user_item_buffer.push_back(*it_2);
          }
          break;
        }
        if(UNLIKELY(it_2 == it_2_end)) {
          for(;it_1 != it_1_end; ++it_1) {
            user_item_buffer.push_back(*it_1);
          }
          break;
        }

        // Need to append the items to user_item_buffer, favoring the
        // stuff in new_user_item_interactions, and ensuring there are
        // no duplicates.
        if(it_1->first == it_2->first) {
          user_item_buffer.push_back(*it_1);
          ++it_1;
          ++it_2;
          continue;
        }

        user_item_buffer.push_back(*((it_1->first < it_2->first) ? (it_1)++ : (it_2)++));
      }

      uiv_ptr = &(user_item_buffer);
    } else {

      // We only have the trained_user_item_list to work with.
      uiv_ptr = &(trained_user_item_list);
    }
  } else {

    if(!new_user_item_interactions.empty()) {

      // We only have new user-item interactions, so use these.
      uiv_ptr = &(new_user_item_interactions);
    } else {

      // Both are empty, so use the seeded data
      uiv_ptr = &(new_user_seed_items);
    }
  }

  DASSERT_FALSE(uiv_ptr->empty());
  _score_items(item_scores, *uiv_ptr);
}

////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<sparse_similarity_lookup> recsys_itemcf::create_similarity_lookup() const {

  std::string similarity_measure = get_option_value("similarity_type");
  return sparse_similarity_lookup::create(similarity_measure, options.current_option_values());
}

//////////////////////////////////////////////////////////////////////////z//////

void recsys_itemcf::internal_save(turi::oarchive& oarc) const {
  std::map<std::string, variant_type> data;
  
  data["new_user_seed_items"] = to_variant(new_user_seed_items);
  data["item_mean_score"]     = to_variant(item_mean_score);
  data["item_mean_min"]       = to_variant(item_mean_min);
  data["item_mean_max"]       = to_variant(item_mean_max);

  variant_deep_save(to_variant(data), oarc);
  
  oarc << item_sim;
}

void recsys_itemcf::internal_load(turi::iarchive& iarc, size_t version) {

#define __EXTRACT(n) n = variant_get_value<decltype(n)>(data[#n])

  if(version <= 1) {

    std::map<std::string, variant_type> data;

    variant_deep_load(data, iarc);

    std::vector<std::vector<std::pair<flexible_type, flexible_type> > > ranked_items;
    std::vector<std::pair<flexible_type, flexible_type> > new_user_seed_items;

    __EXTRACT(ranked_items);
    __EXTRACT(item_mean_score);
    __EXTRACT(new_user_seed_items); 

    this->new_user_seed_items.assign(new_user_seed_items.begin(), new_user_seed_items.end());

    enum similarity_type {JACCARD, COSINE, PEARSON};
    similarity_type type;
    bool has_target;

    iarc >> has_target >> type;
    
    std::string similarity_name;
    switch(type) {
      case JACCARD: similarity_name = "jaccard"; break;
      case COSINE:  similarity_name = "cosine";  break;
      case PEARSON: similarity_name = "pearson"; break;
    }

    // Add in a couple of new options that were not included in the
    // previous version.
    std::map<std::string, flexible_type> _opts = options.current_option_values();

    // Properly convert the only_top_k parameter to the max_item_neighborhood_size.
    size_t max_item_neighborhood_size = 0;
    if(_opts.count("only_top_k")) {
      max_item_neighborhood_size = _opts.at("only_top_k");

      // Go through the possible cases of only_top_k
      if(max_item_neighborhood_size == 0) {
        // Set to current default.
        max_item_neighborhood_size = 64;
      } else {

        // Need to make sure this isn't too large; do this by
        // truncating it to the largest element in ranked_items.
        size_t max_row_size = 0;
        for(const auto& v : ranked_items) {
          max_row_size = std::max(v.size(), max_row_size);
        }

        max_item_neighborhood_size = std::min(max_row_size, max_item_neighborhood_size);
      }
    } else {
      // Set to current default.
      max_item_neighborhood_size = 64;
    }

    _opts["max_item_neighborhood_size"] = max_item_neighborhood_size;
    
    // Now, use this data to populate the new model.
    item_sim = sparse_similarity_lookup::create(similarity_name, _opts);

    // Convert the form of the item data to a flex_list.
    flex_list item_data;
    item_data.assign(item_mean_score.begin(), item_mean_score.end());

    // Dump the ranked items into the sarray of flex_dict format
    // required for the item lookup tables.
    size_t max_num_threads = thread::cpu_count();
    sframe item_item_similarities;
    item_item_similarities.open_for_write(
        {"item_id", "similar", "score"},
        {flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::FLOAT},
        "",
        max_num_threads);

    in_parallel([&](size_t thread_idx, size_t num_threads) {
        size_t row_idx_start = (thread_idx * ranked_items.size()) / num_threads;
        size_t row_idx_end = ((thread_idx+1) * ranked_items.size()) / num_threads;

        // Now, go through all of them
        auto it_out = item_item_similarities.get_output_iterator(thread_idx);

        for(size_t row_idx = row_idx_start; row_idx < row_idx_end; ++row_idx) {
          for(const auto& p : ranked_items[row_idx]) {
            *it_out = {row_idx, p.first, p.second};
            ++it_out;
          }
        }
      });

    item_item_similarities.close();

    item_sim->setup_by_raw_similarity(
        metadata->index_size(ITEM_COLUMN_INDEX),
        item_data,
        item_item_similarities,
        "item_id", "similar", "score", false);

    // Finally, calculate the item_mean_score for the current methods
    item_mean_min = *std::min_element(item_mean_score.begin(), item_mean_score.end());
    item_mean_max = *std::max_element(item_mean_score.begin(), item_mean_score.end());

  } else {

    variant_type data_v;
    variant_deep_load(data_v, iarc);
    
    auto data = variant_get_value<std::map<std::string, variant_type> >(data_v);

    // For some reason, 4.0 and earlier version 2 models had new_user_seed_items
    // stored as vector<vector<double>> instead of vector<pair<size_t, double>>.
    // Not sure how this happened, since this code hasn't changed since 4.0,
    // but let's try to convert it now...
    try {
      new_user_seed_items = std::vector<std::pair<size_t, double> >();
      std::vector<std::vector<double>> vec_new_user_seed_items;
      variant_get_value<decltype(vec_new_user_seed_items)>(data["new_user_seed_items"]);
      for (const auto& vec : vec_new_user_seed_items) {
        DASSERT_EQ(vec.size(), 2);
        size_t item_id = vec[0];
        double value = vec[1];
        new_user_seed_items.push_back(std::make_pair(item_id, value));
      }
    } catch (const flexible_type_internals::type_conversion_error& e) {
      // for normal 4.1+ models, this works
      __EXTRACT(new_user_seed_items);
    }

    __EXTRACT(item_mean_score);
    __EXTRACT(item_mean_min);
    __EXTRACT(item_mean_max);
    
    iarc >> item_sim;
  }
  
#undef __EXTRACT
}

sframe recsys_itemcf::get_similar_items(
    std::shared_ptr<sarray<flexible_type> > items,
    size_t topk) const {

  size_t num_items = metadata->index_size(ITEM_COLUMN_INDEX);

  // return for all items if the ptr is null or the indexed_items is empty
  bool return_for_all_items = (items == nullptr); 
  const size_t n_indexed_items = return_for_all_items ? num_items : items->size();

  // set allow_new_categorical_values=false to skip items that not in
  // training data.
  // index nearest_items
  // allowing new categorical values
  auto user_indexer = metadata->indexer(USER_COLUMN_INDEX);
  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);

  std::string user_column_name = metadata->column_name(USER_COLUMN_INDEX);
  std::string item_column_name = metadata->column_name(ITEM_COLUMN_INDEX);

  // return all neighbors if topk == 0
  if (topk == 0 || topk >= num_items)  {
    topk = num_items - 1;
  }

  std::vector<std::string> column_names =
      {item_column_name, "similar", "score", "rank"};

  std::vector<flex_type_enum> column_types = {
    metadata->column_type(ITEM_COLUMN_INDEX),
    metadata->column_type(ITEM_COLUMN_INDEX),
    flex_type_enum::FLOAT,
    flex_type_enum::INTEGER};

  const size_t max_num_threads = thread::cpu_count();
  sframe ret;
  ret.open_for_write(column_names, column_types, "", max_num_threads);
  std::unique_ptr<typename sarray<flexible_type>::reader_type> reader;

  if(items != nullptr) {
    reader = items->get_reader(max_num_threads);
  }

  turi::timer log_timer;
  log_timer.start();

  in_parallel([&](size_t thread_idx, size_t n_threads) {

      size_t thread_start_idx = (thread_idx * n_indexed_items) / n_threads;
      size_t thread_end_idx = ((thread_idx + 1) * n_indexed_items) / n_threads;

      auto out = ret.get_output_iterator(thread_idx);

      std::vector<flexible_type> out_x_v;
      std::vector<std::pair<size_t, flexible_type> > item_neighbor_list;
      std::vector<flexible_type> in_item_buffer;

      const size_t block_size = 64;

      for(size_t outer_idx = thread_start_idx;
          outer_idx < thread_end_idx;
          outer_idx += block_size) {

        size_t block_end_idx = std::min(outer_idx + block_size, thread_end_idx);

        if(!return_for_all_items) {
          reader->read_rows(outer_idx, block_end_idx, in_item_buffer);
        }

        for(size_t inner_idx = 0; inner_idx < block_end_idx - outer_idx; ++inner_idx) {

          size_t row_idx = outer_idx + inner_idx;

          // if return_for_all, iterate over all items; otherwise pick from sarray
          size_t item;
          if (return_for_all_items) {
            item = row_idx;
          } else {
            item = item_indexer->immutable_map_value_to_index(in_item_buffer[inner_idx]);
          }

          // if a provided item is not in training data, it will be indexed as size(-1)
          // skip these items
          if (item >= num_items)
            continue;

          item_sim->get_similar_items(item_neighbor_list, item, topk);

          const flexible_type& item_ft = item_indexer->map_index_to_value(item);

          // now output to the sframe
          for (size_t idx = 0; idx < item_neighbor_list.size(); idx++) {

            out_x_v = {item_ft,
                       item_indexer->map_index_to_value(item_neighbor_list[idx].first),
                       item_neighbor_list[idx].second,
                       idx + 1};

            *out = out_x_v;
            ++out;
          }
        }
      }

  });

  ret.close();

  return ret;
}



// TODO: resolve these issues at the source level
#if defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wswitch"
#elif defined (__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wswitch"
#endif

std::shared_ptr<coreml::MLModelWrapper> recsys_itemcf::export_to_coreml(const std::string& filename) {

  std::shared_ptr<CoreML::Model> coreml_model = std::make_shared<CoreML::Model>(
      std::string("Item Similarity Recommender Model exported from Turi Create ") + __UNITY_VERSION__);

  auto& proto = coreml_model->getProto();
  auto* desc = proto.mutable_description();
  auto* interactions_feature = desc->add_input();

  std::string target_column = this->get_option_value("target");
  bool target_is_present = (target_column != "");

  interactions_feature->set_name("interactions");
  std::string short_desc_with_ratings = std::string("The user's interactions, represented as a dictionary, where the keys are the item IDs, and the values are the respective ratings.");
  std::string short_desc_no_ratings = std::string("The user's interactions, represented as a dictionary, where the keys are the item IDs, and the values are sentinel values.");
  if (target_is_present) {
    interactions_feature->set_shortdescription(short_desc_with_ratings);
  } else {
    interactions_feature->set_shortdescription(short_desc_no_ratings);
  }

  auto* interactions_feature_type = interactions_feature->mutable_type();

  // TODO - what if it has values > 32 bit? MLMultiArray only supports 32-bit ints.
  // TODO - what if it's string? how to tell? can it be anything else?
  // in that case, possible to use CoreML feature transforms to get the inputs to make sense?
  switch (this->item_type()) {
    case (turi::flex_type_enum::INTEGER): {
      interactions_feature_type->mutable_dictionarytype()->mutable_int64keytype();
      break;
    }
    case (turi::flex_type_enum::STRING): {
      interactions_feature_type->mutable_dictionarytype()->mutable_stringkeytype(); 
      break;
    }
  }

  // Top k input
  auto* top_k_input = desc->add_input();
  top_k_input->set_name("k");
  top_k_input->set_shortdescription("Return the top k recommendations.");
  auto* top_k_input_type = top_k_input->mutable_type();
  // TODO BUG? Custom model doesn't seem to allow optional outputs
  //top_k_input_type->set_isoptional(true); // Should not be required. Defaults to 5?
  top_k_input_type->mutable_int64type();

  // Set up outputs
  auto* rank_output = desc->add_output();
  rank_output->set_name("recommendations");
  rank_output->set_shortdescription("Top k recommendations.");
  auto* rank_output_type = rank_output->mutable_type();
  switch (this->item_type()) {
    case (turi::flex_type_enum::INTEGER): {
      rank_output_type->mutable_dictionarytype()->mutable_int64keytype();
      break;
    }
    case (turi::flex_type_enum::STRING): {
      rank_output_type->mutable_dictionarytype()->mutable_stringkeytype();
      break;
    }
  }
  

  auto* probability_output = desc->add_output();
  probability_output->set_name("probabilities");
  probability_output->set_shortdescription("The probability for each recommendation in the top k.");
  auto* probability_output_type = probability_output->mutable_type();
  switch (this->item_type()) {
    case (turi::flex_type_enum::INTEGER): {
      probability_output_type->mutable_dictionarytype()->mutable_int64keytype();
      break;
    }
    case (turi::flex_type_enum::STRING): {
      probability_output_type->mutable_dictionarytype()->mutable_stringkeytype();
      break;
    }
  }

  // Set up model parameters
  auto* custom_model = proto.mutable_custommodel();
  custom_model->set_classname("TCRecommender");
  custom_model->set_description("Turi Create Recommender support for Core ML");
  auto* custom_model_parameters = custom_model->mutable_parameters();
  auto bytes_value = CoreML::Specification::CustomModel::CustomModelParamValue();

  std::stringstream ss;

  // Swap out the user data, as this doesn't need to get exported with the model.
  auto metadata_bk = this->metadata;
  auto trained_user_items_bk = this->trained_user_items; 

  { 
    scoped_finally metadata_guard([&](){ 
        this->metadata = metadata_bk; 
        this->trained_user_items = trained_user_items_bk;
        });
  
    this->trained_user_items.reset(new sarray<std::vector<std::pair<size_t, double> > > );
    this->trained_user_items->open_for_write();
    this->trained_user_items->close();

    this->metadata = this->metadata->select_columns(
        {this->metadata->column_name(0), this->metadata->column_name(1)},
        true, 
        {this->metadata->column_name(USER_COLUMN_INDEX)});

    this->save_model_to_data(ss);
  }

  bytes_value.set_bytesvalue(ss.str());

  (*custom_model_parameters)["turi_create_model"] = bytes_value;

  if (filename != "") {
    CoreML::Result r = coreml_model->save(filename);
    if (!r.good()) {
      log_and_throw(r.message());
    }
  }

  return std::make_shared<coreml::MLModelWrapper>(coreml_model);

}

#if defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined (__clang__)
  #pragma clang diagnostic pop
#endif

}} // namespace 
