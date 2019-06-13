/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/factorization/factorization_model_sgd_interface.hpp>
#include <toolkits/sgd/basic_sgd_solver.hpp>
#include <toolkits/factorization/ranking_sgd_solver_implicit.hpp>
#include <toolkits/factorization/ranking_sgd_solver_explicit.hpp>
#include <toolkits/factorization/model_factory.hpp>

namespace turi { namespace factorization {

/** Create and train a factorization model with the given options.
 *
 *  The training method constructs an
 *
 *  \param[in] factor_mode One of "linear_model",
 *  "matrix_factorization", or "factorization_model".  If
 *  "linear_model", then the class is essentially linear regression;
 *  if "matrix_factorization", then only the first two columns have
 *  latent factors, and if "factorization_model", then the full
 *  factorization machine model is used.
 *
 *  \param[in] train_data  The training data for the model.
 *
 *  \param[in] options The options used in the current model as well
 *  as training parameters.
 */
std::shared_ptr<factorization_model>
factorization_model::factory_train(
    const std::string& factor_mode,
    const v2::ml_data& train_data,
    std::map<std::string, flexible_type> options) {

  std::shared_ptr<factorization_model> model;
  std::shared_ptr<sgd::sgd_solver_base> solver;

  ////////////////////////////////////////////////////////////////////////////////
  // There are a number of parameters that we need to decide on here;
  // the factory function will internally instantiate the correct
  // solver and correct class.


  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up all the variables to decide things

  // Determine whether we are in ranking mode or not
  bool ranking = (
      options.count("ranking_regularization") && options.at("ranking_regularization") > 0);

  // Are we using implicit data?  If so, handle it with binary targets
  // and logistic data.
  if(!train_data.has_target()) {
    options["binary_target"] = true;
    ranking = true;
  }

  // Set the loss class
  std::string loss_type = (options["binary_target"]
                           ? "loss_logistic"
                           : "loss_squared_error");

  // Set the solver class
  std::string solver_class = (ranking
                              ? "factorization::explicit_ranking_sgd_solver"
                              : "sgd::basic_sgd_solver");

  // Set the regularization type
  std::string raw_regularization_type = options.at("regularization_type");

  std::string regularization_type;

  if(raw_regularization_type == "normal")
    regularization_type = "L2";
  else if(raw_regularization_type == "weighted")
    regularization_type = "ON_THE_FLY";
  else
    ASSERT_TRUE(false);

  if(!(options.at("regularization") > 0
       || options.at("linear_regularization") > 0)) {

    regularization_type = "NONE";
  }

  // Set the number of factors
  flex_int num_factors = options.at("num_factors");

  // Create the model
  std::tie(model, solver) = create_model_and_solver(
      train_data, options,
      loss_type, solver_class, regularization_type, factor_mode, num_factors);


  // Run the solver
  model->_training_stats = solver->run();

  return model;
}

}}
