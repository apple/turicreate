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
#include <unity/lib/unity_sframe.hpp>
#include <sframe/sframe.hpp>
#include <sframe/algorithm.hpp>
#include <fileio/general_fstream.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <util/bitops.hpp>
#include <unity/toolkits/text/unity_text.hpp>
#include <unity/toolkits/text/topic_model.hpp>
#include <unity/toolkits/text/cgs.hpp>
#include <unity/toolkits/text/perplexity.hpp>
#include <timer/timer.hpp>
#include <export.hpp>
namespace turi {
namespace text {


/**
 * Initialize a topic model object.
 *
 * Returns a model_base pointer to Python.
 */
toolkit_function_response_type init(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "data")->get_underlying_sarray();

  flexible_type model_name = safe_varmap_get<flexible_type>(invoke.params, "model_name");

  std::shared_ptr<topic_model> model =
      std::dynamic_pointer_cast<topic_model>(invoke.classes->get_toolkit_class(model_name));

  std::map<std::string, flexible_type> opts = varmap_to_flexmap(invoke.params);
  opts.erase("model_name");
  model->init_options(opts);

  // If any associations are provided, set them.
  sframe associations =
    *((safe_varmap_get<std::shared_ptr<unity_sframe>>(
          invoke.params, "associations"))->get_underlying_sframe());
  if (associations.num_rows() > 0) {
    model->set_associations(associations);
  }

  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;
}



/**
 * Get the current set of options.
 */
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke) {
  log_func_entry();
  toolkit_function_response_type ret_status;

  // retrieve the correct model
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");

  if (model == nullptr) {
    log_and_throw("Provided model is not a topic model.");
  }

  std::map<std::string, flexible_type> options = model->get_current_options();

  // loop through the parameters and record in the return object
  for (const auto& opt : options) {
    ret_status.params[opt.first] = opt.second;
  }

  // return stuff
  ret_status.success = true;
  return ret_status;
}


/**
 * Toolkit function that modifies a model to have a new vocabulary and set of topics.
 */
toolkit_function_response_type set_topics(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get model and data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");
  std::shared_ptr<sarray<flexible_type> > topics =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "topics")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > vocabulary =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "vocabulary")->get_underlying_sarray();
  size_t weight = (safe_varmap_get<flexible_type>(invoke.params, "weight"));

  model->set_topics(topics, vocabulary, weight);

  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;
}

/**
 * Toolkit function that trains a model.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get model and data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");
  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "data")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > validation_train =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "validation_train")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > validation_test =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "validation_test")->get_underlying_sarray();
  size_t verbose = safe_varmap_get<flexible_type>(invoke.params, "verbose");

  // Set validation data if it's provided
  if (validation_train->size() > 0) {
    if (validation_train->size() != validation_test->size()) {
      log_and_throw("Validation set must include a train/test pair having the same length.\n The training part is used to estimate topic proportions for each held-out \ndocument; the test part is used for computing held-out perplexity given the model's parameter estimates.");
    }
    model->init_validation(validation_train, validation_test);
  }

  // Train the model on this data set and this vocabulary
  model->train(dataset, verbose);

  if (!model->is_trained())
    log_and_throw("Model did not successfully complete training. \nIf this was not intended, please report this issue.");

  ret_status.params["model"] = to_variant(model);
  ret_status.success = true;
  return ret_status;
}

/**
 * This function retreives an SFrame containing information about the current
 * topic distribution.
 *
 * Returns an SFrame with columns named topic, word, and score.
 */
toolkit_function_response_type get_topic(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");
  std::vector<flexible_type> topic_ids = safe_varmap_get<flexible_type>(invoke.params, "topic_ids");
  flexible_type num_words = safe_varmap_get<flexible_type>(invoke.params, "num_words");
  flexible_type cdf_cutoff = safe_varmap_get<flexible_type>(invoke.params, "cdf_cutoff");

  // Get a list of the most probably words and their score; write to SFrame
  sframe sf;
  sf.open_for_write({"topic", "word", "score"},
                    {flex_type_enum::INTEGER, flex_type_enum::STRING, flex_type_enum::FLOAT},
                    "", 1);
  auto out = sf.get_output_iterator(0);
  std::vector<flexible_type> out_v;
  for (size_t k : topic_ids) {
    auto top_words = model->get_topic(k, num_words, cdf_cutoff);
    for (size_t i = 0; i < top_words.first.size(); ++i) {
      out_v = {k, top_words.first[i], top_words.second[i]};
      *out = out_v;
      ++out;
    }
  }
  sf.close();

  // Return top_words to Python
  std::shared_ptr<unity_sframe> top_words_sf(new unity_sframe);
  top_words_sf->construct_from_sframe(sf);
  ret_status.params["top_words"] =  to_variant(top_words_sf);

  ret_status.success = true;
  return ret_status;
}


