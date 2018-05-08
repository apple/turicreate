/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <serialization/serialization_includes.hpp>
#include <fileio/temp_files.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/unity_global.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/lib/version_number.hpp>
#include <unity/toolkits/ml_data_2/ml_data.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <unity/toolkits/util/indexed_sframe_tools.hpp>
#include <unity/toolkits/ml_data_2/ml_data_iterators.hpp>
#include <unity/toolkits/util/precision_recall.hpp>
#include <unity/toolkits/util/sframe_utils.hpp>
#include <unity/toolkits/recsys/recsys_model_base.hpp>
#include <unity/toolkits/recsys/models.hpp>
#include <unity/toolkits/recsys/unity_recsys.hpp>
#include <unity/toolkits/recsys/train_test_split.hpp>
#include <sframe/sframe.hpp>
#include <sframe/algorithm.hpp>
#include <fileio/general_fstream.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>
#include <export.hpp>
#include <unity/toolkits/coreml_export/MLModel/src/Format.hpp>


namespace turi { namespace recsys {

////////////////////////////////////////////////////////////////////////////////

/*
 * TOOLKIT INTERACTION
 */

variant_map_type init(variant_map_type& params) {

  variant_map_type ret;

  // get model name
  flexible_type model_name = safe_varmap_get<flexible_type>(params, "model_name");
  // get other options
  std::map<std::string, flexible_type> opts = varmap_to_flexmap(params);
  opts.erase("model_name");  // model_name is not an option that can be set
  logprogress_stream << "Recsys training: model = " << model_name << std::endl;

  // initialize the model
  std::shared_ptr<recsys_model_base> m;
  if (model_name == "item_similarity") {
    m = std::make_shared<recsys_itemcf>();
  } else if (model_name == "item_content_recommender") {
    m = std::make_shared<recsys_item_content_recommender>();
  } else if (model_name == "factorization_recommender") {
    m = std::make_shared<recsys_factorization_model>();
  } else if (model_name == "ranking_factorization_recommender") {
    m = std::make_shared<recsys_ranking_factorization_model>();
  } else if (model_name == "popularity") {
    m = std::make_shared<recsys_popularity>();
  }

  // check the model is actually a recsys model.
  if (m == nullptr) {
    throw(std::string("Invalid model name: " + model_name + " is not a recsys model."));
  }
  m->add_or_update_state({{"model_name", to_variant(m->name())}});
  m->init_options(opts);

  ret["model"] = to_variant(m);

  return ret;
}

variant_map_type train(variant_map_type& params) {

  variant_map_type ret;

  // Get model from Python
  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  // Get dataset from Python
  std::shared_ptr<unity_sframe> unity_dataset =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "dataset");

  sframe dataset = *(unity_dataset->get_underlying_sframe());

  std::shared_ptr<unity_sframe> unity_user_data =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "user_data");

  sframe user_side_data = *(unity_user_data->get_underlying_sframe());

  std::shared_ptr<unity_sframe> unity_item_data =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "item_data");

  sframe item_side_data = *(unity_item_data->get_underlying_sframe());


  std::map<std::string, flexible_type> opts = varmap_to_flexmap(params);

  opts.erase("model_name");

  m->set_options(opts);

  m->setup_and_train(dataset, user_side_data, item_side_data, params);

  ret["model"] = to_variant(m);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

variant_map_type predict(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  sframe sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
      params, "data_to_predict")->get_underlying_sframe());

  // Currently, new_data is ignored, as none of the models use it.

  sframe new_user_data_sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "new_user_data")->get_underlying_sframe());
  sframe new_item_data_sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "new_item_data")->get_underlying_sframe());

  sframe predictions = m->predict(m->create_ml_data(sf, new_user_data_sf, new_item_data_sf));

  variant_map_type ret;

  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe());
  ret_sf->construct_from_sframe(predictions);
  ret["data"] = to_variant(ret_sf);


  return ret;
}

