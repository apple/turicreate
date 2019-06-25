/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/extensions/option_manager.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <toolkits/recsys/models/popularity.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/util/indexed_sframe_tools.hpp>
#include <toolkits/util/sframe_utils.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>

#include <core/random/random.hpp>
#include <memory>
#include <tuple>

namespace turi { namespace recsys {

void recsys_popularity::init_options(const std::map<std::string,
                                 flexible_type>&_options) {

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

  opt.name           = "random_seed";
  opt.description    = "Random seed to use for the model.";
  opt.default_value  = 0;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound    = 0;
  opt.upper_bound    = std::numeric_limits<flex_int>::max();
  options.create_option(opt);

  // Set user specified options
  options.set_options(_options);

  // Save options to state variable
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}



////////////////////////////////////////////////////////////////////////////////

std::map<std::string, flexible_type> recsys_popularity::train(const v2::ml_data& data) {

  bool predict_with_counts = !data.has_target();

  size_t num_items = metadata->index_size(ITEM_COLUMN_INDEX);
  item_predictions.resize(num_items);

  logprogress_stream << data.size() << " observations to process; with "
                     << num_items << " unique items." << std::endl;

  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);
  auto item_stats = metadata->statistics(ITEM_COLUMN_INDEX);

  turi::timer t;
  t.start();

  if(!predict_with_counts) {
    double global_sum = 0;

    std::vector<v2::ml_data_entry> x;
    std::vector<double> mean_rating(num_items, 0.0);

    for(auto it = data.get_iterator(); !it.done(); ++it) {
      it.fill_observation(x);
      size_t item = x[ITEM_COLUMN_INDEX].index;

      mean_rating[item] += it.target_value();
      global_sum += it.target_value();
    }

    for(size_t item = 0; item < num_items; ++item) {
      item_predictions[item] = mean_rating[item] / std::max<size_t>(1, item_stats->count(item));
    }

    unseen_item_prediction = global_sum / (std::max<size_t>(1, data.num_rows()));

  } else {

    for(size_t item = 0; item < num_items; ++item) {
      item_predictions[item] = double(item_stats->count(item));
    }

    unseen_item_prediction = double(num_items) / (std::max<size_t>(1, data.num_rows()));
  }

  // Store the results in an SFrame.
  sframe items_with_predictions = sframe_from_ranged_generator(
      {metadata->column_name(ITEM_COLUMN_INDEX), "prediction"},
      {metadata->column_type(ITEM_COLUMN_INDEX), flex_type_enum::FLOAT},
      metadata->column_size(ITEM_COLUMN_INDEX),
      [&](size_t idx, std::vector<flexible_type>& out) {
        out = {metadata->indexer(ITEM_COLUMN_INDEX)->map_index_to_value(idx),
               item_predictions[idx]};
      });

  std::shared_ptr<unity_sframe> ip_usf(new unity_sframe);
  ip_usf->construct_from_sframe(items_with_predictions);

  add_or_update_state({ {"item_predictions", to_variant(ip_usf)} });

