/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <toolkits/factorization/factorization_model.hpp>
#include <toolkits/sgd/basic_sgd_solver.hpp>
#include <toolkits/sgd/sgd_interface.hpp>

////////////////////////////////////////////////////////////////////////////////
// Option handling for the factorization machine model

namespace turi { namespace factorization {

/** Call the following function to instantiate the options needed for
 *  the factorization_model class.
 *
 *  This does not include the options needed for the sgd
 *
 *  The option_flags parameter is used to control what options are
 *  enabled and what the factorization_model class is expected to
 *  support.  Possible flags are as follows:
 *
 *  ranking: Include options for ranking-based optimization.  This
 *  is required for implicit rating optimization.
 *
 *  \param[in,out] options The option manager to add the options to.
 *
 *  \param[in] option_flags The functionality that the
 *  factorization_model class is expected to support.
 *
 *  This function is defined in factorization_model_options.cpp.
 */
void factorization_model::add_options(
    option_manager& options,
    const std::vector<std::string>& option_flags) {

  std::set<std::string> included_options(option_flags.begin(), option_flags.end());

  option_handling::option_info opt;

  ////////////////////////////////////////////////////////////
  // Regularization and other model parameters

  opt.name = "binary_target";
  opt.description = "Whether to assume the targets are binary or real-valued.";
  opt.default_value = false;
  opt.parameter_type = option_handling::option_info::BOOL;
  options.create_option(opt);

  opt.name = "regularization_type";
  opt.description = "The type of the regularization; can be 'normal' or 'weighted'.";
  opt.default_value = "normal";
  opt.parameter_type = option_handling::option_info::CATEGORICAL;
  opt.allowed_values = {"normal", "weighted"};
  options.create_option(opt);

  opt.name = "num_factors";
  opt.description = "The number of factors to use in fitting the factorization model.";
  opt.default_value = 8;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<flex_int>::max();
  options.create_option(opt);

  opt.name = "nmf";
  opt.description = ("If true, turns off the linear and intercept terms and forces "
                     "the latent factors to be non-negative.");
  opt.default_value = false;
  opt.parameter_type = option_handling::option_info::BOOL;
  options.create_option(opt);

  ////////////////////////////////////////////////////////////
  // General Optimization parameters


  opt.name = "init_random_sigma";
  opt.description = "The random spread to use in initializing the state.";
  opt.default_value = 1e-2;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<double>::max();
  options.create_option(opt);

  opt.name = "linear_regularization";
  opt.description = "The value of the regularization parameter for the linear terms.";
  opt.default_value = 1e-10;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<double>::max();
  options.create_option(opt);

  opt.name = "max_iterations";
  opt.description = ("The maximum number of iterations.");
  opt.default_value = 50;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<flex_int>::max();
  options.create_option(opt);


  opt.name = "num_tempering_iterations";
  opt.description = ("The stability of the optimization benefits from several iterations "
                     "with a stronger regularization value; the regularization is decreased "
                     "to the correct value over this many iterations.");
  opt.default_value = 4;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<long>::max();
  options.create_option(opt);

  opt.name = "tempering_regularization_start_value";
  opt.description = "The starting regularization value of the tempering schedule.";
  opt.default_value = 1e-8;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<double>::max();
  options.create_option(opt);

  opt.name = "adagrad_momentum_weighting";
  opt.description = ("A smoothing step is performed on the adagrad gradients with "
                     "each iteration for stability.  This value is the weighting of the "
                     "current gradients against the mean.");
  opt.default_value = 0.9;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = 1;
  options.create_option(opt);

  if(included_options.count("ranking")) {

    opt.name = "regularization";
    opt.description = "The value of the regularization parameter for the interaction terms.";
    opt.default_value = 1e-10;
    opt.parameter_type = option_handling::option_info::REAL;
    opt.lower_bound = 0;
    opt.upper_bound = std::numeric_limits<double>::max();
    options.create_option(opt);

    opt.name = "ranking_regularization";
    opt.description = ("The amount of penalization applied for each negative sample,"
                       " as a weight against the positive samples.");
    opt.default_value = 0.1;
    opt.parameter_type = option_handling::option_info::REAL;
    opt.lower_bound = 0;
    opt.upper_bound = std::numeric_limits<double>::max();
    options.create_option(opt);

    opt.name = "unobserved_rating_value";
    opt.description = ("When ranking_regularization is enabled, unobserved items"
                       "are penalized if their predicted value is larger than this value. "
                       "The strength of this value is determined by ranking_regularization.");
    opt.parameter_type = option_handling::option_info::REAL;
    opt.default_value = std::numeric_limits<double>::lowest();
    opt.lower_bound = std::numeric_limits<double>::lowest();
    opt.upper_bound = std::numeric_limits<double>::max();
    options.create_option(opt);

    opt.name = "num_sampled_negative_examples";
    opt.description = ("The number of unobserved items to sample when ranking loss or "
                       "ranking_regularization is enabled.");
    opt.default_value = 4;
    opt.parameter_type = option_handling::option_info::INTEGER;
    opt.lower_bound = 1;
    opt.upper_bound = std::numeric_limits<int>::max();
    options.create_option(opt);

    // opt.name = "_ranking_loss_type";
    // opt.description = "The type of ranking loss to use; options are hinge or exponential.";
    // opt.default_value = "logit";
    // opt.parameter_type = option_handling::option_info::CATEGORICAL;
    // opt.allowed_values = {"hinge", "logit", "logistic"};
    // options.create_option(opt);

    opt.name = "solver";
    opt.description = "The optimization method to use for the problem.";
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
                       "implicit matrix factorization.");
    opt.default_value = 1;
    opt.parameter_type = option_handling::option_info::REAL;
    opt.lower_bound = 1e-10;
    opt.upper_bound = std::numeric_limits<int>::max();
    options.create_option(opt);

  } else {

    opt.name = "regularization";
    opt.description = "The value of the regularization parameter for the interaction terms.";
    opt.default_value = 1e-6;
    opt.parameter_type = option_handling::option_info::REAL;
    opt.lower_bound = 0;
    opt.upper_bound = std::numeric_limits<double>::max();
    options.create_option(opt);

    opt.name = "solver";
    opt.description = "The optimization to use for the problem.";
    opt.default_value = "auto";
    opt.parameter_type = option_handling::option_info::CATEGORICAL;
    opt.allowed_values = {"auto", "sgd", "als", "adagrad"};
    options.create_option(opt);
  }

  // Add in the options for the sgd solver.
  sgd::basic_sgd_solver<sgd::sgd_interface_base>::add_options(options);
}

}}
