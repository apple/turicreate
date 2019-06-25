/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGD_INTERFACE_BASE_H_
#define TURI_SGD_INTERFACE_BASE_H_

#include <map>
#include <string>
#include <limits>

namespace turi {

class flexible_type;

namespace v2 {
class ml_data;
struct ml_data_entry;
}

namespace sgd {


/** The base class for all the SGD interfaces.  This interface governs
 *  all the interactions between the sgd solvers and the model.
 *
 *  To implement an sgd solver, subclass sgd_interface_base and
 *  implement the appropriate methods.
 *
 *  Then on top of this, choose the solver, and template it with your
 *  interface.  The basic solver is the basic_sgd_solver, defined in
 *  basic_sgd_solver.hpp.
 *
 *  Example:
 *
 *   class simple_sgd_interface {
 *      ...
 *   };
 *
 *   std::shared_ptr<simple_sgd_interface> iface(new simple_sgd_interface);
 *
 *   basic_sgd_solver<simple_sgd_interface> solver(iface, train_data, options);
 *
 *   auto training_status = solver.run();
 *
 */
class sgd_interface_base {
 public:

  virtual ~sgd_interface_base() = default;

  /**  Called at the start of optimization, before any other functions
   *   are called.
   *
   *   Perform any setup in light of the data used for training the
   *   model.  Since ml_data has some statistics (e.g. maximum row
   *   size), these can be saved for processing stuff later.
   */
  virtual void setup(const v2::ml_data& train_data,
                     const std::map<std::string, flexible_type>& options) {}

  /**  Called before each pass through the data.
   */
  virtual void setup_iteration(size_t iteration, double step_size) {}

  /**  Called at the end of each pass through the data.
   */
  virtual void finalize_iteration() {}

  /** For automatically tuning the SGD step size and calculating the
   *  decrease rate of the step size. This value is also used to
   *  determine an upper bound on the allowed sgd step size, above
   *  which the algorithm stops being numerically stable.  It also
   *  helps govern the decrease rate of the step size over iterations.
   */
  virtual double l2_regularization_factor() const { return 0; }

  /** Gives a hard limit on the sgd step size.  Certain algorithms
   *  will blow up with a step size too large, and this gives a method
   *  of setting a hard limit on step sizes considered.
   */
  virtual double max_step_size() const { return std::numeric_limits<double>::max(); }

  /** If there are any issues with the model, this function can return
   *  false to force a reset.  It is called once at the end of each
   *  iteration.
   *
   *  Returns true if the state is numerically stable, and false if
   *  there are any numerical instabilities detected now or in the
   *  previous pass over the data.  If this is true, then reset_state
   *  is called.
   */
  virtual bool state_is_numerically_stable() const { return true; }

  /**  Sets up the optimization run.  Called at the beginning of an
   *  optimization run or in the presence of numerical instabilities
   *  to reset the solver.  Optimization is attempted again with a
   *  smaller step size.
   */
  virtual void setup_optimization(size_t random_seed = size_t(-1), bool _in_trial_mode = false) = 0;

  /** Calculate the value of the objective function as determined by
   *  the loss function, for a full data set, minus the regularization
   *  penalty.
   *
   *  In reporting this loss, reported_loss_value(...) is called on
   *  this value to get a loss value to print.
   *
   *  \param data The data to use in calculating the objective function.
   *
   *  \return (loss for objective, loss for reporting)
   */
  virtual double calculate_loss(const v2::ml_data& data) const = 0;

  /**  The value of the reported loss.  The apply_sgd_step accumulates
   *   estimated loss values between samples.  This function is called
   *   with this accumulated value to get a value
   *
   *   For example, if squared error loss is used,
   *   reported_loss_name() could give RMSE, and then
   *   reported_loss_value(v) would be std::sqrt(v).
   */
  virtual double reported_loss_value(double accumulative_loss) const = 0;


  /**  The name of the loss to report on each iteration.
   *
   *   For example, if squared error loss is used,
   *   reported_loss_name() could give RMSE, and then
   *   reported_loss_value(v) would be std::sqrt(v).
   */
  virtual std::string reported_loss_name() const = 0;


  /** Calculate the current regularization penalty.  This is used to
   *  compute the objective value, which is interpreted as loss + reg
   *  penalty.
   */
  virtual double current_regularization_penalty() const = 0;

  /** Apply the sgd step.  Called on each data point.
   */
  virtual double apply_sgd_step(size_t thread_idx,
                                const std::vector<v2::ml_data_entry>& x,
                                double y,
                                double step_size) = 0;

};


}}

#endif /* TURI_SGD_INTERFACE_BASE_H_ */