////////////////////////////////////////////////////////////////////////////////

variant_map_type recommend(variant_map_type& params) {

  turi::timer timer;

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

#define __EXTRACT_SFRAME(dest, name)                                    \
  sframe dest = *(                                               \
      (safe_varmap_get<std::shared_ptr<unity_sframe> >(                 \
          params, name))->get_underlying_sframe())

  __EXTRACT_SFRAME(query_sf, "query");
  __EXTRACT_SFRAME(exclusion_data_sf, "exclude");
  __EXTRACT_SFRAME(restrictions_sf, "restrictions");
  __EXTRACT_SFRAME(new_observation_data_sf, "new_data");
  __EXTRACT_SFRAME(new_user_data_sf, "new_user_data");
  __EXTRACT_SFRAME(new_item_data_sf, "new_item_data");

#undef __EXTRACT_SFRAME

  bool exclude_training_interactions = (safe_varmap_get<flexible_type>(params, "exclude_known") == 1);
  size_t top_k = safe_varmap_get<flexible_type>(params, "top_k");
  double diversity = safe_varmap_get<flexible_type>(params, "diversity");
  size_t random_seed = safe_varmap_get<flexible_type>(params, "random_seed");

  timer.start();

  // Rank items
  sframe ranks = m->recommend(query_sf,
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

  variant_map_type ret;

  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(ranks);

  ret["data"] = to_variant(ret_sf);

  return ret;
}

variant_map_type precision_recall(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  // size_t K = safe_varmap_get<flexible_type>(params, "top_k");

  // Take provided SFrame of validation data and convert into recsys_data objects.
  sframe valid_sf =
      *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
          params, "validation_data")->get_underlying_sframe());

  auto user_indexer = m->metadata->indexer(m->USER_COLUMN_INDEX);
  auto item_indexer = m->metadata->indexer(m->ITEM_COLUMN_INDEX);
  std::string user_column_name = m->metadata->column_name(m->USER_COLUMN_INDEX);
  std::string item_column_name = m->metadata->column_name(m->ITEM_COLUMN_INDEX);

  sframe indexed_validation_data = v2::map_to_indexed_sframe(
      {user_indexer, item_indexer},
      valid_sf, true);


  sframe avail_sf =
      *(safe_varmap_get<std::shared_ptr<unity_sframe> >(
          params, "available_data")->get_underlying_sframe());


  // Get the list of cutoffs from Python
  std::vector<flexible_type> cutoffs_flextype
      = safe_varmap_get<dataframe_t>(params, "cutoffs").values["cutoff"];
  std::vector<size_t> cutoffs(cutoffs_flextype.begin(), cutoffs_flextype.end());

  size_t max_k = *std::max_element(cutoffs.begin(), cutoffs.end());

  auto users = sframe({make_unique(indexed_validation_data.select_column(m->USER_COLUMN_INDEX))},
                      {m->metadata->column_name(m->USER_COLUMN_INDEX)});

  static const sframe null_sf;

  // Call rank items
  sframe ranks = m->recommend(users, max_k, null_sf, avail_sf, null_sf, null_sf, null_sf, true);

  sframe raw_pr_stats = m->precision_recall_stats(indexed_validation_data, ranks, cutoffs);

  ////////////////////////////////////////////////////////////////////////////////
  // Convert back to sframes

  sframe pr_stats = v2::map_from_custom_indexed_sframe(
      { {user_column_name, user_indexer} },
      raw_pr_stats);

  std::shared_ptr<unity_sframe> pr_sf(new unity_sframe);
  pr_sf->construct_from_sframe(pr_stats);

  std::shared_ptr<unity_sframe> rank_sf(new unity_sframe);
  rank_sf->construct_from_sframe(ranks);

  variant_map_type ret;
  ret["results"]      = to_variant(pr_sf);
  ret["ranked_items"] = to_variant(rank_sf);


  return ret;
}

////////////////////////////////////////////////////////////////////////////////