  // And we're done.
  std::map<std::string, flexible_type> ret;
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

std::map<std::string, flexible_type> recsys_popularity::train(
    std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > > trained_user_items) {

  bool predict_with_counts = !metadata->has_target();

  size_t num_items = metadata->index_size(ITEM_COLUMN_INDEX);
  item_predictions.resize(num_items);

  auto item_indexer = metadata->indexer(ITEM_COLUMN_INDEX);
  auto item_stats = metadata->statistics(ITEM_COLUMN_INDEX);

  turi::timer t;
  t.start();

  if(!predict_with_counts) {
    double global_sum = 0;

    std::vector<double> mean_rating(num_items, 0.0);

    std::vector<std::vector<std::pair<size_t, double> > > column_data;

    size_t num_obs = 0;
    auto reader = trained_user_items->get_reader();

    for(size_t row = 0; row < trained_user_items->size(); ++row) {
      reader->read_rows(row, row + 1 , column_data);
      for(const auto& p : column_data[0]) {
        mean_rating[p.first] += p.second;
        global_sum += p.second;
        ++num_obs;
      }
    }

    for(size_t item = 0; item < num_items; ++item) {
      item_predictions[item] = mean_rating[item] / std::max<size_t>(1, item_stats->count(item));
    }

    unseen_item_prediction = global_sum / (std::max<size_t>(1, num_obs));

  } else {

    size_t num_obs = 0;
    for(size_t item = 0; item < num_items; ++item) {
      item_predictions[item] = double(item_stats->count(item));
      num_obs += item_stats->count(item);
    }

    unseen_item_prediction = double(num_items) / (std::max<size_t>(1, num_obs));
  }

  // Store the results in an SFrame.
  sframe items_with_predictions = sframe_from_ranged_generator(
      {metadata->column_name(ITEM_COLUMN_INDEX), "prediction"},
      {metadata->column_type(ITEM_COLUMN_INDEX), flex_type_enum::FLOAT},
      metadata->column_size(ITEM_COLUMN_INDEX),
      [&](size_t idx, std::vector<flexible_type>& out) {
        out = {metadata->indexer(ITEM_COLUMN_INDEX)->map_index_to_value(idx),
               item_predictions[idx]};
      });

  std::shared_ptr<unity_sframe> ip_usf(new unity_sframe);
  ip_usf->construct_from_sframe(items_with_predictions);

  add_or_update_state({ {"item_predictions", to_variant(ip_usf)} });

  // And we're done.
  std::map<std::string, flexible_type> ret;
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

sframe recsys_popularity::predict(const v2::ml_data& test_data) const {

  std::shared_ptr<sarray<flexible_type> > ret(new sarray<flexible_type>);

  size_t n_threads = thread::cpu_count();

  size_t num_segments = n_threads;

  ret->open_for_write(num_segments);
  ret->set_type(flex_type_enum::FLOAT);

  // for(size_t thread_idx = 0; thread_idx < n_threads; ++thread_idx) {
  in_parallel([&](size_t thread_idx, size_t num_threads) {

      auto it_out = ret->get_output_iterator(thread_idx);

      std::vector<v2::ml_data_entry> x;

      for(auto it = test_data.get_iterator(thread_idx, n_threads); !it.done(); ++it, ++it_out) {
        it.fill_observation(x);
        size_t item_idx = x[ITEM_COLUMN_INDEX].index;

        *it_out = ((item_idx < metadata->index_size(ITEM_COLUMN_INDEX))
                   ? item_predictions[item_idx]
                   : unseen_item_prediction);
      }
    });

  ret->close();

  return sframe(std::vector<std::shared_ptr<sarray<flexible_type> > >{ret},
                std::vector<std::string>{"prediction"});;
}

sframe recsys_popularity::get_similar_items(
    std::shared_ptr<sarray<flexible_type> > items, size_t k) const {

  return _create_similar_sframe(
      ITEM_COLUMN_INDEX, items, k,
      [&](size_t query_idx, std::vector<std::pair<size_t, double> >& idx_dist_dest) {
        idx_dist_dest.resize(item_predictions.size());

        double v = (query_idx < item_predictions.size()
                    ? item_predictions[query_idx]
                    : unseen_item_prediction);

        double max_diff = 0;

        for(size_t i = 0; i < item_predictions.size(); ++i) {
          double diff = std::pow(v - item_predictions[i], 2);
          idx_dist_dest[i] = {i, diff};
          max_diff = std::max(max_diff, diff);
        }

        for(size_t i = 0; i < item_predictions.size(); ++i) {
          // Scale things so that most similar is given a score of 1,
          // and everything else is given something in [0, 1).
          idx_dist_dest[i].second = (1.0 - idx_dist_dest[i].second / max_diff);
        }

      });
}

sframe recsys_popularity::get_similar_users(
    std::shared_ptr<sarray<flexible_type> > users, size_t k) const {

  return _create_similar_sframe(
      USER_COLUMN_INDEX, users, k,
      [&](size_t query_idx, std::vector<std::pair<size_t, double> >& idx_dist_dest) {

        size_t n = metadata->index_size(USER_COLUMN_INDEX);
        idx_dist_dest.resize(n);

        double v = metadata->statistics(USER_COLUMN_INDEX)->count(query_idx);

        double max_diff = 0;

        for(size_t i = 0; i < n;  ++i) {
          double v2 = metadata->statistics(USER_COLUMN_INDEX)->count(i);
          double diff = std::pow(v - v2, 2);
          idx_dist_dest[i] = {i, diff};
          max_diff = std::max(max_diff, diff);
        }

        for(size_t i = 0; i < n; ++i) {
          // Scale things so that most similar is given a score of 1,
          // and everything else is given something in [0, 1).
          idx_dist_dest[i].second = (1.0 - idx_dist_dest[i].second / max_diff);
        }
      });
}

void recsys_popularity::score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::vector<std::pair<size_t, double> >& user_item_list,
      const std::vector<std::pair<size_t, double> >& new_user_item_data,
      const std::vector<v2::ml_data_row_reference>& new_observation_data,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const {

  for(auto& p : scores) {
    p.second = (p.first < item_predictions.size()) ?
        item_predictions[p.first] : unseen_item_prediction;
  }
}

////////////////////////////////////////////////////////////////////////////////

void recsys_popularity::internal_save(turi::oarchive& oarc) const {
  oarc << item_predictions
       << unseen_item_prediction;
  bool has_nearest_items_model = false;
  oarc << has_nearest_items_model;
}

void recsys_popularity::internal_load(turi::iarchive& iarc, size_t version) {
  ASSERT_EQ(version, POPULARITY_RECOMMENDER_VERSION);
  iarc >> item_predictions
       >> unseen_item_prediction;

  bool has_nearest_items_model;
  iarc >> has_nearest_items_model;

  // Ignore it, as we don't use it.
  nearest_neighbors::ball_tree_neighbors* m = new nearest_neighbors::ball_tree_neighbors();
  if (has_nearest_items_model) {
    iarc >> *m;
  }
}
}}
