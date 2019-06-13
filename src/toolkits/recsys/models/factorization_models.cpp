/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <toolkits/factorization/factorization_model.hpp>
#include <toolkits/recsys/models/factorization_models.hpp>
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/brute_force_neighbors.hpp>
#include <core/logging/table_printer/table_printer.hpp>

#include <toolkits/factorization/als.hpp>

namespace turi { namespace recsys {

void recsys_factorization_model_base::init_options(const std::map<std::string,
                                 flexible_type>& _options) {

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

  opt.name           = "side_data_factorization";
  opt.description    = "Include factors for side data.";
  opt.default_value  = true;
  opt.parameter_type = option_handling::option_info::BOOL;
  options.create_option(opt);

  opt.name           = "random_seed";
  opt.description    = "Random seed to use for the model.";
  opt.default_value  = 0;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound    = 0;
  opt.upper_bound    = std::numeric_limits<flex_int>::max();
  options.create_option(opt);

  std::vector<std::string> option_creation_flags;

  if(include_ranking_options())
    option_creation_flags.push_back("ranking");

  factorization::factorization_model::add_options(options, option_creation_flags);

  // Set user specified options
  options.set_options(_options);

  // Save options to state variable
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));

}


/**
 * Takes two datasets for training.
 * \param[in] training_data_by_user ML-Data sorted by user
 * \param[in] training_data_by_item ML-Data sorted by item
 * \param[in] ranking (True/False)
 */
std::map<std::string, flexible_type>
recsys_factorization_model_base::train(const v2::ml_data& training_data_by_user,
                                       const v2::ml_data& training_data_by_item) {

  std::map<std::string, flexible_type> cur_options = get_current_options();
  logprogress_stream << "Training " << name() << " for recommendations." << std::endl;
  table_printer table( {{"Parameter", 28}, {"Description", 48}, {"Value", 8},  } );

  table.print_header();
  table.print_row("num_factors",
                  "Factor Dimension",
                  size_t(cur_options.at("num_factors")));
  table.print_row("regularization",
                  "L2 Regularization on Factors",
                  double(cur_options.at("regularization")));
  table.print_row("max_iterations",
                  "Maximum Number of Iterations",
                  size_t(cur_options.at("max_iterations")));
  table.print_row("solver",
                  "Solver used for training",
                  cur_options.at("solver"));
  table.print_footer();


  // Solve by ALS
  if (include_ranking_options()){
    model = als::implicit_als(training_data_by_user,
                                training_data_by_item, cur_options);
  } else {
    model = als::als(training_data_by_user,
                                training_data_by_item, cur_options);
  }

  // Get the training state
  std::map<std::string, variant_type> coefficients = model->get_coefficients();
  std::map<std::string, variant_type> training_stats = model->get_training_stats();

  add_or_update_state(
      { {"coefficients", to_variant(coefficients)},
        {"training_stats", to_variant(training_stats)} });

  // This will get pulled out at a later date.
  return std::map<std::string, flexible_type>();
}

