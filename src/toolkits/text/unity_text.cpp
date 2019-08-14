/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/unity_base_types.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <sstream>
#include <string>
#include <iostream>
#include <core/util/bitops.hpp>
#include <toolkits/text/unity_text.hpp>
#include <toolkits/text/topic_model.hpp>
#include <toolkits/text/cgs.hpp>
#include <toolkits/text/alias.hpp>
#include <toolkits/text/perplexity.hpp>
#include <timer/timer.hpp>
#include <core/export.hpp>
namespace turi {
namespace text {


/**
 * Initialize a topic model object.
 *
 * Returns a model_base pointer to Python.
 */
variant_map_type init(variant_map_type& params) {

  variant_map_type ret;

  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "data")->get_underlying_sarray();

  flexible_type model_name = safe_varmap_get<flexible_type>(params, "model_name");

  std::shared_ptr<topic_model> model;
  if (model_name == "cgs_topic_model") {
    model = std::make_shared<cgs_topic_model>();
  } else if (model_name == "alias_topic_model") {
    model = std::make_shared<alias_topic_model>();
  }

  std::map<std::string, flexible_type> opts = varmap_to_flexmap(params);
  opts.erase("model_name");
  model->init_options(opts);

  // If any associations are provided, set them.
  sframe associations =
    *((safe_varmap_get<std::shared_ptr<unity_sframe>>(
          params, "associations"))->get_underlying_sframe());
  if (associations.num_rows() > 0) {
    model->set_associations(associations);
  }

  ret["model"] = to_variant(model);
  return ret;
}



/**
 * Get the current set of options.
 */
variant_map_type get_current_options(variant_map_type& params) {
  log_func_entry();
  variant_map_type ret;

  // retrieve the correct model
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");

  if (model == nullptr) {
    log_and_throw("Provided model is not a topic model.");
  }

  std::map<std::string, flexible_type> options = model->get_current_options();

  // loop through the parameters and record in the return object
  for (const auto& opt : options) {
    ret[opt.first] = opt.second;
  }

  // return stuff
  return ret;
}


/**
 * Toolkit function that modifies a model to have a new vocabulary and set of topics.
 */
variant_map_type set_topics(variant_map_type& params) {

  variant_map_type ret;

  // Get model and data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");
  std::shared_ptr<sarray<flexible_type> > topics =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "topics")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > vocabulary =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "vocabulary")->get_underlying_sarray();
  size_t weight = (safe_varmap_get<flexible_type>(params, "weight"));

  model->set_topics(topics, vocabulary, weight);

  ret["model"] = to_variant(model);
  return ret;
}

/**
 * Toolkit function that trains a model.
 */
variant_map_type train(variant_map_type& params) {

  variant_map_type ret;

  // Get model and data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");
  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "data")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > validation_train =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "validation_train")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > validation_test =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "validation_test")->get_underlying_sarray();
  size_t verbose = safe_varmap_get<flexible_type>(params, "verbose");

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

  ret["model"] = to_variant(model);
  return ret;
}

/**
 * This function retreives an SFrame containing information about the current
 * topic distribution.
 *
 * Returns an SFrame with columns named topic, word, and score.
 */
variant_map_type get_topic(variant_map_type& params) {

  variant_map_type ret;

  // Get data from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");
  std::vector<flexible_type> topic_ids = safe_varmap_get<flexible_type>(params, "topic_ids");
  flexible_type num_words = safe_varmap_get<flexible_type>(params, "num_words");
  flexible_type cdf_cutoff = safe_varmap_get<flexible_type>(params, "cdf_cutoff");

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
  ret["top_words"] =  to_variant(top_words_sf);

  return ret;
}


variant_map_type predict(variant_map_type& params) {

  variant_map_type ret;
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");
  std::shared_ptr<sarray<flexible_type> > dataset =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "data")->get_underlying_sarray();

  flexible_type num_burnin = safe_varmap_get<flexible_type>(params, "num_burnin");

  auto predictions_sa = model->predict_gibbs(dataset, num_burnin);
  std::shared_ptr<unity_sarray> predictions(new unity_sarray());
  predictions->construct_from_sarray(predictions_sa);
  ret["predictions"] = to_variant(predictions);

  return ret;
}

variant_map_type get_perplexity(variant_map_type& params) {

  variant_map_type ret;
  std::shared_ptr<sarray<flexible_type> > test_data =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "test_data")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > predictions =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "predictions")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > topics =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "topics")->get_underlying_sarray();
  std::shared_ptr<sarray<flexible_type> > vocabulary =
      safe_varmap_get<std::shared_ptr<unity_sarray>>(params, "vocabulary")->get_underlying_sarray();

  ret["perplexity"] = perplexity(test_data, predictions, topics, vocabulary);

  return ret;
}

/**
 * Return any value from the model
 */
variant_map_type get_value(variant_map_type& params) {

  variant_map_type ret;

  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");
  flexible_type field = safe_varmap_get<flexible_type>(params, "field");

  if (field == "topics") {
    auto probabilities = model->get_topics_matrix();
    auto vocab = model->get_vocabulary();
    std::vector<std::string> colnames;
    colnames.push_back("topic_probabilities");
    colnames.push_back("vocabulary");
    sframe topics_sf = sframe({probabilities, vocab}, colnames);

    std::shared_ptr<unity_sframe> unity_topics(new unity_sframe());
    unity_topics->construct_from_sframe(topics_sf);
    ret["value"] =  to_variant(unity_topics);
    // model->status["topics"] = unity_topics;
  } else if (field == "vocabulary") {
    auto vocab_sa = model->get_vocabulary();
    std::shared_ptr<unity_sarray> unity_vocab(new unity_sarray());
    unity_vocab->construct_from_sarray(vocab_sa);
    ret["value"] =  to_variant(unity_vocab);
    // model->status["vocabulary"] = unity_vocab;
  } else {
    ret["value"] = model->get_value_from_state(field);
  }

  return ret;
}

/**
 * Return all (key, value) pairs used to describe this model.
 */
variant_map_type summary(variant_map_type& params) {

  variant_map_type ret;

  // Get model from Python
  std::shared_ptr<topic_model> model
    = safe_varmap_get<std::shared_ptr<topic_model>>(params, "model");

  // Get status items
  for (auto& kvp : model->get_state()) {
    ret[kvp.first] = kvp.second;
  }

  return ret;
}

/**
 * Defines get_toolkit_function_registration for the text toolkit
 */
BEGIN_FUNCTION_REGISTRATION
REGISTER_NAMED_FUNCTION("topicmodel_init", init, "params")
REGISTER_NAMED_FUNCTION("topicmodel_set_topics", set_topics, "params")
REGISTER_NAMED_FUNCTION("topicmodel_train", train, "params")
REGISTER_NAMED_FUNCTION("topicmodel_predict", predict, "params")
REGISTER_NAMED_FUNCTION("topicmodel_get_topic", get_topic, "params")
REGISTER_NAMED_FUNCTION("topicmodel_get_perplexity", get_perplexity, "params")
REGISTER_NAMED_FUNCTION("topicmodel_get_value", get_value, "params")
REGISTER_NAMED_FUNCTION("topicmodel_get_current_options", get_current_options, "params")
REGISTER_NAMED_FUNCTION("topicmodel_summary", summary, "params")
END_FUNCTION_REGISTRATION

}
}
