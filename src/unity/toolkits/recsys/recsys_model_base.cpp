/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <flexible_type/flexible_type_base_types.hpp>
#include <sframe/sframe_iterators.hpp>
#include <util/try_finally.hpp>

#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>

#include <unity/toolkits/recsys/models.hpp>
#include <unity/toolkits/recsys/user_item_lists.hpp>
#include <unity/toolkits/recsys/recsys_model_base.hpp>
#include <unity/toolkits/recsys/train_test_split.hpp>
#include <unity/toolkits/util/precision_recall.hpp>
#include <unity/toolkits/util/sframe_utils.hpp>
#include <unity/toolkits/evaluation/metrics.hpp>
#include <util/fast_top_k.hpp>
#include <timer/timer.hpp>
#include <algorithm>
#include <logger/logger.hpp>
#include <sstream>
#include <unity/toolkits/coreml_export/mlmodel_wrapper.hpp>

// Types
#include <unity/lib/variant.hpp>
#include <unity/lib/variant_deep_serialize.hpp>

namespace turi { namespace recsys {


v2::ml_data recsys_model_base::create_ml_data(const sframe& data,
                                              const sframe& new_user_side_data,
                                              const sframe& new_item_side_data) const {
  bool immutable_metadata = false;
  v2::ml_data d(metadata, immutable_metadata);

  d.set_data(data);

  d.add_side_data(new_item_side_data);
  d.add_side_data(new_user_side_data);

  d.fill();

  return d;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Data interaction and training functions
//
////////////////////////////////////////////////////////////////////////////////

void recsys_model_base::setup_and_train(
    const sframe& data,
    const sframe& user_side_data,
    const sframe& item_side_data,
    const std::map<std::string, variant_type>& other_data) {

  DASSERT_TRUE(data.is_opened_for_read());

  turi::timer t;
  t.start();

  // set up the metadata
  std::string user_column = get_option_value("user_id");
  std::string item_column = get_option_value("item_id");

  bool is_als = false;
  if (state.count("solver") > 0){
  std::string solver = variant_get_value<std::string>(state.at("solver"));
    is_als = ( (solver == "ials") || (solver == "als"));
    if (is_als && (get_option_value("num_factors") == 0)) {
      log_and_throw("For solver='" + solver +  "', num_factors must be > 0.");
    }
  }

  if (user_column == item_column)
    log_and_throw("User column and item column must be different.");

  size_t orig_user_column_index = data.column_index(user_column);
  size_t orig_item_column_index = data.column_index(item_column);

  std::vector<std::string> column_ordering = {user_column, item_column};
  std::vector<std::string> included_columns = column_ordering;

  std::string target_column = get_option_value("target");
  bool target_is_present = (target_column != "");

  if(target_is_present && !data.contains_column(target_column)) {
    log_and_throw(std::string("Target column given as '")
                  + target_column + "', but this is not present in the provided data.");
  }

  bool use_target = use_target_column(target_is_present);

  size_t orig_target_column_index = size_t(-1);
  if (use_target) {

    if (target_column == user_column || target_column == item_column)
      log_and_throw("Target column must be different than the user column and the item column.");

    if(target_column == "")
      log_and_throw(std::string("Method ")
                    + name() + " requires a numeric target column of scores or ratings; please specify this column using target_column = <name>.");

    if (!data.contains_column(target_column))
      log_and_throw(std::string("Method ")
                    + name() + " requires a numeric target column of scores or ratings; The provided target column " + target_column + " not found.");

    orig_target_column_index = data.column_index(target_column);
    included_columns.push_back(target_column);
  } else {
    target_column = "";
  }

  // See if there are additional columns present:
  std::vector<size_t> additional_columns;
  for(size_t i = 0; i < data.num_columns(); ++i) {
    if(i != orig_user_column_index
       && i != orig_item_column_index
       && i != orig_target_column_index) {
      additional_columns.push_back(i);
    }
  }

  if(!additional_columns.empty()) {

    if(include_columns_beyond_user_item()) {

      for(size_t c_idx : additional_columns)
        included_columns.push_back(data.column_name(c_idx));

    } else {

      if(additional_columns.size() == 1
         && !use_target
         && (data.column_type(additional_columns[0]) == flex_type_enum::FLOAT
             || data.column_type(additional_columns[0]) == flex_type_enum::INTEGER)) {

        logprogress_stream << "Warning: " << "Column '"
                           << data.column_name(additional_columns[0])
                           << "' ignored." << std::endl;
        logprogress_stream << "    To use this column as the target, set target = \""
                           << data.column_name(additional_columns[0])
                           << "\" and use a method that allows the use of a target."
                           << std::endl;
      } else {

        std::ostringstream columns_ss;

        for(size_t j = 0; j < additional_columns.size() - 1; ++j)
          columns_ss << data.column_name(additional_columns[j]) << ", ";

        columns_ss << data.column_name(additional_columns.back());

        if(!use_target) {
          logprogress_stream
              << "Warning: Ignoring columns " << columns_ss.str() << ";"
              << std::endl;
          logprogress_stream
              << "    To use one of these as a target column, set target = <column_name> "
              << std::endl;
          logprogress_stream
              << "    and use a method that allows the use of a target."
              << std::endl;
        } else {
          logprogress_stream
              << "Warning: Ignoring columns " << columns_ss.str() << ";"
              << std::endl;
          logprogress_stream
              << "    To use these columns in scoring predictions, use a model that allows the use of additional features."
              << std::endl;
        }
      }
    }
  }

  // Construct the first ml_data instance
  v2::ml_data train_ml( {
      {"sort_by_first_two_columns_on_train", true},
      {"uniquify_side_column_names", true},
      {"target_column_always_numeric", true},
      {"ignore_new_columns_after_train", true}});

  // Add in the primary data
  train_ml.set_data(data.select_columns(included_columns),

                    target_column,

                    // forced column ordering
                    {user_column, item_column},

                    // Mode overrides -- make sure these are treated this way.
                    { {user_column, v2::ml_column_mode::CATEGORICAL},
                      {item_column, v2::ml_column_mode::CATEGORICAL} } );

  if(user_side_data.num_columns() != 0 && is_als == false) {
    train_ml.add_side_data(user_side_data, user_column);
  }

  if(item_side_data.num_columns() != 0 && is_als == false) {
    train_ml.add_side_data(item_side_data, item_column);
  }

  if((item_side_data.num_columns() + user_side_data.num_columns() > 0)
                                                        && (is_als == true)) {
    logprogress_stream << "Warning: "
        << "This solver currently does not support side features. "
        << "Proceeding with training without side features."
        << std::endl;
  }

  logprogress_stream << "Preparing data set." << std::endl;
  train_ml.fill();

  metadata = train_ml.metadata();

  ////////////////////////////////////////////////////////////

  // Set other data.  Note -- this sometimes changes the indexing,
  // hence the code later on.
  set_extra_data(other_data);

  // Now, we are genuinely done with the setup step.
  metadata->set_training_index_sizes_to_current_column_sizes();

  ////////////////////////////////////////////////////////////

  trained_user_items = make_user_item_lists(train_ml);

  logprogress_stream << "    Data has " << train_ml.size() << " observations with "
                     << metadata->column_size(USER_COLUMN_INDEX)
                     << " users and "
                     << metadata->column_size(ITEM_COLUMN_INDEX)
                     << " items." << std::endl;

  double data_load_time = t.current_time();
  logprogress_stream << "    Data prepared in: " << data_load_time
                     << "s" << std::endl;
  state["data_load_time"] = to_variant(data_load_time);


  // Train using ALS
  if (is_als == true) {

    // Construct the first ml_data instance
    v2::ml_data train_ml_by_item( {
        {"sort_by_first_two_columns_on_train", true},
        {"uniquify_side_column_names", true},
        {"target_column_always_numeric", true},
        {"ignore_new_columns_after_train", true}});

    train_ml_by_item.set_data(data.select_columns(included_columns),
                      target_column,
                      // forced column ordering
                      {item_column, user_column},
                      // Mode overrides -- make sure these are treated this way.
                      { {item_column, v2::ml_column_mode::CATEGORICAL},
                        {user_column, v2::ml_column_mode::CATEGORICAL} } );

    train_ml_by_item.fill();

    t.start();
    std::map<std::string, flexible_type> ret = train(train_ml, train_ml_by_item);
    state.insert(ret.begin(), ret.end());

  // Train the model
  } else {
    t.start();
    std::map<std::string, flexible_type> ret = train(train_ml);
    state.insert(ret.begin(), ret.end());
  }

  double training_time = t.current_time();
  state["training_time"] = training_time;

  // Save information about the dataset
  state["num_observations"]            = to_variant(train_ml.size());
  state["num_users"]                   = to_variant(metadata->column_size(USER_COLUMN_INDEX));
  state["num_items"]                   = to_variant(metadata->column_size(ITEM_COLUMN_INDEX));
  state["num_features"]                = to_variant(metadata->num_columns());
  state["num_user_side_features"]      = to_variant(user_side_data.num_columns());
  state["num_item_side_features"]      = to_variant(item_side_data.num_columns());
  state["observation_data_column_names"] = to_variant(included_columns);
  state["user_side_data_column_names"] = to_variant(user_side_data.column_names());
  state["item_side_data_column_names"] = to_variant(item_side_data.column_names());

  {
    std::vector<flexible_type> user_type_names(user_side_data.num_columns());

    for (size_t i = 0; i < user_side_data.num_columns(); ++i)
      user_type_names[i] = flex_type_enum_to_name(user_side_data.column_type(i));

    state["user_side_data_column_types"] = to_variant(user_type_names);
  }

  {
    std::vector<flexible_type> item_type_names(item_side_data.num_columns());

    for (size_t i = 0; i < item_side_data.num_columns(); ++i)
      item_type_names[i] = flex_type_enum_to_name(item_side_data.column_type(i));

    state["item_side_data_column_types"] = to_variant(item_type_names);
  }

  if (use_target && state.count("training_rmse") == 0) {

    // Calculate the training rmse manually.  given data is in a
    // different order.

    sframe predictions = predict(train_ml);
    std::vector<double> total_se_accumulator(thread::cpu_count(), 0);

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        total_se_accumulator[thread_idx] = 0;

        auto ml_it = train_ml.get_iterator(thread_idx, num_threads);
        parallel_sframe_iterator sf_it(predictions, thread_idx, num_threads);

        for(; !ml_it.done(); ++ml_it, ++sf_it) {
          double diff = sf_it.value(0) - ml_it.target_value();
          total_se_accumulator[thread_idx] += (diff * diff);
        }
      });

    state["training_rmse"] = std::sqrt(std::accumulate(
        total_se_accumulator.begin(), total_se_accumulator.end(), double(0.0)) / train_ml.size());
  }