toolkit_function_response_type predict(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");
  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "data")->get_underlying_sarray();

  flexible_type num_burnin = safe_varmap_get<flexible_type>(invoke.params, "num_burnin");

  auto predictions_sa = model->predict_gibbs(dataset, num_burnin);
  std::shared_ptr<unity_sarray> predictions(new unity_sarray());
  predictions->construct_from_sarray(predictions_sa);
  ret_status.params["predictions"] = to_variant(predictions);

  ret_status.success = true;
  return ret_status;
}

toolkit_function_response_type get_perplexity(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;
  std::shared_ptr<sarray<flexible_type> > test_data =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "test_data")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > predictions =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "predictions")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > topics =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "topics")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > vocabulary =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(invoke.params, "vocabulary")->get_underlying_sarray();

  ret_status.params["perplexity"] = perplexity(test_data, predictions, topics, vocabulary);

  ret_status.success = true;
  return ret_status;
}

/**
 * Return any value from the model
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");
  flexible_type field = safe_varmap_get<flexible_type>(invoke.params, "field");

  if (field == "topics") {
    auto probabilities = model->get_topics_matrix();
    auto vocab = model->get_vocabulary();
    std::vector<std::string> colnames;
    colnames.push_back("topic_probabilities");
    colnames.push_back("vocabulary");
    sframe topics_sf = sframe({probabilities, vocab}, colnames);

    std::shared_ptr<unity_sframe> unity_topics(new unity_sframe());
    unity_topics->construct_from_sframe(topics_sf);
    ret_status.params["value"] =  to_variant(unity_topics);
    // model->status["topics"] = unity_topics;
  } else if (field == "vocabulary") {
    auto vocab_sa = model->get_vocabulary();
    std::shared_ptr<unity_sarray> unity_vocab(new unity_sarray());
    unity_vocab->construct_from_sarray(vocab_sa);
    ret_status.params["value"] =  to_variant(unity_vocab);
    // model->status["vocabulary"] = unity_vocab;
  } else {
    ret_status.params["value"] = model->get_value_from_state(field);
  }

  ret_status.success = true;
  return ret_status;
}

/**
 * Return all (key, value) pairs used to describe this model.
 */
toolkit_function_response_type summary(toolkit_function_invocation& invoke) {

  toolkit_function_response_type ret_status;

  // Get model from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(invoke.params, "model");

  // Get status items
  for (auto& kvp : model->get_state()) {
    ret_status.params[kvp.first] = kvp.second;
  }

  ret_status.success = true;
  return ret_status;
}

EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {

  std::vector<toolkit_function_specification> specs;

  toolkit_function_specification init_spec;
  init_spec.name = "text_topicmodel_init";
  init_spec.toolkit_execute_function = init;

  toolkit_function_specification set_topics_spec;
  set_topics_spec.name = "text_topicmodel_set_topics";
  set_topics_spec.toolkit_execute_function = set_topics;

  toolkit_function_specification train_spec;
  train_spec.name = "text_topicmodel_train";
  train_spec.toolkit_execute_function = train;

  toolkit_function_specification predict_spec;
  predict_spec.name = "text_topicmodel_predict";
  predict_spec.toolkit_execute_function = predict;

  toolkit_function_specification get_topic_spec;
  get_topic_spec.name = "text_topicmodel_get_topic";
  get_topic_spec.toolkit_execute_function = get_topic;

  toolkit_function_specification perplexity_spec;
  perplexity_spec.name = "text_topicmodel_get_perplexity";
  perplexity_spec.toolkit_execute_function = get_perplexity;

  toolkit_function_specification get_value_spec;
  get_value_spec.name = "text_topicmodel_get_value";
  get_value_spec.toolkit_execute_function = get_value;

  toolkit_function_specification get_current_options_spec;
  get_current_options_spec.name = "text_topicmodel_get_current_options";
  get_current_options_spec.toolkit_execute_function = get_current_options;

  toolkit_function_specification summary_spec;
  summary_spec.name = "text_topicmodel_summary";
  summary_spec.toolkit_execute_function = summary;

  return {init_spec, set_topics_spec, train_spec,
    get_topic_spec, summary_spec, predict_spec,
    get_value_spec, perplexity_spec,
    get_current_options_spec};
}
}
}
