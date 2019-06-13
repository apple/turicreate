/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_SGD_SOLVER_CLASS_H_
#define TURI_SGD_SGD_SOLVER_CLASS_H_

#include <map>
#include <cmath>
#include <vector>
#include <toolkits/sgd/sgd_interface.hpp>
#include <core/logging/assertions.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/variant.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

namespace turi {

class option_manager;

namespace v2 {
class ml_data;
}

namespace sgd {

/** The base solver class for all the general SGD methods.
 *
 *  This class provides the high-level functionality for the sgd
 *  methods.  Particular versions of SGD are implemented using the
 *  run_iteration method, which is called to do one pass through the
 *  data on a particular block of data points.
 */
class sgd_solver_base {

 public:

  virtual ~sgd_solver_base() = default;

 protected:

  /** The constructor.
   */
  sgd_solver_base(const std::shared_ptr<sgd_interface_base>& model_interface,
                  const v2::ml_data& _train_data,
                  const std::map<std::string, flexible_type>& _options);

  /** Call the following function to insert the option definitions
   *  needed for the common sgd optimization class into an option
   *  manager.  Meant to be called by the subclasses of
   *  sgd_solver_base.
   */
  static void add_options(option_manager& options);

 private:

  /** Can't copy this class, so delete the copy constructor.
   */
  sgd_solver_base(const sgd_solver_base&) = delete;

  /** A reference to the training data.  passed as parameter to
   *  subclass.
   */
  const v2::ml_data& train_data;

 public:

  ////////////////////////////////////////////////////////////////////////////////

  /** The main function to run the sgd solver given the current
   *  options.
   */
  std::map<std::string, variant_type> run();

 private:

  ////////////////////////////////////////////////////////////////////////////////
  // Particular methods of running things

  /** Run the sgd algorithm with a fixed step size.  If divergence is
   *  detected, then retry with a smaller step size and warn the
   *  user.
   *
   *  If the sgd_step_size parameter is set to a value greater than
   *  zero, the run() method above will run this solver.
   */
  std::map<std::string, variant_type> run_fixed_sgd_step_size(double sgd_step_size);

 protected:


  /**  The main interface to the model, implementing sgd-specific
   *   routines for that model.
   */
  std::shared_ptr<sgd_interface_base> model_interface;

  /**  The training options of the solver.
   */
  const std::map<std::string, flexible_type> options;


  ////////////////////////////////////////////////////////////////////////////////
  // Virtual methods that need to be implemented by the calling class.

  /** Called at the start of a run, before any run_iteration is
   *  called.
   */
  virtual void setup(sgd_interface_base* iface) {};


  /** Called to run one iteration of the SGD algorithm on the training
   *  data.
   *
   *  \param[in] iteration The iteration number of the current pass
   *  through the data.
   *
   *  \param[in] iface A pointer to the interface class.  This can be
   *  upcast to the true SGDInterface class for use in the actual
   *  code.
   *
   *  \param[in] step_size The step size to use for this pass through
   *  the data.
   *
   *  \return A pair -- (objective_value, loss)
   */
  virtual std::pair<double, double> run_iteration(
      size_t iteration,
      sgd_interface_base* iface,
      const v2::ml_data& data,
      double step_size) = 0;


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
  virtual std::pair<double, double> calculate_objective(
      sgd_interface_base* iface, const v2::ml_data& data, size_t iteration) const;

 private:

  ////////////////////////////////////////////////////////////////////////////////
  //
  // SGD helper functions.
  //
  // The functions below implement specific parts of the

  /** Runs the sgd algorithm with step size tuning to find the best
   *  value.  Returns the best value.
   *
   *  If the sgd_step_size parameter is set to zero, the run() method
   *  above will run this to determine the best step size.
   *
   *  The tuning is done by first running the model on a small subset
   *  of the data with several different step sizes.  The best step
   *  size is chosen as the step size for running the full algorithm.
   */
  double compute_initial_sgd_step_size();

  /** Gets the initial objective value (objective, reportable_training_loss) for
   *  the problem.  Used to tune the sgd step size.
   *
   */
  std::pair<double, double> get_initial_objective_value(const v2::ml_data& data) const;

  /**  Calculates a reasonable stepsize for the current sample.  We
   *  return the smaller step size between 2 stepsizes.  The first is
   *  gamma / (1 + n * lm * gamma) -- the stepsize dictated in
   *
   *  Léon Bottou: Stochastic Gradient Tricks, Neural Networks, Tricks
   *  of the Trade, Reloaded, 430–445, Edited by Grégoire Montavon,
   *  Genevieve B. Orr and Klaus-Robert Müller, Lecture Notes in
   *  Computer Science (LNCS 7700), Springer, 2012
   *
   *  The second step size is gamma / (1 + iteration) **
   *  stepsize_decrease_rate.  This is the more standard sgd step size
   *  that works with non-regularized values.
   */
  double calculate_iteration_sgd_step_size(
      size_t iteration,
      double initial_sgd_step_size, double step_size_decrease_rate,
      double l2_regularization);


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
  bool sgd_test_model_convergence(const std::vector<double>& sgd_path);

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
  double sgd_adjust_step_size(const std::vector<double>& sgd_path, double sgd_step_size);


};

}}

#endif /* TURI_SGD_SGD_SOLVER_CLASS_H_ */