  return;
}

template <typename K, typename T>
static void sort_and_uniquify_map_of_vecs(std::map<K, std::vector<T> >& data) {

  atomic<size_t> counter = 0;
  auto it_end = data.end();

  in_parallel([&](size_t, size_t) {

      auto it = data.begin();
      size_t local_counter = 0;

      while(it != it_end) {
        size_t next_index = (++counter) - 1;

        while(local_counter < next_index) {
          ++it;
          ++local_counter;
        
          if(it == it_end)
            return;
        }

        auto& vec = it->second;

        std::sort(vec.begin(), vec.end());
        auto it = std::unique(vec.begin(), vec.end());

        vec.resize(it - vec.begin());
      }
    });
}

/**  Extract and index a single categorical column into a vector<size_t>
 *
 */
static std::vector<size_t> extract_categorical_column(
    const std::shared_ptr<v2::ml_data_internal::column_indexer>& indexer,
    const std::shared_ptr<sarray<flexible_type> >& raw_col) {

  DASSERT_TRUE(indexer->mode == v2::ml_column_mode::CATEGORICAL);

  size_t n_elements = raw_col->size();

  std::vector<size_t> out;
  out.resize(raw_col->size());

  indexer->initialize();

  scoped_finally indexer_finalizer;
  indexer_finalizer.add([indexer](){indexer->finalize();});

  auto reader = raw_col->get_reader();

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

      size_t start_idx = (thread_idx * n_elements) / num_threads;
      size_t end_idx = ((thread_idx + 1) * n_elements) / num_threads;

      const size_t block_size = 1024;

      std::vector<flexible_type> v_f;

      for(size_t r_idx = start_idx; r_idx < end_idx; r_idx += block_size) {
        size_t block_end_idx = std::min(end_idx, r_idx + block_size);

#ifndef NDEBUG
        size_t n_back =
#endif
            reader->read_rows(r_idx, block_end_idx, v_f);

        DASSERT_EQ(n_back, block_end_idx - r_idx);

        for(size_t i = 0; i < v_f.size(); ++i) {
          size_t index = indexer->map_value_to_index(thread_idx, v_f[i]);
          out[r_idx + i] = index;
        }
      }
    });

  return out;
}

std::shared_ptr<coreml::MLModelWrapper> recsys_model_base::export_to_coreml(const std::string& filename) {
  flexible_type model_name = name();
  log_and_throw("Currently, only item similarity models can be exported to Core ML (use turicreate.item_similarity.create to make such a model).");
  ASSERT_UNREACHABLE();
}

////////////////////////////////////////////////////////////////////////////////

