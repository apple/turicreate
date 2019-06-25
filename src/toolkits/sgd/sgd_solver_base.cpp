/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <map>
#include <cmath>
#include <tuple>
#include <vector>
#include <limits>
#include <core/util/code_optimization.hpp>
#include <toolkits/sgd/sgd_solver_base.hpp>
#include <core/logging/assertions.hpp>
#include <core/logging/table_printer/table_printer.hpp>

#include <toolkits/ml_data_2/ml_data.hpp>

namespace turi { namespace sgd {

////////////////////////////////////////////////////////////////////////////////

void sgd_solver_base::add_options(option_manager& options) {

  option_handling::option_info opt;

  opt.name = "sgd_step_size";
  opt.description = ("The step size to use for the stochastic gradient methods. "
                     "Note that different algorithms treat this parameter differently. "
                     "If zero (default), the step size will be chosen automatically.");
  opt.default_value = 0;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<double>::max();
  options.create_option(opt);

  opt.name = "step_size_decrease_rate";
  opt.description = ("The step size for sgd decreases at this rate; specifically, the step size "
                     "is equal to the original stepsize times n^(-r), where n is the number of "
                     "iterations through the data and r is this value.");
  opt.default_value = 0.75;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0.5;
  opt.upper_bound = 1;
  options.create_option(opt);

  opt.name = "sgd_convergence_threshold";
  opt.description = ("Convergence is tested using variation in the training loss.  When the "
                     "loss does not fall more than this much in "
                     "convergence_interval passes through the data, we stop.");
  opt.default_value = 1e-5;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<double>::max();
  options.create_option(opt);

  opt.name = "sgd_convergence_interval";
  opt.description = ("When the loss has not improved by convergence_threshold"
                     "in this number of passes through the data, break. ");
  opt.default_value = 4;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 4;
  opt.upper_bound = std::numeric_limits<int>::max();
  options.create_option(opt);

  opt.name = "sgd_step_adjustment_interval";
  opt.description = ("When the overall loss has not decreased in this many "
                     "iterations, decrease the step size.");
  opt.default_value = 4;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<int>::max();
  options.create_option(opt);

  opt.name = "sgd_trial_sample_proportion";
  opt.description = ("The proportion of training size to use in a trial dataset when "
                     "setting the sgd step size automatically.");
  opt.default_value = 0.125;
  opt.parameter_type = option_handling::option_info::REAL;
  opt.lower_bound = 0;
  opt.upper_bound = 1;
  options.create_option(opt);

  opt.name = "sgd_trial_sample_minimum_size";
  opt.description = ("The number of observations to use in a trial dataset when"
                     " setting the sgd step size automatically.");
  opt.default_value = 10000;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 1;
  opt.upper_bound = std::numeric_limits<int>::max();
  options.create_option(opt);

  opt.name = "sgd_max_trial_iterations";
  opt.description = ("The maximum number of iterations to run SGD for on the trial data "
                     "set in determining the step size automatically.");
  opt.default_value = 5;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 1;
  opt.upper_bound = std::numeric_limits<int>::max();
  options.create_option(opt);

  opt.name = "track_exact_loss";
  opt.description = ("If true, track the exact loss function and "
                     "objective along side the approximate versions.");
  opt.default_value = false;
  opt.parameter_type = option_handling::option_info::BOOL;
  options.create_option(opt);

  opt.name = "sgd_sampling_block_size";
  opt.description = ("The SGD algorithm will load approximately this many samples into "
                     "memory, then process them in random order.");
  opt.default_value = 128*1024;
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.lower_bound = 2;
  opt.upper_bound = std::numeric_limits<long>::max();
  options.create_option(opt);

  opt.name = "additional_iterations_if_unhealthy";
  opt.description = ("If the model becomes unhealthy and gets reset, allow at most this many additional "
                     "iterations in an attempt to have max_iterations healthy iterations.");
  opt.parameter_type = option_handling::option_info::INTEGER;
  opt.default_value = 5;
  opt.lower_bound = 0;
  opt.upper_bound = std::numeric_limits<int>::max();
  options.create_option(opt);
}


/** The constructor.
 */
sgd_solver_base::sgd_solver_base(const std::shared_ptr<sgd_interface_base>& iface,
                                 const v2::ml_data& _train_data,
                                 const std::map<std::string, flexible_type>& _options)
    : train_data(_train_data)
    , model_interface(iface)
    , options(_options)
{
  model_interface->setup(train_data, _options);
}

/** The main function to run the sgd solver, given the current
 *  options.  Performs several setup steps, then calls the appropriate
 *  sgd routine below.
 */
std::map<std::string, variant_type> sgd_solver_base::run() {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  Figure out which sub-solver to use.  If the specified
  // step size is zero, then we automatically tune the step size.

  double sgd_step_size = options.at("sgd_step_size");

  std::map<std::string, variant_type> ret;

  timer run_timer;

  run_timer.start();

  // Set up the model.
  setup(model_interface.get());

  ////////////////////////////////////////////////////////////////////////////////
  // Run the relevant solver.

  if(sgd_step_size == 0) {
    logprogress_stream << "  Optimizing model using SGD; tuning step size." << std::endl;
    sgd_step_size = compute_initial_sgd_step_size();
  } else {
    logprogress_stream << "  Optimizing model using SGD (step size tuning: OFF)." << std::endl;
  }

  logprogress_stream << "Starting Optimization." << std::endl;
  ret = run_fixed_sgd_step_size(sgd_step_size);

  double seconds = run_timer.current_time();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Cleanup.  Go through and calculate the exact training
  // loss and report that.

  ret["training_time"] = seconds;

  return ret;
}

////////////////////////////////////////////////////////////////////////////////

/** Run the sgd algorithm with a fixed step size.  If divergence is
 *  detected, then retry with a smaller step size and warn the
 *  user.
 *
 *  If the sgd_step_size parameter is set to a value greater than
 *  zero, the run() method above will run this solver.
 */
std::map<std::string, variant_type>
sgd_solver_base::run_fixed_sgd_step_size(double initial_sgd_step_size) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: pull in intial values.