std::map<std::string, flexible_type>
recsys_factorization_model_base::train(const v2::ml_data& training_data) {

  std::map<std::string, flexible_type> cur_options = get_current_options();
  std::map<std::string, flexible_type> default_options = options.get_default_options();

  // Resolve some of the options

  if(cur_options.at("solver") == "auto") {
    if(training_data.num_columns() == 2
       || (bool(cur_options.at("side_data_factorization")) == false)
       || cur_options.at("num_factors") == 0) {  // adagrad with a pure linear model is broken.

      cur_options["solver"] = "sgd";
    } else {
      cur_options["solver"] = "adagrad";
    }
  }

  logprogress_stream << "Training " << name() << " for recommendations." << std::endl;

  table_printer table( {{"Parameter", 30}, {"Description", 48}, {"Value", 8},  } );

#ifndef NDEBUG
  bool force_print = true;
#else
  bool force_print = false;
#endif

  table.print_header();

  table.print_row("num_factors",
                  "Factor Dimension",
                  size_t(cur_options.at("num_factors")));

  table.print_row("regularization",
                  "L2 Regularization on Factors",
                  double(cur_options.at("regularization")));

  table.print_row("solver",
                  "Solver used for training",
                  cur_options.at("solver"));

  if(force_print || cur_options.at("linear_regularization") != 0) {
    table.print_row("linear_regularization",
                    "L2 Regularization on Linear Coefficients",
                    double(cur_options.at("linear_regularization")));
  }

  if(include_ranking_options()
      && (force_print || cur_options.at("ranking_regularization") != 0)) {

    if(force_print || training_data.has_target()) {
      table.print_row("ranking_regularization",
                      "Rank-based Regularization Weight",
                      double(cur_options.at("ranking_regularization")));
    }

    if(force_print
       || (cur_options.at("unobserved_rating_value") != std::numeric_limits<double>::lowest()
           && training_data.has_target())) {

      table.print_row("unobserved_rating_value",
                      "Ranking Target Rating for Unobserved Interactions",
                      double(cur_options.at("unobserved_rating_value")));
    }

    if(force_print
       || (cur_options.at("num_sampled_negative_examples")
           != default_options.at("num_sampled_negative_examples"))) {

         table.print_row("num_sampled_negative_examples",
                         "# Negative Samples Considered per Observation",
                         double(cur_options.at("num_sampled_negative_examples")));
    }
  }

  if(force_print || cur_options.at("nmf")) {
    table.print_row("nmf",
                    "Use Non-Negative Factors",
                    bool(cur_options.at("nmf")));
  }

  if(force_print || cur_options.at("binary_target")) {
    table.print_row("binary_target",
                    "Assume Binary Targets",
                    bool(cur_options.at("binary_target")));
  }

  if(force_print || training_data.has_side_features()) {

    table.print_row("side_data_factorization",
                    "Assign Factors for Side Data",
                    bool(cur_options.at("side_data_factorization")));
  }

  if(force_print || cur_options.at("sgd_step_size") != 0) {
    table.print_row("sgd_step_size",
                    "Starting SGD Step Size",
                    double(cur_options.at("sgd_step_size")));
  }

  table.print_row("max_iterations",
                  "Maximum Number of Iterations",
                  size_t(cur_options.at("max_iterations")));

  table.print_footer();


  std::string factor_mode = (
      cur_options.at("side_data_factorization")
      ? "factorization_machine"
      : "matrix_factorization");

  model = factorization::factorization_model::factory_train(factor_mode, training_data, cur_options);


  // Get the training state

  std::map<std::string, variant_type> coefficients = model->get_coefficients();

  std::map<std::string, variant_type> training_stats = model->get_training_stats();

  add_or_update_state(
      { {"coefficients", to_variant(coefficients)},
        {"training_stats", to_variant(training_stats)} });

  // This will get pulled out at a later date.
  return std::map<std::string, flexible_type>();
}

////////////////////////////////////////////////////////////////////////////////

sframe recsys_factorization_model_base::predict(const v2::ml_data& test_data) const {
  return model->predict(test_data);
}

////////////////////////////////////////////////////////////////////////////////

sframe recsys_factorization_model_base::get_similar_items(
    std::shared_ptr<sarray<flexible_type> > items, size_t k) const {

  if (options.value("num_factors") == 0) {
    log_and_throw("get_similar_items requires models trained with num_factors > 0.");
  }

  return get_similar(ITEM_COLUMN_INDEX, items, k);
}

sframe recsys_factorization_model_base::get_similar_users(
    std::shared_ptr<sarray<flexible_type> > users, size_t k) const {

  if (options.value("num_factors") == 0) {
    log_and_throw("get_similar_users requires models trained with num_factors > 0.");
  }

  return get_similar(USER_COLUMN_INDEX, users, k);
}