void recsys_model_base::choose_diversely(size_t top_k,
                                         std::vector<std::pair<size_t, double> >& candidates,
                                         size_t random_seed,
                                         diversity_choice_buffer& dv_buffer) const {

  std::vector<size_t>& current_candidates             = dv_buffer.current_candidates;
  std::vector<size_t>& chosen_items                   = dv_buffer.chosen_items;
  std::vector<size_t>& current_diversity_score        = dv_buffer.current_diversity_score;
  std::vector<std::pair<size_t, double> >& sim_scores = dv_buffer.sim_scores;

  current_candidates.resize(candidates.size());
  current_diversity_score.resize(candidates.size());
  chosen_items.clear();
  chosen_items.reserve(top_k);

  // Copy these over into the
  for(size_t i = 0; i < candidates.size(); ++i) {
    current_candidates[i] = i;
    current_diversity_score[i] = 0;
  }

  size_t K = candidates.size();
  size_t norm_constant = (K * (K + 1)) / 2;
  size_t candidate_rank_multiplier = 1;
  size_t diversity_norm_constant = 0;

  for(size_t k = 0; k < top_k; ++k) {

    auto prob_score = [&](size_t i) -> size_t {
      return candidate_rank_multiplier * (K - current_candidates[i]) + current_diversity_score[i];
    };

    // Choose the sample, then delete it from the candidate list.  The
    // current model has that the probability of a n item being chosen
    // is p(i) = [ k*(K - i) + a_{i1} + a_{i2} + ... + a_{ik}] / Z_k,
    // where k is the current iteration (k = 1, ..., top_k), K is the
    // original number of candidates, and i, with 0 being the most
    // strongly recommended and i = K-1 being the least strongly
    // recommended. a_{ik} is the rank of i against the item chosen on
    // round k.  This ranking is set by adding in the rank of these
    // items in terms of similarity to the just chosen item, with
    // items least similar to that item being given a higher value of
    // a_{ik}.  Thus these are more likely to be chosen later on.
    //
    size_t pick_index = 0;

    size_t Z = candidate_rank_multiplier * norm_constant + diversity_norm_constant;

#ifndef NDEBUG
    {
      ASSERT_EQ(current_diversity_score.size(), current_candidates.size());

      // Make sure we have the right normalizing constant.
      size_t Z_test = 0;
      size_t dv_test = 0;
      for(size_t j = 0; j < current_candidates.size(); ++j) {
        Z_test += prob_score(j);
        dv_test += current_diversity_score[j];
      }

      DASSERT_EQ(diversity_norm_constant, dv_test);
      DASSERT_EQ(Z, Z_test);
    }
#endif

    size_t random_number = hash64(random_seed, k) % Z;

    // Pick the number.
    for(size_t j = 0; j < current_candidates.size(); ++j) {
      size_t s = prob_score(j);

      if(s > random_number) {
        pick_index = j;
        break;
      } else {
        random_number -= s;
      }
    }

    // Take it out of the current candidates.
    std::swap(current_candidates[pick_index], current_candidates.back());
    size_t item = current_candidates.back();
    current_candidates.pop_back();

    // Take it out of the candidate_rank_multiplier set.
    std::swap(current_diversity_score[pick_index], current_diversity_score.back());
    size_t item_diversity_score = current_diversity_score.back();
    current_diversity_score.pop_back();

    chosen_items.push_back(item);

    // Update the normalizing constants
    norm_constant -= (K - item);
    diversity_norm_constant -= item_diversity_score;

    // Add in the new diversity ranks.
    sim_scores.resize(current_candidates.size());

    for(size_t i = 0; i < sim_scores.size(); ++i) {
      sim_scores[i] = {candidates[current_candidates[i]].first, hash64(random_seed, i)};
    }

    // Call the similarity score function for the model in order to
    // make sure that
    get_item_similarity_scores(item, sim_scores);

    // Replace the item indices in sim_scores with the indices that
    // point to the current_candidates set instead of the model item
    // indices needed by get_item_similarity_scores.  This tracks
    // these items through the following sort/ranking.
    for(size_t j = 0; j < sim_scores.size(); ++j) {
      sim_scores[j].first = j;
    }

    // Sort them so we can use the ranks to robustly add in the
    // diversity measure.  This penalizes the items that are the
    // closest to the item just added.
    std::sort(sim_scores.begin(), sim_scores.end(),
              [](const std::pair<size_t, double>& p1, const std::pair<size_t, double>& p2) {
                return p1.second < p2.second;
              });

    // Now, put them in as ranks.
    for(size_t j = 0; j < sim_scores.size(); ++j) {
      current_diversity_score[sim_scores[j].first] += j;
    }

    diversity_norm_constant += (sim_scores.size() * (sim_scores.size() - 1)) / 2;
    candidate_rank_multiplier += 1;
  }

  // Okay, now we have the appropriate items in the candidate set, so
  // copy it back into the chosen items part.

  std::sort(chosen_items.begin(), chosen_items.end());

  for(size_t i = 0; i < chosen_items.size(); ++i) {
    candidates[i] = candidates[chosen_items[i]];
  }
  candidates.resize(chosen_items.size());
}