variant_map_type get_value(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  std::shared_ptr<recsys_model_base> model
    = safe_varmap_get<std::shared_ptr<recsys_model_base> >(params, "model");

  flexible_type field = safe_varmap_get<flexible_type>(params, "field");

  if (model == nullptr) {
    log_and_throw("Internal error invoking model: model_ptr null.");
  }

  // Make sure this model exists.
  // --------------------------------------------------------------------------
  ret["value"] = model->get_value_from_state(field);
  return ret;
}

variant_map_type list_fields(variant_map_type& params) {

  // Unpack from Python
  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  std::set<std::string> fields;

  for(const std::string& s : model->list_fields())
    fields.insert(s);

  variant_map_type ret;
  ret["value"] = flex_list(fields.begin(), fields.end());
  return ret;
}

variant_map_type get_train_stats(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  std::map<std::string, flexible_type> options = model->get_train_stats();

  variant_map_type ret;
  for (auto& opt : options) {
    ret[opt.first] = opt.second;
  }
  return ret;
}


variant_map_type get_current_options(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> model =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  std::map<std::string, flexible_type> options = model->get_current_options();

  variant_map_type ret;
  for (auto& opt : options) {
    ret[opt.first] = opt.second;
  }
  return ret;
}

variant_map_type set_current_options(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> model =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  std::map<std::string, flexible_type> options = varmap_to_flexmap(params);
  options.erase("model");
  model->set_options(options);

  return variant_map_type();
}

variant_map_type summary(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  variant_map_type ret;
  for (auto& opt : model->get_current_options()) {
    ret[opt.first] = opt.second;
  }
  for (auto& opt : model->get_train_stats()) {
    ret[opt.first] = opt.second;
  }

  return ret;
}

variant_map_type train_test_split(variant_map_type& params) {

  variant_map_type ret;
  std::shared_ptr<unity_sframe> unity_dataset = safe_varmap_get<std::shared_ptr<unity_sframe>>(params, "dataset");
  sframe dataset = *(unity_dataset->get_underlying_sframe());

  flexible_type user_column = safe_varmap_get<flexible_type>(params, "user_id");
  flexible_type item_column = safe_varmap_get<flexible_type>(params, "item_id");
  flexible_type max_num_users = safe_varmap_get<flexible_type>(params, "max_num_users");
  flexible_type item_test_proportion = safe_varmap_get<flexible_type>(params, "item_test_proportion");
  flexible_type random_seed = safe_varmap_get<flexible_type>(params, "random_seed");

  try {
    item_test_proportion = item_test_proportion.to<flex_float>();
  } catch (...) {
    throw std::string("Error interpreting item_test_proportion as float between 0 and 1.");
  }

  try {
    random_seed = random_seed.to<flex_int>();
  } catch (...) {
    throw std::string("Error interpreting random_seed as integer.");
  }
  
  size_t max_users = size_t(-1);
  if (max_num_users.get_type() != flex_type_enum::UNDEFINED) {
    max_users = (size_t) max_num_users;
  }

  auto train_test = make_recsys_train_test_split(dataset, user_column, item_column,
                                                 max_users,
                                                 item_test_proportion.get<flex_float>(),
                                                 random_seed.get<flex_int>());
  std::shared_ptr<unity_sframe> train(new unity_sframe());
  std::shared_ptr<unity_sframe> test(new unity_sframe());
  train->construct_from_sframe(train_test.first);
  test ->construct_from_sframe(train_test.second);

  ret["train"] = to_variant(train);
  ret["test"] = to_variant(test);
  return ret;

}


variant_map_type get_similar_items(variant_map_type& params) {

  variant_map_type ret;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  auto items_sa = safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "items")->get_underlying_sarray();
  size_t k = safe_varmap_get<flexible_type>(params, "k");
  size_t verbose = safe_varmap_get<flexible_type>(params, "verbose");

  int get_all_items = safe_varmap_get<flexible_type>(params, "get_all_items");

  if(get_all_items) {
    items_sa.reset();
  }

  timer.start();

  sframe raw_ranks = m->get_similar_items(items_sa, k);
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(raw_ranks);

  if (verbose) {
    logprogress_stream << "Getting similar items completed in "
                       << timer.current_time() << "" << std::endl;
  }

  ret["data"] = to_variant(ret_sf);

  return ret;
}