  std::map<std::string, variant_type> ret;

  size_t data_size = train_data.size();
  double l2_regularization = model_interface->l2_regularization_factor();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Check for problems down the road.

  // If this bound is violated, then the tracking of the
  // regularization effect will likely cause numerical issues.  The
  // above value comes from having to compute s *= (1 - reg * step)
  // after every data point; if step_size_bound equals the above, then
  // s will be 1e-32 after one pass.  Bounding this should be enough,
  // as it gets reset to 1 after each pass.
  //
  // Note that if reg * step > 1, the model blows up.

  double step_size_bound = ((1.0 - std::pow(1e-32, 1.0 / (std::max(size_t(1), data_size))))
                            / (std::max(1e-32, l2_regularization)));

  // Also consider the hard limit on the step size bound for numeric
  // stability as given by the interface.
  step_size_bound = std::min(model_interface->max_step_size(), step_size_bound);

  if(initial_sgd_step_size > step_size_bound) {
    logprogress_stream
        << "WARNING: Fixed specified step size is too large to be "
        << "numerically stable with given model / regularization value; "
        << "Setting to " << step_size_bound << "." << std::endl;

    initial_sgd_step_size = step_size_bound;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Init a bunch of tracking variables.

  double base_sgd_step_size = initial_sgd_step_size;
  std::vector<double> sgd_objective_path;

  size_t iteration_index     = 0;
  size_t iteration_count     = 0;
  size_t max_iterations_soft = options.at("max_iterations");
  size_t max_iterations_hard = max_iterations_soft + options.at("additional_iterations_if_unhealthy");
  bool training_finished_due_to_model_convergence = false;

  // In the middle of running it, sometimes the state needs to be reset.
  size_t num_resets = 0;

  double initial_objective_value = NAN, initial_loss = NAN;

  double step_size_decrease_rate = options.at("step_size_decrease_rate");
  bool track_exact_loss          = options.at("track_exact_loss");

  std::string stopping_condition;

  /** Begin by reseting the state to the correct value.
   */
  model_interface->setup_optimization(options.at("random_seed"), /*in_trial_mode*/ false);

  std::tie(initial_objective_value, initial_loss) = get_initial_objective_value(train_data);

  sgd_objective_path = {initial_objective_value};

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up all the return stuff and storage.

  std::vector<std::pair<std::string, size_t> > row_spec;

  if(track_exact_loss) {
    row_spec = {
      {"Iter.", 7},
      {"Elapsed Time", 10},
      {"Approx. Objective", 12},
      {std::string("Approx. Training ") + model_interface->reported_loss_name(), 11},
      {"Exact Objective", 12},
      {std::string("Exact Training ") + model_interface->reported_loss_name(), 11},
      {"Step Size", 11} };
  } else {
    row_spec = {
      {"Iter.", 7},
      {"Elapsed Time", 10},
      {"Approx. Objective", 12},
      {std::string("Approx. Training ") + model_interface->reported_loss_name(), 11},
      {"Step Size", 11} };
  }

  table_printer table(row_spec);

  ret["initial_training_loss"]   = to_variant(initial_loss);
  ret["initial_objective_value"] = to_variant(initial_objective_value);
  ret["sgd_step_size"]           = to_variant(initial_sgd_step_size);
  ret["training_loss_type"]      = to_variant(model_interface->reported_loss_name());
  ret["training_options"]        = to_variant(options);

  // A bizarre special case that causes pain, anguish, and demons to
  // break loose if not gaurded against properly.
  if(data_size == 0) {

    logprogress_stream
        << "WARNING: Training data set empty.  This model will be rather useless." << std::endl;

    ret["num_iterations"]        = to_variant(size_t(0));
    ret["final_objective_value"] = to_variant(initial_objective_value);
    ret["final_training_loss"]   = to_variant(initial_loss);

    return ret;
  }

  table.print_header();
  if(track_exact_loss)
    table.print_row("Initial", progress_time(), "", "", initial_objective_value, initial_loss, "");
  else
    table.print_row("Initial", progress_time(), initial_objective_value, initial_loss, "");

  table.print_line_break();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Declare the reset function.

  /** The reset function.  This gets called to reset an unhealthy
   *  state.
   */
  auto reset_unhealthy_state = [&]() {

    model_interface->setup_optimization(turi::timer::usec_of_day(),
                                        /*in_trial_mode*/ false);

    std::tie(initial_objective_value, initial_loss) = get_initial_objective_value(train_data);

    if(track_exact_loss)
      table.print_row("RESET", progress_time(),
                      "", "", initial_objective_value, initial_loss, "");
    else
      table.print_row("RESET", progress_time(),
                      initial_objective_value, initial_loss, "");

    // Reset the objective path
    sgd_objective_path = {initial_objective_value};

    ++num_resets;

    // Set the "soft" iteration limit back to the start
    iteration_index = 0;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5: Go for it.

  while(true) {

    // Have this at the top so that even if the model diverges, we still stop
    ++iteration_count;

    ////////////////////////////////////////////////////////////////////////////////
    // Step 5.1: Have we done enough rounds yet?

    if(iteration_count > max_iterations_hard) {
      stopping_condition = "Maximum number of passes through the data reached (hard limit).";
      ret["num_iterations"] = to_variant(iteration_count);
      ret["num_healthy_iterations"] = to_variant(iteration_index);
      break;
    }

    if(iteration_index >= max_iterations_soft) {
      stopping_condition = "Maximum number of passes through the data reached.";
      ret["num_iterations"] = to_variant(iteration_count);
      ret["num_healthy_iterations"] = to_variant(iteration_index);
      break;
    }

    if(base_sgd_step_size <= 1e-16) {
      stopping_condition = ("SGD step size is below numerical limits.  "
                            "Please rescale your data or add regularization"
                            " to recondition the problem.");
      ret["num_iterations"] = to_variant(iteration_count);
      ret["num_healthy_iterations"] = to_variant(0UL);
      break;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.2: Get the local step size for this pass through the
    // data.

    double iteration_step_size =
        calculate_iteration_sgd_step_size(
            iteration_index, base_sgd_step_size, step_size_decrease_rate, l2_regularization);


    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.3: Run one iteration on the full data.

    double objective_value_estimate, training_loss;

    std::tie(objective_value_estimate, training_loss)
        = run_iteration(iteration_index,
                        model_interface.get(),
                        train_data,
                        iteration_step_size);

    // Test to see if the model has diverged for any reason
    if(!std::isfinite(objective_value_estimate)) {
      logstream(LOG_INFO) << "SGD: Non-finite objective value estimate detected." << std::endl;
      objective_value_estimate = NAN;
    }

    if(!(objective_value_estimate < 10*initial_objective_value)) {
      logstream(LOG_INFO) << "SGD: Objective value estimate > 10x initial detected." << std::endl;
      objective_value_estimate = NAN;
    }

    // If the objective_value_estimate is greate than 2x the initial,
    // get the exact objective value and try again.
    if(!(objective_value_estimate < 2*initial_objective_value)) {
      logstream(LOG_INFO) << "SGD: Objective value estimate > 2x initial detected; possible divergence."
                          << std::endl;

      double exact_objective_value, exact_loss;

      std::tie(exact_objective_value, exact_loss) = calculate_objective(
          model_interface.get(), train_data, iteration_index);

      if(!(exact_objective_value < initial_objective_value)) {
        logstream(LOG_INFO) << "SGD: Exact objective value estimate > 2x initial detected."
                            << std::endl;
        objective_value_estimate = NAN;
      }
    }

    // If the objective_value_estimate is greate than 2x the initial,
    // get the exact objective value and try again.
    if(!(objective_value_estimate < initial_objective_value)) {
      // See if it's been increasing the last 8 iterations and all
      // values are more than the initial objective.  If so, then we
      // have an issue.

      if(sgd_objective_path.size() >= 5) {

        bool gone_up = true;
        for(size_t i = sgd_objective_path.size() - 4; i < sgd_objective_path.size(); ++i) {
          if(sgd_objective_path[i - 1] > sgd_objective_path[i]) {
            gone_up = false;
            break;
          }
        }

        bool all_greater_than_initial = true;
        for(size_t i = sgd_objective_path.size() - 5; i < sgd_objective_path.size(); ++i) {
          if(sgd_objective_path[i] < initial_objective_value) {
            all_greater_than_initial = false;
            break;
          }
        }

        if(gone_up && all_greater_than_initial) {

          double exact_objective_value, exact_loss;

          std::tie(exact_objective_value, exact_loss) = calculate_objective(
              model_interface.get(), train_data, iteration_index);

          // It is necessary to check the initial training loss here
          // as well; there are some cases in which the adagrad causes
          // the updates of the gradient to become very biased, so the
          // L2 regularization value is not treated correctly.
          // However, the ranking objective still improves and the
          // loss still improves.  This prevents an undesired reset in
          // this case.
          if( (!(exact_objective_value < initial_objective_value) )
              && !(exact_loss < initial_loss) ) {

            logstream(LOG_INFO) << "SGD: Objective value estimate increasing over the last 5 iterations "
                                << "and greater than initial; resetting."
                                << std::endl;

            objective_value_estimate = NAN;
          }
        }
      }
    }

    if(!model_interface->state_is_numerically_stable()) {
      logstream(LOG_INFO) << "SGD: model failed numerical stability test." << std::endl;
      objective_value_estimate = NAN;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.4: Check for errors.  NAN gets returned if a numerical
    // error in the model occured, as measured by several
    // method-dependent and model-dependent checks in the
    // run_iteration function.

    if(!std::isfinite(objective_value_estimate)) {

      if(track_exact_loss)
        table.print_row(iteration_index + 1, progress_time(),
                        "DIVERGED", "DIVERGED","DIVERGED", "DIVERGED", iteration_step_size);
      else
        table.print_row(iteration_index + 1, progress_time(),
                        "DIVERGED", "DIVERGED",iteration_step_size);

      base_sgd_step_size = iteration_step_size / 2;

      ret["sgd_step_size"] = to_variant(base_sgd_step_size);

      reset_unhealthy_state();

      continue;

    } else {

      if(track_exact_loss) {
        double exact_objective_value, exact_loss;

        std::tie(exact_objective_value, exact_loss) = calculate_objective(
            model_interface.get(), train_data, iteration_index);

        table.print_progress_row(iteration_count,
                                 iteration_index + 1, progress_time(), objective_value_estimate,
                                 training_loss,
                                 exact_objective_value,
                                 exact_loss,
                                 iteration_step_size);

      } else {
        table.print_progress_row(iteration_count,
                                 iteration_index + 1, progress_time(), objective_value_estimate,
                                 training_loss,
                                 iteration_step_size);
      }

      sgd_objective_path.push_back(objective_value_estimate);
      base_sgd_step_size = sgd_adjust_step_size(sgd_objective_path, base_sgd_step_size);
    }

    ////////////////////////////////////////////////////////////
    // Step 2.5: Check for convergence.  Either by the path not
    // decreasing recently, or the objective being really close to
    // zero.
    {

      if(objective_value_estimate < 1e-16) {
        stopping_condition = "Model appears to be exactly solvable.";
        ret["num_iterations"] = to_variant(iteration_count);
        ret["num_healthy_iterations"] = to_variant(iteration_index+1);
        training_finished_due_to_model_convergence = true;
        break;
      }

      bool is_converged = sgd_test_model_convergence(sgd_objective_path);

      if(is_converged) {
        stopping_condition = "Convergence on objective within bounds.";
        ret["num_iterations"] = to_variant(iteration_count);
        ret["num_healthy_iterations"] = to_variant(iteration_index+1);
        training_finished_due_to_model_convergence = true;
        break;
      }
    }

    ////////////////////////////////////////////////////////////
    // Finally, increment the iteration_index, which measures the
    // number of healthy iterations.  It's at the end as continue is
    // called when there is an error.

    ++iteration_index;
  }

  table.print_footer();


  // This can trigger falsely, as the final_training_loss can be on a
  // different scale then the objective value.

  // if(final_objective_value > 3 * final_training_loss) {
  //   logstream(LOG_WARNING)
  //       << ("A significant part of your objective function is due to your "
  //           "regularization penalty.  Consider decreasing your regularization value "
  //           "to better fit the data.");
  // }

  logprogress_stream << "Optimization Complete: " << stopping_condition << std::endl;
  logprogress_stream << "Computing final objective value and training "
                     << model_interface->reported_loss_name() << "." << std::endl;

  double final_objective_value, final_training_loss;
  std::tie(final_objective_value, final_training_loss)
      = calculate_objective(model_interface.get(), train_data, iteration_index);

  ret["final_objective_value"] = to_variant(final_objective_value);
  ret["final_training_loss"]   = to_variant(final_training_loss);

  logprogress_stream << "       Final objective value: " << final_objective_value << std::endl;
  logprogress_stream << "       Final training " << model_interface->reported_loss_name()
                     << ": " << final_training_loss << std::endl;



  ////////////////////////////////////////////////////////////////////////////////
  // Model health scores

  {
    ////////////////////////////////////////////////////////////
    // Positive:

    // A perfect score: sees decrease

    std::map<std::string, variant_type> health;

    ////////////////////////////////////////////////////////////
    // Calculate monotonicity
    double optimization_stability_score = 0.0;
    double optimization_stability_score_num = 0; // For the average

    {
      size_t monotonicity_count = 0;
      for(size_t i = 0; i < sgd_objective_path.size() - 1; ++i) {
        if(sgd_objective_path[i] > sgd_objective_path[i + 1])
          ++monotonicity_count;
        else
          break;
      }

      double monotonicity = double(monotonicity_count) / std::max(size_t(1), sgd_objective_path.size() - 1);
      health["monotonicity"] = to_variant(monotonicity);

      optimization_stability_score += monotonicity;
      optimization_stability_score_num += 1;
    }

    {
      health["num_resets"] = to_variant(num_resets);

      double healthy_iteration_proportion
          = (training_finished_due_to_model_convergence
             ? 1.0
             : (double(sgd_objective_path.size()) - 1.0) / max_iterations_soft);

      health["healthy_iteration_proportion"]
          = to_variant(healthy_iteration_proportion);

      optimization_stability_score += healthy_iteration_proportion;
      optimization_stability_score_num += 1;
    }

    bool decreasing_objective = (final_objective_value < initial_objective_value);
    health["decreasing_objective"] = to_variant(decreasing_objective);

    if(sgd_objective_path.size() >= 2) {
      bool decreasing_objective_initial = (sgd_objective_path[1] < initial_objective_value);
      health["decreasing_objective_initial"] = to_variant(decreasing_objective_initial);

      optimization_stability_score += (decreasing_objective_initial ? 1.0 : 0);
      optimization_stability_score_num += 1.0;
    }

    {
      // Get the proportional decrease of the final relative to the minimum
      double min_score = *std::min_element(sgd_objective_path.begin(), sgd_objective_path.end());
      min_score = std::min(final_objective_value, min_score);

      double relative_decrease_proportion
          = std::max(0.0,
                     ((initial_objective_value - final_objective_value)
                      / (initial_objective_value - min_score)));

      health["relative_decrease_proportion"] = to_variant(relative_decrease_proportion);

      optimization_stability_score += relative_decrease_proportion;
      optimization_stability_score_num += 1.0;
    }

    optimization_stability_score /= optimization_stability_score_num;

    ret["training_stability"]       = to_variant(health);
    ret["training_stability_score"] = to_variant(optimization_stability_score);

  }

  ////////////////////////////////////////////////////////////////////////////////
  // Get the progress statistics

  sframe iteration_training_stats = table.get_tracked_table();

  // Need to rename the names consistently

  if(track_exact_loss) {
    DASSERT_EQ(iteration_training_stats.num_columns(), 7);

    iteration_training_stats.set_column_name(0, "iteration");
    iteration_training_stats.set_column_name(1, "elapsed_seconds");
    iteration_training_stats.set_column_name(2, "approximate_objective");
    iteration_training_stats.set_column_name(3, "approximate_training_loss");
    iteration_training_stats.set_column_name(4, "exact_objective");
    iteration_training_stats.set_column_name(5, "exact_training_loss");
    iteration_training_stats.set_column_name(6, "sgd_step_size");
  } else {
    DASSERT_EQ(iteration_training_stats.num_columns(), 5);

    iteration_training_stats.set_column_name(0, "iteration");
    iteration_training_stats.set_column_name(1, "elapsed_seconds");
    iteration_training_stats.set_column_name(2, "approximate_objective");
    iteration_training_stats.set_column_name(3, "approximate_training_loss");
    iteration_training_stats.set_column_name(4, "sgd_step_size");
  }

  std::shared_ptr<unity_sframe> lt_sf(new unity_sframe);
  lt_sf->construct_from_sframe(iteration_training_stats);

  ret["progress"] = to_variant(lt_sf);

  return ret;
}

/** Run the sgd algorithm with step size tuning to find the best
 *  value.
 *
 *  If the sgd_step_size parameter is set to zero, the run() method
 *  above will run this solver.
 *
 *  The tuning is done by first running the model on a small subset
 *  of the data with several different step sizes.  The best step
 *  size is chosen as the step size for running the full algorithm.
 */
double sgd_solver_base::compute_initial_sgd_step_size() {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Find a reasonable step size starting point for the data
  // to choose it automatically.

  const size_t data_size = train_data.size();

  // A bizarre special case that causes pain, anguish, and demons to
  // break loose if not gaurded against properly.
  if(data_size == 0)
    return 1;

  const double l2_regularization = model_interface->l2_regularization_factor();

  // Run the above for a number of different step sizes.
  // Heuristically, it is generally not good to choose a step size
  // that is larger than the radius of the data.  In the linear
  // regression context with the current implemented scaling, the
  // radius of the data is approximately the number of features in
  // each observation.  We'll use approximate upper limit as the
  // starting point, then decrease by factors of 4.  However, this
  // also tends to be really conservative of a bound, and our testing
  // method fails quickly on error, so it's fine to overestimate this
  // part.

  double step_size_bound_theory = 0.5 / std::max(size_t(1), train_data.num_columns());

  double theory_suggested_step_size_bound = std::max(1.0, 100*step_size_bound_theory);


  // The other bound that we consider is based on the step size.
  // Based on the regularization value, we do not want the sgd scaling
  // parameters decreasing too quickly.  This bound says that the l2
  // regularization drives an otherwise unaffected parameter to 1e-32
  // of its original value on each iteration; i.e.
  // (1-s * lm)^data_size == 1e-32. This was chosen for numeric
  // stability.

  double step_size_bound_numerical_stability =
    ((1.0 - std::pow(1e-32, 1.0 / std::max(size_t(1), data_size)))
     / (std::max(1e-32, l2_regularization)));

  // Also consider the hard limit on the step size bound for numeric
  // stability as given by the interface.
  step_size_bound_numerical_stability = std::min(model_interface->max_step_size(),
                                                 step_size_bound_numerical_stability);


  // Give it a little extra room just in case -- usually, in the case
  // of numerical failure from too large a step size, it fails
  // quickly.
  double step_size_search_start_point  = std::min(theory_suggested_step_size_bound,
                                                  step_size_bound_numerical_stability);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: We are doing this loop in two stages.  The first stage is
  // to find a good step size on a subset of the data, and the second
  // is to run it with that step size on the data.

  size_t max_iterations                = options.at("max_iterations");
  double step_size_decrease_rate       = options.at("step_size_decrease_rate");

  size_t sgd_trial_sample_minimum_size = options.at("sgd_trial_sample_minimum_size");
  double sgd_trial_sample_proportion   = options.at("sgd_trial_sample_proportion");
  size_t max_trial_iterations          = options.at("sgd_max_trial_iterations");

  size_t n_iterations = std::min(max_iterations, max_trial_iterations);

  // Set the number of trial sample iterations based on the dataset size.
  size_t n_trial_sample_observations = std::min(
      data_size,
      std::max(sgd_trial_sample_minimum_size,
               size_t(data_size * sgd_trial_sample_proportion)));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Get a test data set

  v2::ml_data trial_data = train_data.create_subsampled_copy(n_trial_sample_observations,
                                                             options.at("random_seed"));
  /** Begin by reseting the state to the correct value.
   */
  size_t random_seed = options.at("random_seed");
  model_interface->setup_optimization(random_seed, /*in_trial_mode*/ true);

  double test_initial_objective_value, test_initial_loss;
  std::tie(test_initial_objective_value, test_initial_loss)
      = get_initial_objective_value(trial_data);

  logprogress_stream << "  Using " << trial_data.size() << " / "
                     << data_size << " points for tuning the step size." << std::endl;

  // Init all the stuff for the optimization
  double best_objective_value  = std::numeric_limits<double>::max();
  double current_sgd_step_size = step_size_search_start_point;
  double best_sgd_step_size    = step_size_search_start_point;

  size_t minimum_is_at_try_index = 0;

  table_printer sgd_table( {{"Attempt", 0}, {"Initial Step Size", 0}, {"Estimated Objective Value", 40}});

  sgd_table.print_header();

  for(size_t try_index = 0; current_sgd_step_size >= 1e-12; ++try_index) {

    // Reset the state to the old starting point.
    model_interface->setup_optimization(random_seed, /*trial mode*/ true);

    bool step_size_is_viable = true;

    double step_size = current_sgd_step_size;

    for(size_t iteration = 0; iteration < n_iterations; ++iteration) {

      double iteration_step_size =
          calculate_iteration_sgd_step_size(
              iteration, step_size, step_size_decrease_rate, l2_regularization);

      // Get the local step size.
      double this_value = run_iteration(
          iteration, model_interface.get(), trial_data, iteration_step_size).first;

      // std::cout << __FILE__ << __LINE__ << "this_value = " << this_value << std::endl;

      // Check for errors
      if(!std::isfinite(this_value)
         || this_value == std::numeric_limits<double>::max()
         || !(std::fabs(this_value) < 10*test_initial_objective_value)
         || !model_interface->state_is_numerically_stable() )
      {
        step_size_is_viable = false;
        break;
      }
    }

    if(step_size_is_viable) {
      double training_loss, trial_objective_value;

      std::tie(trial_objective_value, training_loss) =
          calculate_objective(model_interface.get(), trial_data, n_iterations-1);

      if(!(trial_objective_value < test_initial_objective_value)) {

        std::ostringstream ss;
        ss << "No Decrease ("
           << trial_objective_value << " >= "
           << test_initial_objective_value << ")";

        sgd_table.print_row(try_index, current_sgd_step_size, ss.str());
        current_sgd_step_size *= 0.25;
        step_size_is_viable = false;
      } else {

        sgd_table.print_row(try_index, step_size, trial_objective_value);

        // Is there a chance that this one is better than the
        // previous ones?

        if(trial_objective_value <= 1e-16) {
          best_objective_value = trial_objective_value;
          best_sgd_step_size = current_sgd_step_size;
          break;
        }

        if(trial_objective_value < best_objective_value) {
          best_objective_value = trial_objective_value;
          best_sgd_step_size = current_sgd_step_size;
          minimum_is_at_try_index = try_index;
        } else {

          // Break if we're clearly out of range.
          if (0.5 * trial_objective_value >= best_objective_value
              || minimum_is_at_try_index + 3 <= try_index)
          {
            break;
          }
        }

        current_sgd_step_size *= 0.5;
      }
    } else {

      sgd_table.print_row(try_index, current_sgd_step_size, "Not Viable");
      current_sgd_step_size *= 0.25;
    }

    // Test for the case where we have had a decent step size, but
    // then a string of bad step sizes after that.  Typically happens
    // when the step size gets to a point where no decrease is
    // registered.
    if(!step_size_is_viable) {

      // Break if we've had a value that works; otherwise, we should
      // keep going.
      if (best_objective_value < test_initial_objective_value
          && minimum_is_at_try_index + 3 <= try_index) {

        break;
      }
    }
  }

  // We are in a bad place.
  if(best_objective_value == std::numeric_limits<double>::max()) {

    // Choose a reasonably conservative value; the rest of the
    // algorithm may lower this further if it still causes
    // numerical issues.
    best_sgd_step_size = std::min(0.005, step_size_search_start_point * 1e-2);

    sgd_table.print_line_break();
    sgd_table.print_row("Final", best_sgd_step_size, "Unknown");
    sgd_table.print_footer();

    logprogress_stream
        << ("WARNING: Having difficulty finding viable stepsize; Model may be at optimum. "
            "Continuing with small step size.")
        << std::endl;

  } else {

    sgd_table.print_line_break();
    sgd_table.print_row("Final", best_sgd_step_size, best_objective_value);
    sgd_table.print_footer();
  }

  return best_sgd_step_size;
}

/** Called to calculate the current objective value for the data.
 *  Defaults to calling calculate_loss() +
 *  current_regularizer_value() in the current interface.  the
 *  function to get the current regularization term; however, can be
 *  overridden if need be.  (For example, for optimizing ranking
 *  functions, the loss function doesn't fit into the standard
 *  framework laid out by the model's calculate_fx function.
 *
 *  \return (objective value, reportable training loss)
 */
std::pair<double, double> sgd_solver_base::calculate_objective(
    sgd_interface_base* iface, const v2::ml_data& data, size_t iteration) const {

  if(data.size() == 0)
    return {0.0, 0.0};

  double obj_loss = iface->calculate_loss(data);

  double regularization = iface->current_regularization_penalty();

  return {obj_loss + regularization, iface->reported_loss_value(obj_loss)};
}

/** Gets the initial objective value (objective, reportable_training_loss) for
 *  the problem.  Used to tune the sgd step size.
 *
 */
std::pair<double,double> sgd_solver_base::get_initial_objective_value(
    const v2::ml_data& data) const {

  if(data.size() == 0)
    return {0.0, 0.0};

  // Set the initial_objective_value.  This helps us determine if the
  // model has diverged.  In addition, perform a quick check of the
  // initial state; see if we need to reset the overall starting state
  // from whatever has been given us.
  bool initial_state_already_reset = false;

  double obj_val, loss;

  while(true) {

    std::tie(obj_val, loss) = calculate_objective(model_interface.get(), data, 0);

    if(!std::isfinite(obj_val)) {

      if(initial_state_already_reset) {
        log_and_throw("ERROR: Data and/or initial starting parameters cause numerical issues. "
                      "Ensure that your data is free of extreme values or NaNs (see SFrame.dropna(...) ).");
      } else {
        logstream(LOG_WARNING)
            << "WARNING: Data and/or initial starting parameters cause numerical issues; attempting restart."
            << std::endl;
      }

      model_interface->setup_optimization();
      initial_state_already_reset = true;

    } else {
      break;
    }
  }

  return {obj_val, loss};
}

/**  Calculates a reasonable stepsize for the current sample.  We
 *  return the smaller step size between 2 stepsizes.  The first is
 *  gamma / (1 + n * lm * gamma) -- the stepsize dictated in
 *
 *  Léon Bottou: Stochastic Gradient Tricks, Neural Networks, Tricks
 *  of the Trade, Reloaded, 430–445, Edited by Grégoire Montavon,
 *  Genevieve B. Orr and Klaus-Robert Müller, Lecture Notes in
 *  Computer Science (LNCS 7700), Springer, 2012
 *
 *  The second step size is gamma / (1 + n / data_size) **
 *  stepsize_decrease_rate.  This is the more standard sgd step size
 *  that works with non-regularized values.
 */
double sgd_solver_base::calculate_iteration_sgd_step_size(
    size_t iteration,
    double initial_sgd_step_size, double step_size_decrease_rate,
    double l2_regularization)
{
  // For adagrad, the step size decrease rate is ignored.
  if(step_size_decrease_rate == 0)
    return initial_sgd_step_size;

  double t = iteration * train_data.size();

  double reg_step_size_dm = 1.0 + l2_regularization * t * initial_sgd_step_size;

  double limit_step_size_dm = std::pow(1.0 + iteration, step_size_decrease_rate);

  double step = initial_sgd_step_size / std::max(reg_step_size_dm, limit_step_size_dm);

  return step;
}


/** Tests whether a model has converged or not by looking at changes
 *  in the last few iterations of the data.  Values are passed in with
 *  a map of options.
 *
 *  The technique looks at the max, min, and mean of the loss function
 *  in the last sgd_convergence_interval iterations.  If the
 *  difference of the max and the mean, divided by std::max(1, mean)
 *  is less than sgd_convergence_threshhold, then we assume the model
 *  is converged.
 *
 *  Setting sgd_convergence_interval to 0 or
 *  sgd_convergence_threshhold to 0 disables this test, forcing the
 *  algorithm to run for the full max_iterations.
 */
bool sgd_solver_base::sgd_test_model_convergence(
    const std::vector<double>& objective_value_path) {

  size_t iteration = objective_value_path.size();
  size_t convergence_interval  = options.at("sgd_convergence_interval");
  double convergence_threshold = options.at("sgd_convergence_threshold");

  // Setting sgd_convergence_interval to 0 disables this test
  if(convergence_interval == 0 || convergence_threshold == 0)
    return false;

  if(objective_value_path.back() < 1e-16)
    return true;

  if(iteration > convergence_interval) {

    double min_v = std::numeric_limits<double>::max();
    double max_v = std::numeric_limits<double>::min();
    double sum = 0;

    for(size_t i = iteration - convergence_interval; i < objective_value_path.size(); ++i) {
      double v = objective_value_path[i];

      max_v = std::max(v, max_v);
      min_v = std::min(v, min_v);
      sum += v;
    }

    double mean = sum / convergence_interval;

    if( (max_v - min_v) / std::max(1.0, mean) <= convergence_threshold) {
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

/** Adjusts the step size dynamically based on whether things are
 *  converging or not.  Returns the new step size.
 *
 *  If the loss value has gone steadily down in all of the last
 *  sgd_convergence_interval iterations, then the step size is
 *  increased such that if this happens over a full
 *  sgd_convergence_interval iterations, then the step size is
 *  doubled.  If a one-sided t-test on the differences between all the
 *  previous loss values does not show that it is decreasing with
 *  confidence > 95%, then the step size is decreased by the same
 *  amount.
 */
double sgd_solver_base::sgd_adjust_step_size(
    const std::vector<double>& sgd_path, double sgd_step_size) {

  size_t iteration             = sgd_path.size();
  size_t adjustment_interval   = options.at("sgd_step_adjustment_interval");

  // This disables this test
  if(adjustment_interval == 0)
    return sgd_step_size;

  if(iteration < adjustment_interval)
    return sgd_step_size;

  // Perform a t-test to ensure that we've actually gone down somewhat
  // in the last convergence_threshold iterations.  This is done by
  // testing whether the mean of loss differences in the last
  // convergence_threshold iterations is less than zero with 95%
  // confidence. If it is not, then we cut the step size in half over
  // the course of adjustment_interval iterations.  This will move
  // the overall algorithm towards the scenario in which the prior
  // convergence test will pick it up.

  double S1 = 0;
  double S2 = 0;
  int n = 0;
  int increase_count = 0;

  for(size_t i = iteration - adjustment_interval + 1; i < sgd_path.size(); ++i) {
    double v = sgd_path[i] - sgd_path[i-1];
    S1 += v;
    S2 += v*v;
    ++n;
    if(v > 0)
      ++increase_count;
  }

  double m = S1 / n;
  double v = (n / double(n - 1)) * (S2 / n - m*m);
  double t = (-m) / ( sqrt(v / n) );

  double stat;
  int df = n - 1;

  switch (df) {
    case 2: {stat = 2.920; break; }
    case 3: {stat = 2.353; break; }
    case 4: {stat = 2.132; break; }
    case 5: {stat = 2.015; break; }
    case 6: {stat = 1.943; break; }
    case 7: {stat = 1.895; break; }
    case 8: {stat = 1.860; break; }
    default: {stat = 1.8; break; }
  }

  if(increase_count != 0 && t < stat) {
    if(increase_count == n)
      sgd_step_size *= std::pow(0.1, 1.0 / adjustment_interval);
    else
      sgd_step_size *= std::pow(0.5, 1.0 / adjustment_interval);
  }

  return sgd_step_size;
}



}}