sframe recsys_model_base::recommend(
    const sframe& query_data,
    size_t top_k,
    const sframe& restriction_data,
    const sframe& exclusion_data,  // Okay, take directly from "exclude"
    const sframe& new_observation_data,
    const sframe& new_user_data,
    const sframe& new_item_data,
    bool exclude_training_interactions,
    double diversity_factor,
    size_t random_seed) const {

  
  const std::string& user_column_name = metadata->column_name(USER_COLUMN_INDEX);
  const std::string& item_column_name = metadata->column_name(ITEM_COLUMN_INDEX);
  
  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up the query data. This is what we'll be iterating
  // over.

  
  // We have three cases here -- all users, a list of users, and an ml_data of observations.
  enum {ALL, LIST, OBSERVATION_ROWS} user_processing_mode;
  
  size_t n_queries;
  
  // Used in LIST mode
  std::vector<size_t> user_query_list;
  
  // Used in OBSERVATION_ROWS mode
  std::unique_ptr<v2::ml_data> query_ml;
  std::vector<size_t> query_column_index_remapping;
  
  // The list of users case
  if(query_data.num_columns() == 0) {
    user_processing_mode = ALL;
  } else if(query_data.num_columns() == 1) {
    user_processing_mode = LIST;
  } else {
    user_processing_mode = OBSERVATION_ROWS;
  }

  switch(user_processing_mode) {

    case ALL: {
      n_queries = metadata->index_size(USER_COLUMN_INDEX);
      // Nothing to be done here.
      break;
    }

    case LIST: {
      // Need to populate the user list
      if(query_data.column_name(0) != user_column_name) {
        log_and_throw("If given, query data for recommend(...) requires a user column.");
      }
  
      user_query_list = extract_categorical_column(metadata->indexer(USER_COLUMN_INDEX),
                                             query_data.select_column(user_column_name));
  
      n_queries = user_query_list.size();
      break;
    }

    case OBSERVATION_ROWS: {

      std::vector<std::string> ref_data_names = query_data.column_names();
      
      if(!query_data.contains_column(user_column_name)) {
        log_and_throw("Query data for recommend(...) requires a user column to be present.");
      }

      if(query_data.contains_column(item_column_name)) {
        log_and_throw("Query data for recommend(...) cannot contain an item column.");
      }


      for(size_t i = 0; i < ref_data_names.size(); ++i) {
        const std::string& cn = ref_data_names[i];

        if(!metadata->contains_column(cn)) {
          log_and_throw( (std::string("Query data contains column ")
                          + cn
                          + ", which was not present at train time.").c_str() );
        }

        if(metadata->is_side_column(cn)) {
          log_and_throw( (std::string("Query data contains column ")
                          + cn
                          + ", which was part of the side data at training time. "
                          + "To use this column to query, use new_user_data or new_item_data.").c_str());
        }
      }

      // Now, rearrange the order of the ref_data_names to most closely
      // match the local order
      std::sort(ref_data_names.begin(), ref_data_names.end(),
                [&](const std::string& c1, const std::string& c2) {
                  return metadata->column_index(c1) < metadata->column_index(c2);
                });
      query_ml.reset(new v2::ml_data(metadata->select_columns(ref_data_names)));
      query_ml->fill(query_data);
  
      // Now, build the column remapping; after select columns, the
      // column indices may be reordered.
      const auto& qml = query_ml->metadata();
  
      query_column_index_remapping.resize(qml->num_columns());
      for(size_t i = 0; i < qml->num_columns(); ++i) {
        query_column_index_remapping[i] = metadata->column_index(qml->column_name(i));
      }

  
      n_queries = query_ml->num_rows();
  
      break;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set up the new observation data and the current side features

  std::shared_ptr<v2::ml_data_side_features> current_side_features;

  // The new user information
  std::map<size_t, std::vector<std::pair<size_t, double> > > new_user_item_lookup;
  std::map<size_t, std::vector<v2::ml_data_row_reference> > new_obs_data_lookup;

  if(new_observation_data.num_rows() > 0
     || new_user_data.num_rows() > 0
     || new_item_data.num_rows() > 0) {

    v2::ml_data new_data = create_ml_data(new_observation_data, new_user_data, new_item_data);

    std::vector<v2::ml_data_entry> x;

    for(auto it = new_data.get_iterator(); !it.done(); ++it) {
      it.fill_observation(x);
      size_t user = x[USER_COLUMN_INDEX].index;
      size_t item = x[ITEM_COLUMN_INDEX].index;
      new_user_item_lookup[user].push_back({item, it.target_value()});
      new_obs_data_lookup[user].push_back(it.get_reference());
    }

    sort_and_uniquify_map_of_vecs(new_user_item_lookup);

    if(new_data.has_side_features())
      current_side_features = new_data.get_side_features();

  } else {

    if(metadata->has_side_features())
      current_side_features = metadata->get_side_features();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the restriction sets

  // May be empty if there are no items to restrict, or if the items
  // are only restricted by user.
  std::vector<size_t> item_restriction_list;

  // May be empty
  std::map<size_t, std::vector<size_t> > item_restriction_list_by_user;

  if(restriction_data.num_rows() > 0) {
    // Restrictions on which sets are okay

    // Read out the item restrictions
    if(restriction_data.num_columns() == 1) {

      if(restriction_data.column_name(0) != item_column_name)
        log_and_throw("Restriction data must be either a single item column or a user, item column.");

      item_restriction_list = extract_categorical_column(
          metadata->indexer(ITEM_COLUMN_INDEX), restriction_data.select_column(0));

      std::sort(item_restriction_list.begin(), item_restriction_list.end());
      auto end_it = std::unique(item_restriction_list.begin(), item_restriction_list.end());

      item_restriction_list.resize(end_it - item_restriction_list.begin());

    } else if(restriction_data.num_columns() == 2) {
      // User - item restrictions.

      if(std::set<std::string>{
          restriction_data.column_name(0),
              restriction_data.column_name(1)}
        != std::set<std::string>{
          metadata->column_name(USER_COLUMN_INDEX),
              metadata->column_name(ITEM_COLUMN_INDEX)}) {

        log_and_throw("If restriction is done by users and items, then both "
                      "user and item columns must be present.");
      }

      std::vector<size_t> users = extract_categorical_column(
          metadata->indexer(USER_COLUMN_INDEX), restriction_data.select_column(user_column_name));

      std::vector<size_t> items = extract_categorical_column(
          metadata->indexer(ITEM_COLUMN_INDEX), restriction_data.select_column(item_column_name));

      DASSERT_EQ(users.size(), items.size());

      for(size_t i = 0; i < users.size(); ++i)
        item_restriction_list_by_user[users[i]].push_back(items[i]);

      sort_and_uniquify_map_of_vecs(item_restriction_list_by_user);

    } else {
      log_and_throw("Currently, restriction data must be either items or and sframe of user/item pairs.");
    }
  }

  /// Some constants used in the code
  static constexpr double neg_inf = std::numeric_limits<double>::lowest();
  const size_t max_n_threads = thread::cpu_count();

  ////////////////////////////////////////////////////////////////////////////////
  // Set up the query size for the recommender.

  if(diversity_factor < 0)
    log_and_throw("Diversity factor must be greater than or equal to 0.");

  size_t top_k_query_number = size_t(round(top_k * (1 + diversity_factor)));
  bool enable_diversity = (top_k_query_number != top_k);

  std::vector<diversity_choice_buffer> dv_buffers;

  if(enable_diversity) {
    dv_buffers.resize(max_n_threads);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up the lookup tables for the user_item pairs on the
  // new data and the exclusion lists.  In memory for now, as we
  // expect these to be small.

  std::map<size_t, std::vector<size_t> > exclusion_lists;

  if(exclusion_data.num_columns() != 0) {

    // User - item restrictions.
    if(!exclusion_data.contains_column(user_column_name)
       || ! exclusion_data.contains_column(item_column_name)) {

      log_and_throw("Exclusion SFrame must have both user and item columns.");
    }

    std::vector<size_t> users = extract_categorical_column(
        metadata->indexer(USER_COLUMN_INDEX), exclusion_data.select_column(user_column_name));

    std::vector<size_t> items = extract_categorical_column(
        metadata->indexer(ITEM_COLUMN_INDEX), exclusion_data.select_column(item_column_name));

    DASSERT_EQ(users.size(), items.size());

    for(size_t i = 0; i < users.size(); ++i)
      exclusion_lists[users[i]].push_back(items[i]);

    sort_and_uniquify_map_of_vecs(exclusion_lists);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up the lookup tables for the user_item pairs on the
  // new data and the exclusion lists.  In memory for now, as we
  // expect these to be small.

  ////////////////////////////////////////////////////////////////////////////////
  // set up a reference vector that we use to populate the set of
  // scores sent in to the score_all_items function.



  typedef std::pair<size_t, double> item_score_pair;

  ////////////////////////////////////////////////////////////////////////////////
  // Iterate through the query data

  // Init a reader for the users
  auto trained_user_items_reader = trained_user_items->get_reader();

  atomic<size_t> n_queries_processed;

  // create the output container for the rank items
  std::vector<std::string> column_names = {
    metadata->column_name(USER_COLUMN_INDEX),
    metadata->column_name(ITEM_COLUMN_INDEX),
    "score", "rank"};

  // These types are indexed, they will be mapped back later
  std::vector<flex_type_enum> column_types = {
    metadata->column_type(USER_COLUMN_INDEX),
    metadata->column_type(ITEM_COLUMN_INDEX),
    flex_type_enum::FLOAT, flex_type_enum::INTEGER};

  const size_t num_segments = max_n_threads;

  sframe ret;
  ret.open_for_write(column_names, column_types, "", num_segments);

  timer log_timer;
  log_timer.start();

  const std::vector<size_t> empty_vector;
  const std::vector<std::pair<size_t, double> > empty_pair_vector;
  const std::vector<v2::ml_data_row_reference> empty_ref_vector;

  auto _run_recommendations = [&](size_t thread_idx, size_t n_threads)
    GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

      std::vector<item_score_pair> item_score_list;
      item_score_list.reserve(metadata->index_size(ITEM_COLUMN_INDEX));

      std::vector<std::vector<std::pair<size_t, double> > > user_item_lists;

      auto out = ret.get_output_iterator(thread_idx);
      std::vector<flexible_type> out_x_v;
      std::vector<v2::ml_data_entry> query_x;

      std::unique_ptr<v2::ml_data_iterator> it_ptr;

      ////////////////////////////////////////////////////////////
      // Setup stuff:

      size_t n_users          = size_t(-1);
      size_t user_index       = size_t(-1);
      size_t user_index_start = size_t(-1);
      size_t user_index_end   = size_t(-1);

      switch(user_processing_mode) {
        case ALL: {
          n_users = metadata->index_size(USER_COLUMN_INDEX);
          user_index_start = (thread_idx * n_users) / n_threads;
          user_index_end   = ((thread_idx+1) * n_users) / n_threads;
          user_index = user_index_start;
          break;
        }
        case LIST: {
          n_users = user_query_list.size();
          user_index_start = (thread_idx * n_users) / n_threads;
          user_index_end   = ((thread_idx+1) * n_users) / n_threads;
          user_index = user_index_start;
          break;
        }
        case OBSERVATION_ROWS: {
          it_ptr.reset(new v2::ml_data_iterator(query_ml->get_iterator(thread_idx, n_threads)));
          break;
        }
      }

      while(true) {

        size_t user;
        uint64_t user_hash_key = 0;

        bool done_flag = false;

        switch(user_processing_mode) {
          case ALL: {
            if(user_index == user_index_end) {
              done_flag = true;
              break;
            }

            query_x = {v2::ml_data_entry{USER_COLUMN_INDEX, user_index, 1.0},
                       v2::ml_data_entry{ITEM_COLUMN_INDEX, 0, 1.0} };

            if(current_side_features != nullptr)
              current_side_features->add_partial_side_features_to_row(
                  query_x, USER_COLUMN_INDEX, user_index);

            user = user_index;
            user_hash_key = user;
            break;
          }

          case LIST: {

            if(user_index == user_index_end) {
              done_flag = true;
              break;
            }

            user = user_query_list[user_index];

            query_x = {v2::ml_data_entry{USER_COLUMN_INDEX, user, 1.0},
                       v2::ml_data_entry{ITEM_COLUMN_INDEX, 0, 1.0} };

            user_hash_key = user;

            if(current_side_features != nullptr)
              current_side_features->add_partial_side_features_to_row(
                  query_x, USER_COLUMN_INDEX, user);
            break;
          }

          case OBSERVATION_ROWS: {
            DASSERT_TRUE(it_ptr != nullptr);

            if(it_ptr->done()) {
              done_flag = true;
              break;
            }

            it_ptr->fill_observation(query_x);
            DASSERT_EQ(query_x[0].column_index, 0);

            user = query_x[0].index;
            user_hash_key = user;

            // Now insert an empty ITEM column index vector.
            query_x.insert(query_x.begin() + 1, v2::ml_data_entry{ITEM_COLUMN_INDEX, 0, 1.0});

            // Now, need to go through and adjust the columns of the
            // query_x to match those of the original data.

            for(size_t i = 2; i < query_x.size(); ++i) {
              v2::ml_data_entry& qe = query_x[i];
              qe.column_index = query_column_index_remapping[qe.column_index];
            }

            user_hash_key = hash64( (const char*)(query_x.data()), sizeof(v2::ml_data_entry)*query_x.size());
            break;
          }

          default: {
            log_and_throw("Unsupported value for user_processing_mode");
            ASSERT_UNREACHABLE();
          }
        }

        if(done_flag)
          break;

        // Get the additional data, if present
        auto nil_it = new_user_item_lookup.find(user);
        const std::vector<std::pair<size_t, double> >& new_user_item_list =
            (nil_it == new_user_item_lookup.end()
             ? empty_pair_vector
             : nil_it->second);

        // Get the additional exclusion lists, as needed
        auto exc_it = exclusion_lists.find(user);
        const std::vector<size_t>& excl_list =
            (exc_it == exclusion_lists.end()
             ? empty_vector
             : exc_it->second);

        // Read in the next row from the user-item data the model was
        // trained on.  This will also be used for excluding stuff.
        size_t rows_read_for_user = trained_user_items_reader->read_rows(user, user + 1, user_item_lists);

        const std::vector<std::pair<size_t, double> >& user_items =
            (rows_read_for_user > 0 ? user_item_lists.front() : empty_pair_vector);

        // Add in all the scores that are not in the exclusion list
        item_score_list.clear();

        auto train_it = user_items.cbegin();
        const auto& train_it_end = user_items.cend();

        auto new_data_it = new_user_item_list.cbegin();
        const auto& new_data_it_end = new_user_item_list.cend();

        auto exclude_it = excl_list.cbegin();
        const auto& exclude_it_end = excl_list.cend();

        auto check_item_okay_and_advance_iters = [&](size_t item) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
          // Check explicit exclusion list.
          if(exclude_it != exclude_it_end && *exclude_it < item)
            ++exclude_it;

          if(exclude_it != exclude_it_end && *exclude_it < item) {
            do {
              ++exclude_it;
            } while(exclude_it != exclude_it_end && *exclude_it < item);
          }

          if(exclude_it != exclude_it_end && *exclude_it == item)
            return false;

          if(!exclude_training_interactions)
            return true;

          // Check the training stuff
          if(train_it != train_it_end && train_it->first < item)
            ++train_it;

          if(train_it != train_it_end && train_it->first < item) {
            do {
              ++train_it;
            } while(train_it != train_it_end && train_it->first < item);
          }

          if(train_it != train_it_end && train_it->first == item)
            return false;

          // Check new data list
          if(new_data_it != new_data_it_end && new_data_it->first < item)
            ++new_data_it;

          if(new_data_it != new_data_it_end && new_data_it->first < item) {
            do {
              ++new_data_it;
            } while(new_data_it != new_data_it_end && new_data_it->first < item);
          }

          if(new_data_it != new_data_it_end && new_data_it->first == item)
            return false;

          return true; 
        };

        if(!item_restriction_list.empty()) {
          DASSERT_TRUE(item_restriction_list_by_user.empty());

          size_t idx = 0;
          item_score_list.resize(item_restriction_list.size());

          for(size_t item : item_restriction_list) {
            if(check_item_okay_and_advance_iters(item))
              item_score_list[idx++] = {item, neg_inf};
          }

          item_score_list.resize(idx);

        } else if(!item_restriction_list_by_user.empty()) {

          auto it = item_restriction_list_by_user.find(user);

          if(it != item_restriction_list_by_user.end()) {

            const std::vector<size_t>& irl = it->second;

            size_t idx = 0;
            item_score_list.resize(irl.size());

            for(size_t item : irl) {
              if(check_item_okay_and_advance_iters(item))
                item_score_list[idx++] = {item, neg_inf};
            }

            item_score_list.resize(idx);

          } else {
            item_score_list.clear();
          }

        } else {
          const size_t n_items = metadata->column_size(ITEM_COLUMN_INDEX);

          size_t idx = 0;
          item_score_list.resize(n_items);

          for(size_t item = 0; item < n_items; ++item) {
            if(check_item_okay_and_advance_iters(item))
              item_score_list[idx++] = {item, neg_inf};
          }

          item_score_list.resize(idx);
        }

        // Only do this if we need to; although that's most of the time.
        if(LIKELY(!item_score_list.empty())) {

          auto new_obs_data_lookup_it = new_obs_data_lookup.find(user);

          const auto& new_obs_data_vec = (new_obs_data_lookup_it == new_obs_data_lookup.end()
                                          ? empty_ref_vector
                                          : new_obs_data_lookup_it->second);

          // Score all the items
          score_all_items(item_score_list,
                          query_x,
                          top_k_query_number,
                          user_items,
                          new_user_item_list,
                          new_obs_data_vec,
                          current_side_features);

          size_t n_qk = std::min(top_k_query_number, item_score_list.size());
          size_t n_k = std::min(top_k, item_score_list.size());

          // Sort and get the top_k.
          auto score_sorter = [](const item_score_pair& vi1, const item_score_pair& vi2) {
            return vi1.second < vi2.second;
          };

          extract_and_sort_top_k(item_score_list, n_qk, score_sorter);

          if(enable_diversity && n_qk > n_k) {
            choose_diversely(n_k, item_score_list, hash64(random_seed,user_hash_key), dv_buffers[thread_idx]);

            DASSERT_EQ(item_score_list.size(), n_k);
          }

          // now append them all to the output sframes
          for(size_t i = 0; i < n_k; ++i, ++out) {
            size_t item = item_score_list[i].first;
            double score = item_score_list[i].second;
            out_x_v = {metadata->indexer(USER_COLUMN_INDEX)->map_index_to_value(user),
                       metadata->indexer(ITEM_COLUMN_INDEX)->map_index_to_value(item),
                       score,
                       i + 1};

            *out = out_x_v;
          }
        }

        size_t cur_n_queries_processed = (++n_queries_processed);

        if(cur_n_queries_processed % 1000 == 0) {
          logprogress_stream << "recommendations finished on "
                             << cur_n_queries_processed << "/" << n_queries << " queries."
                             << " users per second: "
                             << double(cur_n_queries_processed) / log_timer.current_time()
                             << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Now, do the incrementation

        switch(user_processing_mode) {
          case LIST:
          case ALL:
            ++user_index;
            break;
          case OBSERVATION_ROWS:
            DASSERT_TRUE(it_ptr != nullptr);
            ++(*it_ptr);
            break;
        }
      }
  };

  // Conditionally run the recommendations based on the number of
  // threads.  If we don't run it in parallel here, it allows lower
  // level algorithms to be parallel.
  if(n_queries < max_n_threads) {
    _run_recommendations(0, 1);
  } else {
    in_parallel(_run_recommendations);
  }

  ret.close();

  return ret;
}


// This is a hack to have sframe cross over to python. In the future, the extensions mechanism 
// should do this automatically and we shouldn't have to write this workaround. 
std::shared_ptr<unity_sframe_base> recsys_model_base::recommend_extension_wrapper(
  std::shared_ptr<unity_sframe_base> reference_data,
  std::shared_ptr<unity_sframe_base> new_observation_data,
  flex_int top_k) const {

  std::shared_ptr<unity_sframe> usframe_refdata =
              std::dynamic_pointer_cast<unity_sframe> (reference_data);
  const sframe& outputSFrame = this->recommend(
    *(std::dynamic_pointer_cast<unity_sframe>(reference_data)->get_underlying_sframe()),
    top_k,
    sframe(), // restriction_data
    sframe(), // exclusion_data
    *(std::dynamic_pointer_cast<unity_sframe>(new_observation_data)->get_underlying_sframe())
  );
  std::shared_ptr<unity_sframe> usframe = std::make_shared<unity_sframe>();
  usframe->construct_from_sframe(outputSFrame);
  
  return usframe;

}

// This is a hack to have sframe cross over to python. In the future, the extensions mechanism 
// should do this automatically and we shouldn't have to write this workaround. 
std::shared_ptr<unity_sframe_base> recsys_model_base::get_num_users_per_item_extension_wrapper(
  ) const {

  const sframe& outputSFrame = this->get_num_users_per_item();
  std::shared_ptr<unity_sframe> usframe = std::make_shared<unity_sframe>();
  usframe->construct_from_sframe(outputSFrame);
  
  return usframe;

}

// This is a hack to have sframe cross over to python. In the future, the extensions mechanism 
// should do this automatically and we shouldn't have to write this workaround. 
std::shared_ptr<unity_sframe_base> recsys_model_base::get_num_items_per_user_extension_wrapper(
  ) const {

  const sframe& outputSFrame = this->get_num_items_per_user();
  std::shared_ptr<unity_sframe> usframe = std::make_shared<unity_sframe>();
  usframe->construct_from_sframe(outputSFrame);
  
  return usframe;

}

////////////////////////////////////////////////////////////////////////////////

sframe recsys_model_base::precision_recall_stats(
    const sframe& indexed_validation_data,
    const sframe& recommend_output,
    const std::vector<size_t>& cutoffs) const {

  turi::timer timer;
  timer.start();

  // TODO: Redo this function!!! There are tons of in-memory sections.
  std::vector<size_t> users = get_unique_values(indexed_validation_data.select_column(USER_COLUMN_INDEX));

  // should preserve the order
  indexed_column_groupby pred_ranks(recommend_output.select_column(metadata->column_name(USER_COLUMN_INDEX)),
                                    recommend_output.select_column(metadata->column_name(ITEM_COLUMN_INDEX)),
                                    false, false);

  indexed_column_groupby val_ranks(indexed_validation_data.select_column(USER_COLUMN_INDEX),
                                   indexed_validation_data.select_column(ITEM_COLUMN_INDEX),
                                   false, false);

  sframe ret;
  ret.open_for_write(
      {metadata->column_name(USER_COLUMN_INDEX),
          "cutoff", "precision", "recall", "count"},
      {flex_type_enum::INTEGER,
            flex_type_enum::INTEGER,
            flex_type_enum::FLOAT,
            flex_type_enum::FLOAT,
            flex_type_enum::INTEGER});

  size_t num_segments = ret.num_segments();


  std::vector<flexible_type> out_v;

  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    size_t start_idx = (sidx * users.size()) / num_segments;
    size_t end_idx   = ((sidx+1) * users.size()) / num_segments;

    auto it_out = ret.get_output_iterator(sidx);

    for(size_t i = start_idx; i < end_idx; ++i) {

      const std::vector<size_t>& vr = val_ranks.dest_group(users[i]);
      const std::vector<size_t>& pr = pred_ranks.dest_group(users[i]);

      const std::vector<std::pair<double, double> > prv =
          turi::recsys::precision_and_recall(vr, pr, cutoffs);

      for(size_t j = 0; j < cutoffs.size(); ++j, ++it_out) {
        out_v = {users[i], cutoffs[j], prv[j].first, prv[j].second, vr.size()};
        *it_out = out_v;
      }
    }
  }

  ret.close();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

/// serialization -- save
void recsys_model_base::save_impl(turi::oarchive& oarc) const {
  // Write state
  variant_deep_save(state, oarc);

  oarc << options
       << metadata;

  if (oarc.dir) {
    // if no dir_archive available, skip writing user data
    // the load then must take place with no dir_archive available
    // (symmetric save/load only with respect to presence of dir_archive)
    oarc << *trained_user_items;
  } else {
    // write trained_user_items as a vector<vector<pair<size_t, double>>> instead of sarray
    // so we don't require a dir_archive
    std::vector<std::vector<std::pair<size_t, double> > > temp_trained_user_items;
    auto reader = trained_user_items->get_reader();
    for (size_t i=0; i<trained_user_items->num_segments(); i++) {
        auto iter = reader->begin(i);
        auto end = reader->end(i);
        while (iter != end) {
          temp_trained_user_items.push_back(*iter);
          ++iter;
        }
    }
    oarc << temp_trained_user_items;
  }

  // save the file version
  oarc << internal_get_version();

  internal_save(oarc);
}

////////////////////////////////////////////////////////////////////////////////

/// serialization -- load
void recsys_model_base::load_version(turi::iarchive& iarc, size_t version) {

  // We are backwards compatible with only the previous version
  std::stringstream ss;
  ss << "Unable to load model. Only models after turicreate 1.0 can be loaded."
     << " Please re-train your model and re-save.";
  if (version == 0){
    log_and_throw(ss.str());
  }

  // Read from the state variable
  variant_deep_load(state, iarc);

  iarc >> options
       >> metadata;

  // Now, if there have been any additional options added since this
  // model was saved, go through and add those to the option manager.

  // if no dir_archive, skip attempting to load user data
  // (see comments in save_impl)
  if (iarc.dir) {
    if (version == 1) {
      // Version 1 had the base type stored as a flex_dict so
      // item_similarity could use it in a graph, but for speed version
      // 2+ stores it as vector of index, double pairs.  Thus we need to
      // convert that here.

      auto tmp = std::make_shared<sarray<flex_dict> >();
      iarc >> *tmp;

      trained_user_items.reset(new sarray<std::vector<std::pair<size_t, double> > >);

      size_t n = tmp->size();

      // Convert everything over
      size_t max_num_threads = thread::cpu_count();
      trained_user_items->open_for_write(max_num_threads);
      auto reader = tmp->get_reader(max_num_threads);

      in_parallel([&](size_t thread_idx, size_t num_threads) {
          size_t start_idx = (thread_idx * n) / num_threads;
          size_t end_idx = ((thread_idx+1) * n) / num_threads;

          auto it_out = trained_user_items->get_output_iterator(thread_idx);

          std::vector<flex_dict> row_buf_v;
          std::vector<std::pair<size_t, double> > out;

          for(size_t i = start_idx; i < end_idx; ++i, ++it_out) {
            reader->read_rows(i, i+1, row_buf_v);

            const auto& row = row_buf_v[0];
            out.assign(row.begin(), row.end());
            *it_out = out;
          }
        });

      trained_user_items->close();

    } else {
      trained_user_items.reset(new sarray<std::vector<std::pair<size_t, double> > >);
      iarc >> *trained_user_items;
    }
  } else {
    // no dir_archive - read from an std::vector instead of sarray
    std::vector<std::vector<std::pair<size_t, double> > > temp_trained_user_items;
    iarc >> temp_trained_user_items;
    trained_user_items.reset(new sarray<std::vector<std::pair<size_t, double> > >);
    trained_user_items->open_for_write();
    auto iter = trained_user_items->get_output_iterator(0);
    for (const auto& val : temp_trained_user_items) {
      (*iter) = val;
      ++iter;
    }
    trained_user_items->close();
  }

  size_t internal_version;
  iarc >> internal_version;
  internal_load(iarc, internal_version);
}

  /** Some of the models, such as popularity, can be built entirely
   *  from data already contained in the model.  This method allows us
   *  to create a new model while bypassing the typical
   *  setup_and_train method.  This simply imports all the relevant
   *  variables over; the final training is left up to the model.
   */
void recsys_model_base::import_all_from_other_model(const recsys_model_base* other) {
  recsys_model_base::operator=(*other);
}

std::shared_ptr<recsys_model_base> recsys_model_base::get_popularity_baseline() const {

  std::shared_ptr<recsys_popularity> pop(new recsys_popularity);

  pop->import_all_from_other_model(this);
  pop->train(trained_user_items);

  return pop;
}

////////////////////////////////////////////////////////////////////////////////

flex_dict recsys_model_base::get_data_schema() const {

  size_t n = metadata->num_columns();
  
  flex_dict schema(n);

  for(size_t i = 0; i < n; ++i) {
    schema[i] = {metadata->column_name(i),
                 flex_type_enum_to_name(metadata->column_type(i))};
  }

  return schema; 
}

////////////////////////////////////////////////////////////////////////////////

/// return stats about algorithm runtime, etc.
std::map<std::string, flexible_type> recsys_model_base::get_train_stats() {
  auto ret = std::map<std::string, flexible_type>();
  std::vector<std::string> keys = {"training_time", "training_rmse"};
  for (auto k : keys) {
    if (state.count(k) != 0) {
      ret[k] = safe_varmap_get<flexible_type>(state, k);
    }
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////


/**
 *  Returns information about all the users in the overlap of the
 *  item pairs listed in two columns in unindexed_item_pairs.  All
 *  these items must be present in the training data.
 *
 *  Returns an sframe with information about this
 *  intersection. Columns are item_1, item_2, num_users_1,
 *  num_users_2, item_intersection (dict, user -> (rating_1, rating_2)
 *
 *  For each user, we iterate through the (item, rating) pairs in
 *  trained_user_items. Then, for each (item_a, item_b) in each
 *  per-user set of items that is also in the provided list of item
 *  pairs, we register that user in the intersection list for that
 *  entry.
 */
sframe recsys_model_base::get_item_intersection_info(const sframe& unindexed_item_pairs) const {

  if(unindexed_item_pairs.num_columns() != 2
     || unindexed_item_pairs.column_type(0) != metadata->column_type(ITEM_COLUMN_INDEX)
     || unindexed_item_pairs.column_type(1) != metadata->column_type(ITEM_COLUMN_INDEX)) {
    log_and_throw("Provided list of item pairs must be 2-column "
                  "sframe with each column containing an item.");
  }
  
  struct item_data {
    size_t row_index;
    size_t item_1, item_2;
    size_t n_users_item_1 = 0;
    size_t n_users_item_2 = 0;
    std::vector<std::pair<size_t, std::pair<double, double> > > users_in_intersection;
    simple_spinlock access_lock;
  };

  auto user_indexer = metadata->indexer(USER_COLUMN_INDEX);
  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);
  auto item_statistics = metadata->statistics(ITEM_COLUMN_INDEX);

  // A lookup
  std::vector<item_data> item_outputs(unindexed_item_pairs.size());

  std::set<size_t> all_items_considered;

  size_t pos = 0;
  for(parallel_sframe_iterator it(unindexed_item_pairs); !it.done(); ++it, ++pos) {
    item_data& idata = item_outputs[pos];

    idata.row_index = pos;
    idata.item_1 = item_indexer->immutable_map_value_to_index(it.value(0));
    idata.item_2 = item_indexer->immutable_map_value_to_index(it.value(1));

    idata.n_users_item_1 = item_statistics->count(idata.item_1);
    idata.n_users_item_2 = item_statistics->count(idata.item_2);

    if(idata.item_1 != size_t(-1)) { 
      all_items_considered.insert(idata.item_1);
    }
    
    if(idata.item_2 != size_t(-1)) { 
      all_items_considered.insert(idata.item_2);
    }
  }

  // Sort as we iterate through this below assuming it's in the same
  // order as iterating through the sorted per-user item list
  std::sort(item_outputs.begin(), item_outputs.end(),
            [](const item_data& id1, const item_data& id2) {
              return std::tie(id1.item_1, id1.item_2) < std::tie(id2.item_1, id2.item_2);
            });

  auto reader = trained_user_items->get_reader();

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      size_t start_user = (thread_idx * trained_user_items->size()) / num_threads;
      size_t end_user = ((thread_idx+1) * trained_user_items->size()) / num_threads;

      std::vector<std::vector<std::pair<size_t, double> > > user_row;

      for(size_t user = start_user; user < end_user; ++user) {
        reader->read_rows(user, user+1, user_row);

        auto& user_item_list = user_row[0];

        // Filter out all the items we don't care about.
        auto last_it = std::remove_if(
            user_item_list.begin(), user_item_list.end(),
            [&](const std::pair<size_t, double>& p) {
              return all_items_considered.find(p.first) == all_items_considered.end();
            });
        user_item_list.resize(last_it - user_item_list.begin());

        // The items in the row are in sorted order, so use that in
        // adding in the item_outputs.
        auto iout_it = item_outputs.begin();

        for(const auto& p1 : user_item_list) {

          size_t item_1 = p1.first;
          double score_1 = p1.second;

          for(const auto& p2 : user_item_list) {
            size_t item_2 = p2.first;
            double score_2 = p2.second;

            // Find the next item in the list of item_outputs that match this item pair. 
            auto it = std::lower_bound(
                iout_it, item_outputs.end(), std::make_pair(item_1, item_2),
                [](const item_data& idata, const std::pair<size_t, size_t>& p) {
                  return std::make_pair(idata.item_1, idata.item_2) < p;
                });

            if(it == item_outputs.end() || it->item_1 != item_1 || it->item_2 != item_2)
              continue;

            // Move the base ahead for speed
            iout_it = it;

            {
              std::lock_guard<simple_spinlock> lg(it->access_lock);
              it->users_in_intersection.push_back({user, {score_1, score_2}});
            }
          }
        }
      }
    });

  // Now, go through and put all the answers into an sframe in the
  // orginal order. 
  std::sort(item_outputs.begin(), item_outputs.end(),
            [](const item_data& id1, const item_data& id2) {
              return id1.row_index < id2.row_index;
            });

  sframe out_data_1 = unindexed_item_pairs;
  
  sframe out_data_2 = sframe_from_ranged_generator(
      {"num_users_1", "num_users_2", "intersection"},
      {flex_type_enum::INTEGER, flex_type_enum::INTEGER, flex_type_enum::DICT},
      item_outputs.size(),
      [&](size_t idx, std::vector<flexible_type>& out) {
        const item_data& idata = item_outputs[idx];
        flex_dict fd(idata.users_in_intersection.size());

        for(size_t i = 0; i < idata.users_in_intersection.size(); ++i) {
          const auto& p = idata.users_in_intersection[i]; 
          fd[i] = {user_indexer->map_index_to_value(p.first),
                   flex_list{flexible_type(p.second.first), flexible_type(p.second.second)}};
        }
        
        out = {idata.n_users_item_1,
               idata.n_users_item_2,
               std::move(fd)};
      });

  // Now join with the original columns.
  for(size_t i : {0, 1, 2}) {
    out_data_1 = out_data_1.add_column(out_data_2.select_column(i), out_data_2.column_name(i));
  }

  return out_data_1;
}

gl_sframe recsys_model_base::api_get_item_intersection_info(gl_sframe item_pairs) {

  sframe item_info = this->get_item_intersection_info(item_pairs.materialize_to_sframe());

  return gl_sframe(item_info);
}

sframe recsys_model_base::get_num_items_per_user() const {

  size_t num_users = metadata->index_size(USER_COLUMN_INDEX);

  std::vector<std::string> column_names = {
    metadata->column_name(USER_COLUMN_INDEX),
    "num_items"
  };

  std::vector<flex_type_enum> column_types = {
    metadata->column_type(USER_COLUMN_INDEX),
    flex_type_enum::INTEGER
  };

  size_t num_segments = 1;

  sframe ret;
  ret.open_for_write(column_names, column_types, "", num_segments);

  std::vector<flexible_type> out_v;
  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    size_t start_idx = (sidx * num_users) / num_segments;
    size_t end_idx   = ((sidx+1) * num_users) / num_segments;

    auto it_out = ret.get_output_iterator(sidx);
    for(size_t i = start_idx; i < end_idx; ++i) {
      out_v = {
        metadata->indexer(USER_COLUMN_INDEX)->map_index_to_value(i),
        metadata->statistics(USER_COLUMN_INDEX)->count(i)
      };
      *it_out = out_v;
    }
  }
  ret.close();

  return ret;
}


sframe recsys_model_base::get_num_users_per_item() const {

  size_t num_items = metadata->index_size(ITEM_COLUMN_INDEX);

  std::vector<std::string> column_names = {
    metadata->column_name(ITEM_COLUMN_INDEX),
    "num_users"
  };

  std::vector<flex_type_enum> column_types = {
    metadata->column_type(ITEM_COLUMN_INDEX),
    flex_type_enum::INTEGER
  };

  size_t num_segments = 1;

  sframe ret;
  ret.open_for_write(column_names, column_types, "", num_segments);

  std::vector<flexible_type> out_v;
  for(size_t sidx = 0; sidx < num_segments; ++sidx) {

    size_t start_idx = (sidx * num_items) / num_segments;
    size_t end_idx   = ((sidx+1) * num_items) / num_segments;

    auto it_out = ret.get_output_iterator(sidx);
    for(size_t i = start_idx; i < end_idx; ++i) {
      out_v = {
        metadata->indexer(ITEM_COLUMN_INDEX)->map_index_to_value(i),
        metadata->statistics(ITEM_COLUMN_INDEX)->count(i)
      };
      *it_out = out_v;
    }
  }
  ret.close();

  return ret;
}

gl_sframe recsys_model_base::api_get_similar_items(gl_sarray items, size_t k, size_t verbose, int get_all_items) const {

  turi::timer timer;

  auto items_sa = items.materialize_to_sarray();

  if(get_all_items) {
    items_sa.reset();
  }

  timer.start();

  sframe raw_ranks = this->get_similar_items(items_sa, k);

  if (verbose) {
    logprogress_stream << "Getting similar items completed in "
                       << timer.current_time() << "" << std::endl;
  }

  return gl_sframe(raw_ranks);
}



gl_sframe recsys_model_base::api_get_similar_users(gl_sarray users, size_t k, int get_all_users) const {


  turi::timer timer;

  auto users_sa = users.materialize_to_sarray();

  if(get_all_users) {
    users_sa.reset();
  }

  timer.start();

  sframe raw_ranks = this->get_similar_users(users_sa, k);

  logprogress_stream << "Getting similar users completed in "
      << timer.current_time() << "" << std::endl;

  return gl_sframe(raw_ranks);
}

gl_sframe recsys_model_base::api_predict(gl_sframe data_to_predict, gl_sframe new_user_data, gl_sframe new_item_data) const {

  sframe sf = data_to_predict.materialize_to_sframe();

  // Currently, new_data is ignored, as none of the models use it.

  sframe new_user_data_sf = new_user_data.materialize_to_sframe();
  sframe new_item_data_sf = new_item_data.materialize_to_sframe();

  sframe predictions = this->predict(this->create_ml_data(sf, new_user_data_sf, new_item_data_sf));

  return predictions;
}

variant_map_type recsys_model_base::api_get_current_options() {

  std::map<std::string, flexible_type> options = this->get_current_options();

  variant_map_type ret;
  for (auto& opt : options) {
    ret[opt.first] = opt.second;
  }
  return ret;
}


variant_map_type recsys_model_base::api_set_current_options(std::map<std::string, flexible_type> options) {

  options.erase("model");
  this->set_options(options);

  return variant_map_type();
}

void recsys_model_base::api_train(
    gl_sframe _dataset, gl_sframe _user_data, gl_sframe _item_data,
    const std::map<std::string, flexible_type>& opts,
    const variant_map_type& extra_data) {

  sframe dataset = _dataset.materialize_to_sframe();
  sframe user_data = _user_data.materialize_to_sframe();
  sframe item_data = _item_data.materialize_to_sframe();

  this->init_options(opts);
  this->setup_and_train(dataset, user_data, item_data, extra_data);

}


gl_sframe recsys_model_base::api_recommend(gl_sframe _query, gl_sframe _exclude, gl_sframe _restrictions, gl_sframe _new_data, gl_sframe _new_user_data,
  gl_sframe _new_item_data, bool exclude_training_interactions, size_t top_k, double diversity, size_t random_seed) {

  turi::timer timer;

  sframe query_sf = _query.materialize_to_sframe();
  sframe exclusion_data_sf = _exclude.materialize_to_sframe();
  sframe restrictions_sf = _restrictions.materialize_to_sframe();
  sframe new_observation_data_sf = _new_data.materialize_to_sframe();
  sframe new_user_data_sf = _new_user_data.materialize_to_sframe();
  sframe new_item_data_sf = _new_item_data.materialize_to_sframe();

  timer.start();

  // Rank items
  sframe ranks = this->recommend(query_sf,
                              top_k,
                              restrictions_sf,
                              exclusion_data_sf,
                              new_observation_data_sf,
                              new_user_data_sf,
                              new_item_data_sf,
                              exclude_training_interactions,
                              diversity,
                              random_seed);

  logstream(LOG_INFO) << "Ranking completed in " << timer.current_time() << std::endl;

  return gl_sframe(ranks);

}

gl_sframe recsys_model_base::api_precision_recall_by_user(
    gl_sframe validation_data, 
    gl_sframe recommend_output,
    const std::vector<size_t>& cutoffs) {

  const std::string& user_col = metadata->column_name(USER_COLUMN_INDEX);
  const std::string& item_col = metadata->column_name(ITEM_COLUMN_INDEX);

  validation_data[user_col] = validation_data[user_col].apply(
      metadata->indexer(USER_COLUMN_INDEX)->indexing_lambda(), flex_type_enum::INTEGER);
  validation_data[item_col] = validation_data[item_col].apply(
      metadata->indexer(ITEM_COLUMN_INDEX)->indexing_lambda(), flex_type_enum::INTEGER);

  recommend_output[user_col] = recommend_output[user_col].apply(
      metadata->indexer(USER_COLUMN_INDEX)->indexing_lambda(), flex_type_enum::INTEGER);
  recommend_output[item_col] = recommend_output[item_col].apply(
      metadata->indexer(ITEM_COLUMN_INDEX)->indexing_lambda(), flex_type_enum::INTEGER);

  gl_sframe stats = gl_sframe(precision_recall_stats(
        validation_data.materialize_to_sframe(),
        recommend_output.materialize_to_sframe(), cutoffs));

  stats[user_col] = stats[user_col].apply(
     metadata->indexer(USER_COLUMN_INDEX)->deindexing_lambda(), 
     metadata->column_type(USER_COLUMN_INDEX));

  stats.materialize();

  return stats;
}

variant_map_type recsys_model_base::api_get_data_schema() {

  variant_map_type ret;
  ret["schema"] = this->get_data_schema();

  return ret;
}

variant_map_type recsys_model_base::summary() {

  variant_map_type ret;
  for (auto& opt : this->get_current_options()) {
    ret[opt.first] = opt.second;
  }
  for (auto& opt : this->get_train_stats()) {
    ret[opt.first] = opt.second;
  }

  return ret;
}

EXPORT variant_map_type train_test_split(gl_sframe _dataset,
                                         const std::string& user_column,
                                         const std::string& item_column,
                                         flexible_type max_num_users,
                                         double item_test_proportion,
                                         size_t random_seed) {
  variant_map_type ret;
  sframe dataset = _dataset.materialize_to_sframe();
  size_t max_users = (max_num_users == FLEX_UNDEFINED) ? std::numeric_limits<size_t>::max() : size_t(max_num_users);

  auto train_test = make_recsys_train_test_split(dataset, user_column, item_column,
                                                 max_users,
                                                 item_test_proportion,
                                                 random_seed);

  ret["train"] = to_variant(gl_sframe(train_test.first));
  ret["test"] = to_variant(gl_sframe(train_test.second));
  return ret;

}





BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(train_test_split, "data", "user_column", "item_column", "max_num_users", "item_test_proportion", "random_seed") 
END_FUNCTION_REGISTRATION

}}