sframe recsys_factorization_model_base::get_similar(
    size_t column_index, std::shared_ptr<sarray<flexible_type> > query, size_t k) const {

  if(_get_similar_buffers.empty()) {
    std::lock_guard<mutex> lg(_get_similar_buffers_lock);

    if(_get_similar_buffers.empty()) {
      _get_similar_buffers.resize(thread::cpu_count());
    }
  }

  return _create_similar_sframe(
      column_index, query, k,
      [&](size_t query_idx, std::vector<std::pair<size_t, double> >& idx_dist_dest) {

        auto& similarities = _get_similar_buffers[thread::thread_id()];

        model->calculate_intracolumn_similarity(similarities, column_index, query_idx);

        idx_dist_dest.resize(similarities.size());
        for(size_t j = 0; j < size_t(similarities.size()); ++j) {
          double v = std::isfinite(similarities[j]) ? similarities[j] : -1.0;
          idx_dist_dest[j] = {j, v};
        }
      });
}

////////////////////////////////////////////////////////////////////////////////

void recsys_factorization_model_base::get_item_similarity_scores(
    size_t item, std::vector<std::pair<size_t, double> >& sim_scores) const {

  model->get_item_similarity_scores(item, sim_scores);
}

////////////////////////////////////////////////////////////////////////////////


void recsys_factorization_model_base::score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::vector<std::pair<size_t, double> >& user_item_list,
      const std::vector<std::pair<size_t, double> >& new_user_item_data,
      const std::vector<v2::ml_data_row_reference>& new_observation_data,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const {

  model->score_all_items(scores, query_row, top_k, known_side_features);
}

////////////////////////////////////////////////////////////////////////////////

void recsys_factorization_model_base::internal_save(turi::oarchive& oarc) const {
  oarc << model;

  bool has_nearest_items_model = false;
  oarc << has_nearest_items_model;
}

////////////////////////////////////////////////////////////////////////////////

void recsys_factorization_model_base::internal_load(turi::iarchive& iarc,
                                                size_t version) {
  iarc >> model;

  bool has_nearest_items_model;
  iarc >> has_nearest_items_model;

  if (has_nearest_items_model) {
    auto nearest_items_model = std::make_shared<nearest_neighbors::brute_force_neighbors>();
    iarc >> *nearest_items_model;
  }

  // Version 0: GLC 1.0, 1.0.1
  // Version 1: GLC 1.1
  if(version == 0) {

    // Create a new option for solver
    if (include_ranking_options() == true){
      option_handling::option_info opt;
      opt.name = "solver";
      opt.description = "The optimization to use for the problem.";
      opt.default_value = "auto";
      opt.parameter_type = option_handling::option_info::CATEGORICAL;
      opt.allowed_values = {"auto", "sgd", "ials", "adagrad"};
      options.create_option(opt);


      opt.name = "ials_confidence_scaling_type";
      opt.description = ("The functional relationship between the preferences"
                         " and the confidence in implcit matrix factorization.");
      opt.default_value = "auto";
      opt.parameter_type = option_handling::option_info::CATEGORICAL;
      opt.allowed_values = {"auto", "log", "linear"};
      options.create_option(opt);

      opt.name = "ials_confidence_scaling_factor";
      opt.description = ("The multiplier for the confidence scaling function for"
                         "Implcit matrix factorization.");

      opt.default_value = 1;
      opt.parameter_type = option_handling::option_info::REAL;
      opt.lower_bound = 1;
      opt.upper_bound = std::numeric_limits<int>::max();
      options.create_option(opt);

    } else {
      option_handling::option_info opt;
      opt.name = "solver";
      opt.description = "The optimization to use for the problem.";
      opt.default_value = "auto";
      opt.parameter_type = option_handling::option_info::CATEGORICAL;
      opt.allowed_values = {"auto", "sgd", "als", "adagrad"};
      options.create_option(opt);
    }

    // Set the option in the options manager and state
    options.set_option("solver", options.value("optimization_method"));
    state["solver"] = options.value("optimization_method");

    // Remove the old option from state and options-manager
    options.delete_option("optimization_method");
    state.erase("optimization_method");
  }
}

}}
