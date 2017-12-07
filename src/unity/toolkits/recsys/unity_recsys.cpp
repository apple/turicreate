/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <serialization/serialization_includes.hpp>
#include <fileio/temp_files.hpp>
#include <unity/lib/api/model_interface.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sframe.hpp>
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


namespace turi { namespace recsys {

////////////////////////////////////////////////////////////////////////////////

/*
 * TOOLKIT INTERACTION
 */

toolkit_function_response_type init(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // get model name
  flexible_type model_name = safe_varmap_get<flexible_type>(invoke.params, "model_name");
  // get other options
  std::map<std::string, flexible_type> opts = varmap_to_flexmap(invoke.params);
  opts.erase("model_name");  // model_name is not an option that can be set
  logprogress_stream << "Recsys training: model = " << model_name << std::endl;
  // initialize the model
  std::shared_ptr<recsys_model_base> m =
      std::dynamic_pointer_cast<recsys_model_base>(invoke.classes->get_toolkit_class(model_name));

  // check the model is actually a recsys model.
  if (m == nullptr) {
    throw(std::string("Invalid model name: " + model_name + " is not a recsys model."));
  }
  m->add_or_update_state({{"model_name", to_variant(m->name())}});
  m->init_options(opts);

  ret_status.params["model"] = to_variant(m);
  ret_status.success = true;

  return ret_status;
}

toolkit_function_response_type train(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get model from Python
  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  // Get dataset from Python
  std::shared_ptr<unity_sframe> unity_dataset =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "dataset");

  sframe dataset = *(unity_dataset->get_underlying_sframe());

  std::shared_ptr<unity_sframe> unity_user_data =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "user_data");

  sframe user_side_data = *(unity_user_data->get_underlying_sframe());

  std::shared_ptr<unity_sframe> unity_item_data =
      safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "item_data");

  sframe item_side_data = *(unity_item_data->get_underlying_sframe());


  std::map<std::string, flexible_type> opts = varmap_to_flexmap(invoke.params);

  opts.erase("model_name");

  m->set_options(opts);

  m->setup_and_train(dataset, user_side_data, item_side_data, invoke.params);

  ret_status.params["model"] = to_variant(m);
  ret_status.success = true;

  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

toolkit_function_response_type predict(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  sframe sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
      invoke.params, "data_to_predict")->get_underlying_sframe());

  // Currently, new_data is ignored, as none of the models use it.

  sframe new_user_data_sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "new_user_data")->get_underlying_sframe());
  sframe new_item_data_sf = *(safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "new_item_data")->get_underlying_sframe());

  sframe predictions = m->predict(m->create_ml_data(sf, new_user_data_sf, new_item_data_sf));

  toolkit_function_response_type ret_status;

  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe());
  ret_sf->construct_from_sframe(predictions);
  ret_status.params["data"] = to_variant(ret_sf);

  ret_status.success = true;

  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