variant_map_type get_similar_users(variant_map_type& params) {

  variant_map_type ret;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  auto users_sa = safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "users")->get_underlying_sarray();
  size_t k = safe_varmap_get<flexible_type>(params, "k");

  int get_all_users = safe_varmap_get<flexible_type>(params, "get_all_users");

  if(get_all_users) {
    users_sa.reset();
  }

  timer.start();

  sframe raw_ranks = m->get_similar_users(users_sa, k);
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(raw_ranks);

  logprogress_stream << "Getting similar users completed in "
      << timer.current_time() << "" << std::endl;

  ret["data"] = to_variant(ret_sf);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

variant_map_type get_item_intersection_info(variant_map_type& params) {

  variant_map_type ret;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  // Get dataset from Python
  std::shared_ptr<unity_sframe> item_pairs =
      safe_varmap_get<std::shared_ptr<unity_sframe> >(params, "item_pairs");

  sframe item_info = m->get_item_intersection_info(*(item_pairs->get_underlying_sframe()));
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(item_info);

  ret["item_intersections"] = to_variant(ret_sf);

  return ret;
}


variant_map_type get_num_users_per_item(variant_map_type& params) {

  variant_map_type ret;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  sframe result = m->get_num_users_per_item();
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(result);

  ret["data"] = to_variant(ret_sf);

  return ret;
}


variant_map_type get_num_items_per_user(variant_map_type& params) {

  variant_map_type ret;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  sframe result = m->get_num_items_per_user();
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(result);

  ret["data"] = to_variant(ret_sf);

  return ret;
}


////////////////////////////////////////////////////////////////////////////////

variant_map_type get_popularity_baseline(variant_map_type& params) { 
  variant_map_type ret;

  // Get model from Python
  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(params, "model");

  std::shared_ptr<recsys_model_base> pop_model = m->get_popularity_baseline();

  ret["popularity_model"] = to_variant(pop_model);

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

EXPORT variant_map_type get_data_schema(variant_map_type& params) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base> >(params, "model");

  variant_map_type ret;
  ret["schema"] = model->get_data_schema();
  
  return ret;
}

////////////////////////////////////////////////////////////////////////////////

void export_to_coreml(
    std::shared_ptr<recsys_model_base> recsys_model,
    const std::string& filename
) {
  recsys_model->export_to_coreml(recsys_model, filename);
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(init, "params")
REGISTER_FUNCTION(train, "params")
REGISTER_FUNCTION(predict, "params")
REGISTER_FUNCTION(recommend, "params")
REGISTER_FUNCTION(get_value, "params")
REGISTER_FUNCTION(list_fields, "params")
REGISTER_FUNCTION(precision_recall, "params")
REGISTER_FUNCTION(get_train_stats, "params")
REGISTER_FUNCTION(get_current_options, "params")
REGISTER_FUNCTION(set_current_options, "params")
REGISTER_FUNCTION(summary, "params")
REGISTER_FUNCTION(train_test_split, "params")
REGISTER_FUNCTION(get_similar_items, "params")
REGISTER_FUNCTION(get_similar_users, "params")
REGISTER_FUNCTION(get_num_items_per_user, "params")
REGISTER_FUNCTION(get_num_users_per_item, "params")
REGISTER_FUNCTION(get_popularity_baseline, "params")
REGISTER_FUNCTION(get_data_schema, "params")
REGISTER_FUNCTION(get_item_intersection_info, "params")
REGISTER_FUNCTION(export_to_coreml, "model", "filename")
END_FUNCTION_REGISTRATION

}}