toolkit_function_response_type recommend(toolkit_function_invocation& invoke) {

  turi::timer timer;

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

#define __EXTRACT_SFRAME(dest, name)                                    \
  sframe dest = *(                                               \
      (safe_varmap_get<std::shared_ptr<unity_sframe> >(                 \
          invoke.params, name))->get_underlying_sframe())

  __EXTRACT_SFRAME(query_sf, "query");
  __EXTRACT_SFRAME(exclusion_data_sf, "exclude");
  __EXTRACT_SFRAME(restrictions_sf, "restrictions");
  __EXTRACT_SFRAME(new_observation_data_sf, "new_data");
  __EXTRACT_SFRAME(new_user_data_sf, "new_user_data");
  __EXTRACT_SFRAME(new_item_data_sf, "new_item_data");

#undef __EXTRACT_SFRAME

  bool exclude_training_interactions = (safe_varmap_get<flexible_type>(invoke.params, "exclude_known") == 1);
  size_t top_k = safe_varmap_get<flexible_type>(invoke.params, "top_k");
  double diversity = safe_varmap_get<flexible_type>(invoke.params, "diversity");
  size_t random_seed = safe_varmap_get<flexible_type>(invoke.params, "random_seed");

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

  toolkit_function_response_type ret_status;

  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(ranks);

  ret_status.params["data"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}

toolkit_function_response_type precision_recall(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  // size_t K = safe_varmap_get<flexible_type>(invoke.params, "top_k");

  // Take provided SFrame of validation data and convert into recsys_data objects.
  sframe valid_sf =
      *(safe_varmap_get<std::shared_ptr<unity_sframe>>(
          invoke.params, "validation_data")->get_underlying_sframe());

  auto user_indexer = m->metadata->indexer(m->USER_COLUMN_INDEX);
  auto item_indexer = m->metadata->indexer(m->ITEM_COLUMN_INDEX);
  std::string user_column_name = m->metadata->column_name(m->USER_COLUMN_INDEX);
  std::string item_column_name = m->metadata->column_name(m->ITEM_COLUMN_INDEX);

  sframe indexed_validation_data = v2::map_to_indexed_sframe(
      {user_indexer, item_indexer},
      valid_sf, true);


  sframe avail_sf =
      *(safe_varmap_get<std::shared_ptr<unity_sframe> >(
          invoke.params, "available_data")->get_underlying_sframe());


  // Get the list of cutoffs from Python
  std::vector<flexible_type> cutoffs_flextype
      = safe_varmap_get<dataframe_t>(invoke.params, "cutoffs").values["cutoff"];
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

  toolkit_function_response_type ret_status;
  ret_status.params["results"]      = to_variant(pr_sf);
  ret_status.params["ranked_items"] = to_variant(rank_sf);

  ret_status.success = true;

  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

toolkit_function_response_type get_value(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  std::shared_ptr<recsys_model_base> model
    = safe_varmap_get<std::shared_ptr<recsys_model_base> >(invoke.params, "model");

  flexible_type field = safe_varmap_get<flexible_type>(invoke.params, "field");

  if (model == nullptr) {
    log_and_throw("Internal error invoking model: model_ptr null.");
  }

  // Make sure this model exists.
  // --------------------------------------------------------------------------
  ret_status.params["value"] = model->get_value_from_state(field);
  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type list_fields(toolkit_function_invocation& invoke) {

  // Unpack from Python
  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  std::set<std::string> fields;

  for(const std::string& s : model->list_keys())
    fields.insert(s);

  toolkit_function_response_type ret_status;
  ret_status.params["value"] = flex_list(fields.begin(), fields.end());
  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type get_train_stats(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  std::map<std::string, flexible_type> options = model->get_train_stats();

  toolkit_function_response_type ret_status;
  for (auto& opt : options) {
    ret_status.params[opt.first] = opt.second;
  }
  ret_status.success = true;
  return ret_status;
}


toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> model =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  std::map<std::string, flexible_type> options = model->get_current_options();

  toolkit_function_response_type ret_status;
  for (auto& opt : options) {
    ret_status.params[opt.first] = opt.second;
  }
  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type set_current_options(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> model =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  std::map<std::string, flexible_type> options = varmap_to_flexmap(invoke.params);
  options.erase("model");
  model->set_options(options);

  toolkit_function_response_type ret_status;
  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type summary(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  toolkit_function_response_type ret_status;
  for (auto& opt : model->get_current_options()) {
    ret_status.params[opt.first] = opt.second;
  }
  for (auto& opt : model->get_train_stats()) {
    ret_status.params[opt.first] = opt.second;
  }

  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type train_test_split(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  std::shared_ptr<unity_sframe> unity_dataset = safe_varmap_get<std::shared_ptr<unity_sframe>>(invoke.params, "dataset");
  sframe dataset = *(unity_dataset->get_underlying_sframe());

  flexible_type user_column = safe_varmap_get<flexible_type>(invoke.params, "user_id");
  flexible_type item_column = safe_varmap_get<flexible_type>(invoke.params, "item_id");
  flexible_type max_num_users = safe_varmap_get<flexible_type>(invoke.params, "max_num_users");
  flexible_type item_test_proportion = safe_varmap_get<flexible_type>(invoke.params, "item_test_proportion");
  flexible_type random_seed = safe_varmap_get<flexible_type>(invoke.params, "random_seed");

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

  ret_status.params["train"] = to_variant(train);
  ret_status.params["test"] = to_variant(test);
  ret_status.success = true;
  return ret_status;

}


toolkit_function_response_type get_similar_items(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  auto items_sa = safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "items")->get_underlying_sarray();
  size_t k = safe_varmap_get<flexible_type>(invoke.params, "k");
  size_t verbose = safe_varmap_get<flexible_type>(invoke.params, "verbose");

  int get_all_items = safe_varmap_get<flexible_type>(invoke.params, "get_all_items");

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

  ret_status.params["data"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}

toolkit_function_response_type get_similar_users(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  auto users_sa = safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "users")->get_underlying_sarray();
  size_t k = safe_varmap_get<flexible_type>(invoke.params, "k");

  int get_all_users = safe_varmap_get<flexible_type>(invoke.params, "get_all_users");

  if(get_all_users) {
    users_sa.reset();
  }

  timer.start();

  sframe raw_ranks = m->get_similar_users(users_sa, k);
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(raw_ranks);

  logprogress_stream << "Getting similar users completed in "
      << timer.current_time() << "" << std::endl;

  ret_status.params["data"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

toolkit_function_response_type get_item_intersection_info(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  turi::timer timer;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  // Get dataset from Python
  std::shared_ptr<unity_sframe> item_pairs =
      safe_varmap_get<std::shared_ptr<unity_sframe> >(invoke.params, "item_pairs");

  sframe item_info = m->get_item_intersection_info(*(item_pairs->get_underlying_sframe()));
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(item_info);

  ret_status.params["item_intersections"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}


toolkit_function_response_type get_num_users_per_item(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  sframe result = m->get_num_users_per_item();
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(result);

  ret_status.params["data"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}


toolkit_function_response_type get_num_items_per_user(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  std::shared_ptr<recsys_model_base> m = safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  sframe result = m->get_num_items_per_user();
  std::shared_ptr<unity_sframe> ret_sf(new unity_sframe);
  ret_sf->construct_from_sframe(result);

  ret_status.params["data"] = to_variant(ret_sf);
  ret_status.success = true;

  return ret_status;
}


////////////////////////////////////////////////////////////////////////////////

toolkit_function_response_type get_popularity_baseline(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get model from Python
  std::shared_ptr<recsys_model_base> m =
      safe_varmap_get<std::shared_ptr<recsys_model_base>>(invoke.params, "model");

  std::shared_ptr<recsys_model_base> pop_model = m->get_popularity_baseline();

  ret_status.params["popularity_model"] = to_variant(pop_model);
  ret_status.success = true;

  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

EXPORT toolkit_function_response_type get_data_schema(toolkit_function_invocation& invoke) {

  std::shared_ptr<recsys_model_base> model = safe_varmap_get<std::shared_ptr<recsys_model_base> >(invoke.params, "model");

  toolkit_function_response_type ret_status;
  ret_status.params["schema"] = model->get_data_schema();
  ret_status.success = true;
  
  return ret_status;
}

////////////////////////////////////////////////////////////////////////////////

EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {

  toolkit_function_specification init_spec;
  init_spec.name = "recsys_init";
  init_spec.toolkit_execute_function = init;

  toolkit_function_specification train_spec;
  train_spec.name = "recsys_train";
  train_spec.toolkit_execute_function = train;

  toolkit_function_specification predict_spec;
  predict_spec.name = "recsys_predict";
  predict_spec.toolkit_execute_function = predict;

  toolkit_function_specification recommend_spec;
  recommend_spec.name = "recsys_recommend";
  recommend_spec.toolkit_execute_function = recommend;

  toolkit_function_specification get_value_spec;
  get_value_spec.name = "recsys_get_value";
  get_value_spec.toolkit_execute_function = get_value;

  toolkit_function_specification list_fields_spec;
  list_fields_spec.name = "recsys_list_fields";
  list_fields_spec.toolkit_execute_function = list_fields;

  toolkit_function_specification precision_recall_spec;
  precision_recall_spec.name = "recsys_precision_recall";
  precision_recall_spec.toolkit_execute_function = precision_recall;

  toolkit_function_specification get_train_stats_spec;
  get_train_stats_spec.name = "recsys_get_train_stats";
  get_train_stats_spec.toolkit_execute_function = get_train_stats;


  toolkit_function_specification get_current_options_spec;
  get_current_options_spec.name = "recsys_get_current_options";
  get_current_options_spec.toolkit_execute_function = get_current_options;

  toolkit_function_specification set_current_options_spec;
  set_current_options_spec.name = "recsys_set_current_options";
  set_current_options_spec.toolkit_execute_function = set_current_options;

  toolkit_function_specification summary_spec;
  summary_spec.name = "recsys_summary";
  summary_spec.toolkit_execute_function = summary;

  toolkit_function_specification train_test_split_spec;
  train_test_split_spec.name = "recsys_train_test_split";
  train_test_split_spec.toolkit_execute_function = train_test_split;

  toolkit_function_specification get_similar_items_spec;
  get_similar_items_spec.name = "recsys_get_similar_items";
  get_similar_items_spec.toolkit_execute_function = get_similar_items;

  toolkit_function_specification get_similar_users_spec;
  get_similar_users_spec.name = "recsys_get_similar_users";
  get_similar_users_spec.toolkit_execute_function = get_similar_users;

  toolkit_function_specification get_num_items_per_user_spec;
  get_num_items_per_user_spec.name = "recsys_get_num_items_per_user";
  get_num_items_per_user_spec.toolkit_execute_function = get_num_items_per_user;

  toolkit_function_specification get_num_users_per_item_spec;
  get_num_users_per_item_spec.name = "recsys_get_num_users_per_item";
  get_num_users_per_item_spec.toolkit_execute_function = get_num_users_per_item;

  toolkit_function_specification get_popularity_baseline_spec;
  get_popularity_baseline_spec.name = "recsys_get_popularity_baseline";
  get_popularity_baseline_spec.toolkit_execute_function = get_popularity_baseline;
  
  toolkit_function_specification get_data_schema_spec;
  get_data_schema_spec.name = "recsys_get_data_schema";
  get_data_schema_spec.toolkit_execute_function = get_data_schema;

  toolkit_function_specification get_item_intersection_info_spec;
  get_item_intersection_info_spec.name = "recsys_get_item_intersection_info";
  get_item_intersection_info_spec.toolkit_execute_function = get_item_intersection_info;


  return {init_spec, train_spec, predict_spec, recommend_spec, get_value_spec,
          list_fields_spec,
          precision_recall_spec, get_train_stats_spec,
          get_current_options_spec, set_current_options_spec,
          summary_spec, train_test_split_spec,
          get_similar_items_spec, get_similar_users_spec,
          get_num_items_per_user_spec, get_num_users_per_item_spec,
          get_popularity_baseline_spec,
          get_data_schema_spec,
	  get_item_intersection_info_spec};
}

}}


