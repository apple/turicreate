/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FACTORIZATION_GLM_SGD_INTERFACE_H_
#define TURI_FACTORIZATION_GLM_SGD_INTERFACE_H_

#include <Eigen/Core>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include <atomic>

#include <util/code_optimization.hpp>
#include <util/fast_integer_power.hpp>
#include <unity/toolkits/sgd/sgd_interface.hpp>
#include <unity/toolkits/factorization/factorization_model_impl.hpp>

namespace turi { namespace factorization {

////////////////////////////////////////////////////////////////////////////////

/** The type of the regularization used.  We currently have 3
 *  different modes.  Each of these modes uses different variables,
 *  given by the parameters below.
 */
enum class model_regularization_type {L2, ON_THE_FLY, NONE};

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline T clip_1m1(T v) {
  return (v < T(-1)) ? T(-1) : ( (v > T(1)) ? T(1) : v);
}

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline T sqr(T v) { return v * v; }

/**  This class provides the interface layer for the basic sgd solver,
 *   this time for the second_order model.  It provides functions to
 *   calculate the gradient and apply a gradient update.  (In the
 *   original design, these were just folded into the model; they are
 *   now seperated out to make the original model simpler.)
 *
 *   For documentation on the interface requirements of each of the
 *   solvers, see the sgd algorithms; each one requires specific
 *   interface functions to be defined.
 */
template <typename GLMModel,
          typename _LossModelProfile,
          model_regularization_type _regularization_type>
class factorization_sgd_interface final : public sgd::sgd_interface_base {

 public:

  ////////////////////////////////////////////////////////////////////////////////
  // Only one way to instantiate this class.

  factorization_sgd_interface(const std::shared_ptr<GLMModel>& _model)
      : model(_model)
      , iteration_sample_count(0)
  {}

  factorization_sgd_interface(const factorization_sgd_interface&) = delete;
  const factorization_sgd_interface& operator=(const factorization_sgd_interface&) = delete;

  ////////////////////////////////////////////////////////////
  // Import the appropriate typedefs / constants from the model

  typedef _LossModelProfile LossModelProfile;
  LossModelProfile loss_model;

  typedef typename GLMModel::factor_type        factor_type;
  typedef typename GLMModel::factor_matrix_type factor_matrix_type;
  typedef typename GLMModel::vector_type        vector_type;

  static constexpr model_factor_mode factor_mode   = GLMModel::factor_mode;
  static constexpr flex_int num_factors_if_known         = GLMModel::num_factors_if_known;

  // Enable item locking if we're in matrix factorization mode.
  static constexpr bool enable_item_locking = true; // (factor_mode == model_factor_mode::matrix_factorization);

  /**  Trial mode is used to find the sgd step size.
   *
   */
  bool currently_in_trial_mode = false;

  /** The model_regularization_type.
   */
  static constexpr model_regularization_type regularization_type = _regularization_type;

  // If in nmf_mode, disable intercept and linear terms.
  bool nmf_mode = false;

 private:

  /** The number of factors in the model. This can sometimes be set
   *  statically, as we do for several of the default models, yielding
   *  a significant optimization benefit.
   */
  inline size_t num_factors() const GL_HOT_INLINE_FLATTEN {
    return ((factor_mode == model_factor_mode::pure_linear_model)
            ? 0
            : ( (num_factors_if_known == Eigen::Dynamic)
                ? model->num_factors()
                : num_factors_if_known));
  }

  // The model we're optimizing.  This also contains the state.
  std::shared_ptr<GLMModel> model;

  /** Returns the total dimension of all the features, i.e. the number
   * of features.
   */
  inline size_t n_total_dimensions() const GL_HOT_INLINE_FLATTEN {
    return model->n_total_dimensions;
  }

  /** Returns the dimension of the factor matrix.  Only global feature
   *  indices less than this have factors in the V factor matrix in
   *  the model.
   */
  inline size_t num_factor_dimensions() const GL_HOT_INLINE_FLATTEN {

    switch(factor_mode) {
      case model_factor_mode::pure_linear_model:
        return 0;

      case model_factor_mode::matrix_factorization:
      case model_factor_mode::factorization_machine:
        return model->num_factor_dimensions;
      default:
        return 0;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Regularization

  double _lambda_w = NAN;
  double _lambda_V = NAN;

  size_t current_iteration = size_t(-1);

  // This is for the tempering iterations on the
  double current_iteration_step_size = 0;


  size_t num_tempering_iterations = 0;
  double tempering_regularization_start_value = 0;

  double current_lambda_w(size_t iteration) const {
    return _interpolate_reg_value(iteration, _lambda_w);
  }

  double current_lambda_V(size_t iteration) const {
    return _interpolate_reg_value(iteration, _lambda_V);
  }

  double _interpolate_reg_value(size_t iteration, double lambda) const {
    if(iteration >= num_tempering_iterations)
      return lambda;

    // If we're in trial mode, only run it with the tempering for one
    // iteration; this way we can test the step size for stability
    // with the tempered step size but still test for optimization
    // without that.
    if(currently_in_trial_mode && iteration != 0)
      return lambda;

    double end_reg = std::max(1e-12, lambda);
    double begin_reg = tempering_regularization_start_value;

    if(end_reg >= begin_reg)
      return end_reg;

    // get step as an interpolation between the the tempering start
    // and the lower value.
    double s = double(iteration) / num_tempering_iterations;

    double ret = std::exp(std::log(begin_reg) * (1.0 - s) + std::log(end_reg) * s);

    // logprogress_stream
    //     << "ITER " << iteration << "; lambda = " << lambda
    //     << "; s = " << s << "; actual = " << ret << std::endl;

    return ret;
  }

  size_t data_size;

  // Variables needed for L2 regularization. This term tracks the regularization of the
  // terms in the state that are only touched by the L2 regularization
  // term at each iteration.  The original s_w are now equal to
  // s_w_factor^iteration_sample_count.

  double s_w_factor = NAN, s_V_factor = NAN;
  fast_integer_power s_w_factor_pow, s_V_factor_pow;
  bool s_w_identically_1 = true, s_V_identically_1 = true;
  std::atomic<size_t> iteration_sample_count;
  

  // Variables needed for the on-the-fly (aka weighted) regularization.  This vector
  // is of length n_total_dimensions, and is equal to the number of
  // parameters that each regularization value hits.
  double w_shrinkage = NAN, V_shrinkage = NAN;
  vector_type on_the_fly__regularization_scaling_factors;

  // Used if the items are locked.
  std::vector<simple_spinlock> item_locks;

  size_t parameter_scaling_offset;
  vector_type parameter_scaling;

  ////////////////////////////////////////////////////////////
  //
  // A container to hold the state updates that get applied during the
  // (locked) gradient update step.  Each thread has one of these
  // buffers to avoid allocations during the update step.
  //
  size_t n_threads = 1;

  struct sgd_processing_buffer {
    double w0;

    struct variable {

      size_t index;
      double xv;
      double w;

      factor_type V_row;
      factor_type xV_row;

      // This stores the row pointer to the actual, original row.
      float * __restrict__ V_row_ptr;
    };

    std::vector<variable> v;

    factor_type xv_accumulator;
  };

  mutable std::vector<sgd_processing_buffer> buffers, alt_buffers;


  // For adagrad:  If true, use the adagrad model.
  bool adagrad_mode = true;
  // vector_type w_adagrad_g;
  // factor_matrix_type V_adagrad_g;
  volatile double w0_adagrad_g;

  float adagrad_momentum_weighting = 1.0;

  Eigen::Matrix<float, Eigen::Dynamic, num_factors_if_known, Eigen::RowMajor> V_adagrad_g;
  Eigen::Matrix<float, Eigen::Dynamic, 1> w_adagrad_g;

  ////////////////////////////////////////////////////////////////////////////////
  // Set up and tear down methods for the different things.

 public:
  /**  Set up all the stuff needed for processing the data at each
   *   iteration.
   */
  void setup(const v2::ml_data& train_data,
             const std::map<std::string, flexible_type>& options) {

    ////////////////////////////////////////////////////////////
    // Set up some common constants used everywhere.

    n_threads = thread::cpu_count();

    data_size = train_data.size();

    _lambda_w = options.at("linear_regularization");
    _lambda_V = options.at("regularization");
    num_tempering_iterations = options.at("num_tempering_iterations");
    num_tempering_iterations = std::min(num_tempering_iterations, size_t(options.at("max_iterations")));
    tempering_regularization_start_value = options.at("tempering_regularization_start_value");

    nmf_mode = options.at("nmf");

    adagrad_mode = (options.at("solver") == "adagrad");

    if(adagrad_mode)
        adagrad_momentum_weighting = options.at("adagrad_momentum_weighting");

    ////////////////////////////////////////////////////////////
    // Set up all the buffers

    buffers.resize(n_threads);
    alt_buffers.resize(n_threads);

    size_t max_row_size = train_data.max_row_size();

    // Init the buffer arrays.
    for(std::vector<sgd_processing_buffer>* bv : {&buffers, &alt_buffers}) {
      for(sgd_processing_buffer& buffer : (*bv)) {

        buffer.v.resize(max_row_size);

        for(auto& var : buffer.v) {
          var.V_row.resize(num_factors());
          var.xV_row.resize(num_factors());
        }

        buffer.xv_accumulator.resize(num_factors());
      }
    }

    ////////////////////////////////////////////////////////////
    // Set all the iteration-based constants to appropriate values for
    // computing things when no iterations are happening

    s_w_factor = 1.0;
    s_w_factor_pow.set_base(1.0);
    s_w_identically_1 = true;

    s_V_factor = 1.0;
    s_V_factor_pow.set_base(1.0);
    s_V_identically_1 = true;

    iteration_sample_count = 0;

    ////////////////////////////////////////////////////////////
    // Set up things needed for the different regularization interfaces.

    switch(regularization_type) {
      case model_regularization_type::L2:
        break;

      case model_regularization_type::ON_THE_FLY:
        {
          on_the_fly__regularization_scaling_factors.resize(n_total_dimensions());

          size_t pos = 0;
          for(size_t c_idx = 0; c_idx < train_data.num_columns(); ++c_idx) {
            for(size_t i = 0; i < train_data.metadata()->index_size(c_idx); ++i, ++pos) {
              on_the_fly__regularization_scaling_factors[pos]
                  = (train_data.metadata()->statistics(c_idx)->count(i)
                     / (std::max(size_t(1), train_data.size())));
            }
          }

          break;
        }

      case model_regularization_type::NONE:
        break;
    }

    // Set up the locking buffers if needed.
    static constexpr size_t ITEM_COLUMN_INDEX = 1;

    if(enable_item_locking)
      item_locks.resize(train_data.metadata()->index_size(ITEM_COLUMN_INDEX));

    // Set up the stuff for adagrad
    if(adagrad_mode) {
      w_adagrad_g.resize(model->w.size());
      V_adagrad_g.resize(model->V.rows(), num_factors());
    }
  }

 public:

  ////////////////////////////////////////////////////////////////////////////////
  // Functions to help with the SGD stuff.

  /**  Returns the l2 regularization coefficient.  
   *
   */
  double l2_regularization_factor() const {
    return (regularization_type == model_regularization_type::L2
            ? std::max(_lambda_w, _lambda_V)
            : 0); 
  }

  /**  Returns an upper bound on the sgd step size.  
   *
   */
  double max_step_size() const {
    switch(regularization_type) {
      case model_regularization_type::L2:
      case model_regularization_type::ON_THE_FLY:

        // This ensures that (1 - step_size * lambda) > 0, a very
        // important requirement for numerical stability. 
        
        return 0.9 / (1e-16 + std::max(current_lambda_w(0), current_lambda_V(0)));

      case model_regularization_type::NONE:
      default:
        return std::numeric_limits<double>::max();
    }
  }

  /**  Set up the class and constants before every iteration.
   */
  void setup_iteration(size_t iteration, double step_size) {

    // Set the global current iteration value.
    current_iteration = iteration;
    current_iteration_step_size = step_size;

    // The following text is saved for debugging mode stuff.

    // if(adagrad_mode && factor_mode == model_factor_mode::factorization_machine) {
    //   size_t idx = model->index_offsets[2];
    //   logprogress_stream << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ " << std::endl;
    //   logprogress_stream << "w = " << model->w[idx]
    //                      << "; w_adagrad = " << w_adagrad_g[idx] << std::endl;
    //   logprogress_stream << "V         = " << model->V.row(idx) << std::endl;
    //   logprogress_stream << "V_adagrad = " << V_adagrad_g.row(idx) << std::endl;

    //   logprogress_stream << "w0 = " << model->w0  << "; w0_adagrad = " << w0_adagrad_g << std::endl;

    //   logprogress_stream << "w max = " << model->w.maxCoeff()
    //                      << ";\t w min = " << model->w.minCoeff()
    //                      << ";\t w_ada min = " << w_adagrad_g.minCoeff()
    //                      << ";\t w_ada min = " << w_adagrad_g.maxCoeff() << std::endl;

    //   logprogress_stream << "V max = " << model->V.maxCoeff()
    //                      << ";\t V min = " << model->V.minCoeff()
    //                      << ";\t V_ada min = " << V_adagrad_g.minCoeff()
    //                      << ";\t V_ada max = " << V_adagrad_g.maxCoeff() << std::endl;
    // }

    switch(regularization_type) {
      case model_regularization_type::L2: {

        // Set the sample count to 0.
        iteration_sample_count = 0;

        double lambda_w = current_lambda_w(iteration);
        double lambda_V = current_lambda_V(iteration);

        // The s scaling factor in the l2 regularization is just the
        // power of (1 - step_size * lambda) raised to the power n,
        // where n is the number of samples seen so far on this
        // iteration.

        w_shrinkage = 1.0;
        V_shrinkage = 1.0;

        double w_step_size = step_size;
        double V_step_size = step_size;

        if(adagrad_mode) {
          w_step_size /= std::max(1.0, std::sqrt(double(w_adagrad_g.mean())));
          if(V_adagrad_g.rows() != 0)
            V_step_size /= std::max(1.0, std::sqrt(double(V_adagrad_g.mean())));
        }
        
        s_w_factor = 1.0 - w_step_size * lambda_w;
        s_w_factor_pow.set_base(s_w_factor);
        s_w_identically_1 = (s_w_factor == 1.0);

        s_V_factor = 1.0 - V_step_size * lambda_V;
        s_V_factor_pow.set_base(s_V_factor);
        s_V_identically_1 = (s_V_factor == 1.0);
        break;
      }
      case model_regularization_type::ON_THE_FLY:

        w_shrinkage = 1.0 - step_size * _lambda_w;
        V_shrinkage = 1.0 - step_size * _lambda_V;

        // These factors are identically 1.
        s_w_factor = 1.0;
        s_w_factor_pow.set_base(s_w_factor);
        s_w_identically_1 = true;

        s_V_factor = 1.0;
        s_V_factor_pow.set_base(s_V_factor);
        s_V_identically_1 = true;
        break;
        
      case model_regularization_type::NONE:

        w_shrinkage = 1.0;
        V_shrinkage = 1.0;

        s_w_factor = 1.0;
        s_w_factor_pow.set_base(s_w_factor);
        s_w_identically_1 = true;

        s_V_factor = 1.0;
        s_V_factor_pow.set_base(s_V_factor);
        s_V_identically_1 = true;

        break;
    }

    // Now, for the iteration, set this.  It means that underflows are
    // flushed to zero.
    set_denormal_are_zero();

    current_iteration = iteration;
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Finalizes the iteration.  Called after each pass through the data.
   *
   */
  void finalize_iteration() {

    if(adagrad_mode && adagrad_momentum_weighting != 1.0) {
      float rho = adagrad_momentum_weighting;

      if(w_adagrad_g.size() != 0)
        w_adagrad_g.array() = rho * w_adagrad_g.array() + (1 - rho) * w_adagrad_g.mean();
      
      if(V_adagrad_g.rows() != 0)
        V_adagrad_g.array() = rho * V_adagrad_g.array() + (1 - rho) * V_adagrad_g.mean();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Sync all the current stuff to the model.

    switch(regularization_type) {

      case model_regularization_type::L2:
        {
          double n_samples_processed = iteration_sample_count;
          double s_w = s_w_factor_pow.pow(n_samples_processed);
          double s_V = s_V_factor_pow.pow(n_samples_processed);

          in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

              if(!nmf_mode && s_w != 1.0) {

                size_t start_w_idx = (thread_idx * n_total_dimensions()) / num_threads;
                size_t end_w_idx = ((thread_idx + 1) * n_total_dimensions()) / num_threads;

                for(size_t i = start_w_idx; i < end_w_idx; ++i) {

                  model->w[i] *= s_w;

                  if(std::fabs(model->w[i]) < 1e-16)
                    model->w[i] = 0;
                }
              }

              if(num_factor_dimensions() != 0 && s_V != 1.0) {

                size_t start_V_idx = (thread_idx * num_factor_dimensions() ) / num_threads;
                size_t end_V_idx = ((thread_idx + 1) * num_factor_dimensions() ) / num_threads;

                for(size_t i = start_V_idx; i < end_V_idx; ++i) {
                  for(size_t j = 0; j < num_factors(); ++j) {

                    model->V(i,j) *= s_V;

                    if(std::fabs(model->V(i,j)) < 1e-16)
                      model->V(i,j) = 0;
                  }
                }
              }
            });

        }

      case model_regularization_type::ON_THE_FLY:
      case model_regularization_type::NONE:

        // Don't need to do anything for this regularization mode.
        break;
    }

    unset_denormal_are_zero(); 
  }

  /// Test whether the current state is numerically stable or not; if
  /// not, it needs to be reset.
  bool state_is_numerically_stable() const GL_HOT_INLINE_FLATTEN {
    if(!(std::isfinite(model->w0) && std::fabs(model->w0) <= 1e12))
      return false;

    // This tests for the corner case of the model hitting the point
    // where all the factors are identically 0 and then getting stuck.
    if(nmf_mode) {
      // Test for hitting the zero point.
      for(size_t i = 0; i < num_factor_dimensions(); ++i) {
        if(model->V.row(i).sum() > 1e-16)
          return true;
      }

      return false;
    } else {
      return true;
    }
  }

  /**  Sets up the optimization run.  Called at the beginning of an
   *  optimization run or to reset the state.
   */
  void setup_optimization(size_t random_seed = size_t(-1), bool trial_mode = false) {
    if(random_seed == size_t(-1))
      random_seed = model->options.at("random_seed");

    model->reset_state(random_seed, 0.001);
    currently_in_trial_mode = trial_mode;

    if(adagrad_mode) {
      w_adagrad_g.setConstant(1e-16);
      V_adagrad_g.setConstant(1e-16);
      w0_adagrad_g = 1e-16;
    }

  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Calculate the current regularization penalty.
   *
   */
  double current_regularization_penalty() const {

    double lambda_w = current_lambda_w(current_iteration);
    double lambda_V = current_lambda_V(current_iteration);

    if(regularization_type == model_regularization_type::NONE
       || (lambda_w == 0 && lambda_V == 0) ) {
      return 0;
    }

    size_t n_threads = thread::cpu_count();

    std::vector<double> accumulative_regularization_penalty(n_threads, 0);

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

        size_t w_start_idx = (thread_idx * size_t(model->w.size())) / num_threads;
        size_t w_end_idx   = ((thread_idx + 1) * size_t(model->w.size())) / num_threads;

        size_t V_start_idx = (thread_idx * size_t(model->V.rows())) / num_threads;
        size_t V_end_idx   = ((thread_idx + 1) * size_t(model->V.rows())) / num_threads;

        if(regularization_type == model_regularization_type::ON_THE_FLY) {

          if(lambda_w != 0) {
            for(size_t i = w_start_idx; i < w_end_idx; ++i) {

              accumulative_regularization_penalty[thread_idx]
                  += (lambda_w
                      * on_the_fly__regularization_scaling_factors[i]
                      * (model->w[i] * model->w[i]));
            }
          }

          if(lambda_V != 0) {
            for(size_t i = V_start_idx; i < V_end_idx; ++i) {

              accumulative_regularization_penalty[thread_idx]
                  += (lambda_V
                      * on_the_fly__regularization_scaling_factors[i]
                      * model->V.row(i).squaredNorm());
            }
          }

        } else {

          if(lambda_w != 0) {
            accumulative_regularization_penalty[thread_idx] +=
                lambda_w * model->w.segment(w_start_idx, w_end_idx - w_start_idx).squaredNorm();
          }

          if(lambda_V != 0) {
            accumulative_regularization_penalty[thread_idx] +=
                lambda_V * model->V.block(V_start_idx, 0, V_end_idx - V_start_idx, num_factors()).squaredNorm();
          }
        }

      });

    double total_reg = std::accumulate(accumulative_regularization_penalty.begin(),
                                       accumulative_regularization_penalty.end(),
                                       double(0.0));

    return total_reg;
  }


  /** Calculate the value of the objective function as determined by the
   *  loss function, for a full data set, minus the regularization
   *  penalty.
   */
  double calculate_loss(const v2::ml_data& data) const {

    std::vector<double> total_loss_accumulator(thread::cpu_count(), 0);

    volatile bool numerical_error_detected = false;

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {

        std::vector<v2::ml_data_entry> x;

        for(auto it = data.get_iterator(thread_idx, num_threads);
            !it.done() && !numerical_error_detected;
            ++it) {

          it.fill_observation(x);

          double y = it.target_value();

          double fx_pred = calculate_fx(thread_idx, x);
          double point_loss = loss_model.loss(fx_pred, y);

          if(!std::isfinite(point_loss)) {
            numerical_error_detected = true;
            break;
          }

          total_loss_accumulator[thread_idx] += point_loss;
        }
      }
      );

    if(numerical_error_detected)
      return NAN;

    double total_loss = std::accumulate(total_loss_accumulator.begin(),
                                        total_loss_accumulator.end(), double(0));

    size_t n = data.size();
    double loss_value = (n != 0) ? (total_loss / n) : 0;

    return loss_value;
  }

  /**  The value of the reported loss.  The apply_sgd_step accumulates
   *   estimated loss values between samples.  This function is called
   *   with this accumulated value to get a value
   *
   *   For example, if squared error loss is used,
   *   reported_loss_name() could give RMSE, and then
   *   reported_loss_value(v) would be std::sqrt(v).
   */
  double reported_loss_value(double accumulative_loss) const {
    return loss_model.reported_loss_value(accumulative_loss);
  }

  /**  The name of the loss to report on each iteration.
   *
   *   For example, if squared error loss is used,
   *   reported_loss_name() could give RMSE, and then
   *   reported_loss_value(v) would be std::sqrt(v).
   */
  std::string reported_loss_name() const {
    return loss_model.reported_loss_name();
  }

  ////////////////////////////////////////////////////////////////////////////////

 private:

  /** Fill up the buffer with the current state, performing
   *  appropriate scaling along the way.  Return the current function
   *  value.
   *
   *  This version is called when the factor type is full -- i.e. the
   *  factorization machine.
   */
  inline double _fill_buffer_calc_value(
      sgd_processing_buffer& buffer,
      const std::vector<v2::ml_data_entry>& x,
      double l2_s_w_old, double l2_s_V_old) const GL_HOT {

    const bool using_l2_regularization
        = (regularization_type == model_regularization_type::L2);

    const double s_w = using_l2_regularization ? l2_s_w_old : 1;
    const double s_V = using_l2_regularization ? l2_s_V_old : 1;

    // Set the row size.
    const size_t x_size = x.size();

    ////////////////////////////////////////////////////////////////////////////////
    // Each of the three models has a different method for computing
    // the factor mode.

    switch(factor_mode) {

      ////////////////////////////////////////////////////////////////////////////////
      // FACTORIZATION MODEL -- ALL FACTORS COMPUTED.

      case model_factor_mode::factorization_machine: {

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 1: Pull in a snapshot of the current model values for the
        // linear part of the model, and calculate the fx_value from the
        // linear part as well.

        double fx_value = 0;

        buffer.xv_accumulator.setZero();

        for(size_t j = 0; j < x_size; ++j) {
          const v2::ml_data_entry& v = x[j];
          auto& b = buffer.v[j];

          // Get the global index
          const size_t global_idx = model->index_offsets[v.column_index] + v.index;
          b.index = global_idx;

          b.w = model->w[global_idx];

          // Set the x scaling on this column
          double value_shift, value_scale;
          std::tie(value_shift, value_scale) = model->column_shift_scales[global_idx];

          b.xv = value_scale * (v.value - value_shift);

          // Set all the rows to be correct
          b.V_row = model->V.row(global_idx);
          b.xV_row = (s_V * b.xv) * b.V_row;
          buffer.xv_accumulator += b.xV_row;

          fx_value += (s_w * b.xv) * b.w;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 2: Calculate the inner product between factors by first
        // calculating the total sum -- this is stored in xv_accumulator
        // -- then do another pass through the data to calculate the
        // summation over inner product.  The interactions are the square
        // of the accumulator minus the sum of the inner products.

        {
          double fx_delta = 0;
          for(size_t j = 0; j < x.size(); ++j) {
            const auto& b = buffer.v[j];
            fx_delta += buffer.xv_accumulator.dot(b.xV_row) - b.xV_row.squaredNorm();
          }

          fx_value += 0.5*fx_delta;
        }


        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 3: Add in the intercept term.  As this term has the
        // most thread contention, make sure it's read as closely to
        // the gradient update as possible.

        buffer.w0 = model->w0;
        fx_value += buffer.w0;

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 4: We're done, return!

        return fx_value;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // MATRIX FACTORIZATION -- FACTORS ON ONLY FIRST TWO TERMS

      case model_factor_mode::matrix_factorization: {

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Step 1: Pull in a snapshot of the current model values for the
        // linear part of the model, and calculate the fx_value from the
        // linear part as well.

        DASSERT_GE(buffer.v.size(), x.size());

        double fx_value = 0;

        for(size_t j : {1, 0}) {
          const v2::ml_data_entry& v = x[j];
          auto& b = buffer.v[j];

          DASSERT_EQ(j, v.column_index);

          // Get the global index
          const size_t global_idx = (j == 0 ? 0 : model->index_offsets[1]) + v.index;
          b.index = global_idx;
          b.V_row = model->V.row(global_idx);
          
          // No column scaling on the first two dimensions under MF model.
          DASSERT_EQ(v.value, 1);
          b.xv = 1.0;

          // No need for volatile on the item terms, as they are locked.
          fx_value += s_w * model->w[global_idx];
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Step 2: Calculate the dimensions past the first two.  These
        // can have anything in them.

        if(x_size > 2) {
          for(size_t j = 2; j < x_size; ++j) {
            const v2::ml_data_entry& v = x[j];
            auto& b = buffer.v[j];

            // Get the global index
            const size_t global_idx = model->index_offsets[v.column_index] + v.index;
            b.index = global_idx;

            // Set the scaling on this column
            double value_shift, value_scale;
            std::tie(value_shift, value_scale) = model->column_shift_scales[global_idx];

            b.xv = value_scale * (v.value - value_shift);

            fx_value += (s_w * b.xv) * model->w[global_idx];
          }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  Step 3: Pull in the contribution from the factors.
        fx_value += (s_V * s_V) * (buffer.v[0].V_row.dot(buffer.v[1].V_row));

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Finally, add in the intercept term.  As this term has the
        // most thread contention, make sure it's read as closely to
        // the gradient update as possible.

        buffer.w0 = model->w0;
        fx_value += buffer.w0;

        return fx_value;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // LINEAR REGRESSION -- NO FACTORS AT ALL.

      case model_factor_mode::pure_linear_model: {

        DASSERT_GE(buffer.v.size(), x.size());

        double fx_value = 0;

        for(size_t j = 0; j < x_size; ++j) {
          const v2::ml_data_entry& v = x[j];
          auto& b = buffer.v[j];

          // Get the global index
          const size_t global_idx = model->index_offsets[v.column_index] + v.index;
          b.index = global_idx;

          b.w = model->w[global_idx];

          // Set the scaling on this column
          double value_shift, value_scale;
          std::tie(value_shift, value_scale) = model->column_shift_scales[global_idx];

          b.xv = value_scale * (v.value - value_shift);

          fx_value += (s_w * b.xv) * b.w;
        }

        buffer.w0 = model->w0;
        fx_value += buffer.w0;

        return fx_value;
      }

      default:
        return 0;
    }
  }


  /** The information from the regularization updates.
   */
  struct _regularization_updates {
    double s_w_old, s_w_new_inv;
    double s_V_old, s_V_new_inv;
  };

  /** Apply the updates to the regularization scaling parameters.
   *
   */
  inline _regularization_updates _apply_regularization_update(
      double step_size, bool apply_regularization = true)
      GL_HOT_INLINE_FLATTEN {

    switch(regularization_type) {

      case model_regularization_type::L2: {

        _regularization_updates ru;

        size_t n = (apply_regularization
                    ? size_t(iteration_sample_count.fetch_add(1, std::memory_order_relaxed))
                    : size_t(iteration_sample_count));

        if(s_w_identically_1) {
          ru.s_w_old = 1;
          ru.s_w_new_inv = 1;
        } else {
          ru.s_w_old = s_w_factor_pow.pow(n);
          ru.s_w_new_inv = 1.0 / (ru.s_w_old * s_w_factor);
        }

        if(s_V_identically_1) {
          ru.s_V_old = 1;
          ru.s_V_new_inv = 1;
        } else {
          ru.s_V_old = s_V_factor_pow.pow(n);
          ru.s_V_new_inv = 1.0 / (ru.s_V_old * s_V_factor);
        }

        return ru;
      }
      case model_regularization_type::ON_THE_FLY:
      case model_regularization_type::NONE:
      default:
        return {1.0, 1.0, 1.0, 1.0};

    }
  }

  /** Apply w0 update.
   */
  inline void _apply_w0_gradient(
      sgd_processing_buffer& buffer,
      double l_grad,
      double step_size) GL_HOT_INLINE_FLATTEN {

    // In the case of squared error, for numerical stability, limit
    // the update to not go past the bottom of the quadratic curve.
    // Otherwise, things seem to go out of controll waaaay too often.

    // Only update this term if we aren't in squared error loss.
    if(std::is_same<LossModelProfile, loss_squared_error>::value)
      return;

    double delta = l_grad;

    if(adagrad_mode) {
      double _wg = (w0_adagrad_g += (delta * delta));
      delta /= std::sqrt(_wg);
    }

    model->w0 -= step_size * delta / n_threads;;
  }

  atomic<size_t> hits;

  /** Apply updates to the linear and quadratic terms.
   */
  inline void _apply_w_V_gradient(
      sgd_processing_buffer& buffer, double l_grad,
      double s_w_new_inv, double s_V_new_inv,
      size_t x_size, double step_size) GL_HOT_INLINE_FLATTEN {


    static constexpr bool using_on_the_fly_regularization =
        (regularization_type == model_regularization_type::ON_THE_FLY);

    typedef volatile float * __restrict__ vfloat_ptr;

    float ss_scaling_factor = float(adagrad_mode ? sq(step_size / current_iteration_step_size) : 1.0);

    // There are several modes here...
    switch(factor_mode) {

      ////////////////////////////////////////////////////////////////////////////////
      // FACTORIZATION MODEL -- ALL FACTORS COMPUTED.

      case model_factor_mode::factorization_machine: {

        // Prefetch all rows except the user terms.
        for(size_t j = 1; j < x_size; ++j)
          __builtin_prefetch(&(model->V(buffer.v[j].index)), 1, 1);

        // Now apply the gradient to w and to the factors in turn
        for(size_t j = 0; j < x_size; ++j) {

          auto& b = buffer.v[j];

          if(b.xv == 0) continue;

          // Step is the amount to move minus the regularization scaling and step size

          // The l_grad is needed for the adagrad computation; the scale
          // is how much actually gets applied.  The latter depends on
          // step size and regularization scale.

          double w_grad = l_grad * b.xv;
          double step_w_scale = step_size;
          double step_V_scale = step_size;

          // Apply the linear terms
          if(!nmf_mode) {

            if(adagrad_mode) {
              w_adagrad_g[b.index] += ss_scaling_factor * w_grad * w_grad;
              step_w_scale /= std::sqrt(w_adagrad_g[b.index]);
            }

            // Set the new linear terms
            model->w[b.index] -= clip_1m1(w_grad * step_w_scale) * s_w_new_inv;;

            if(using_on_the_fly_regularization) {
              model->w[b.index] *= w_shrinkage;
            }
          }

          // Use xV_row as a buffer to hold the gradient step

          // Unclipped =

          // b.xV_row -= buffer.xv_accumulator;

          b.xV_row = (l_grad * (buffer.xv_accumulator - b.xV_row));
          if(adagrad_mode) {

            for(size_t i = 0; i < num_factors(); ++i) {
              V_adagrad_g(b.index, i) += ss_scaling_factor * b.xV_row[i] * b.xV_row[i];
              b.xV_row[i] /= std::sqrt(V_adagrad_g(b.index, i));
            }
          }

          if(nmf_mode) {

            for(size_t i = 0; i < num_factors(); ++i)
              b.V_row[i] -= clip_1m1(step_V_scale * b.xV_row[i]) * s_V_new_inv;

            if(using_on_the_fly_regularization)
              b.V_row *= V_shrinkage;

            for(size_t i = 0; i < num_factors(); ++i) {
              if(b.V_row[i] < 0)
                b.V_row[i] = 0;
            }

            model->V.row(b.index) = b.V_row;

          } else {
            if(using_on_the_fly_regularization) {
              for(size_t i = 0; i < num_factors(); ++i)
                model->V(b.index, i)
                    = V_shrinkage * (b.V_row[i] - clip_1m1(step_V_scale * b.xV_row[i]) * s_V_new_inv);

            } else {
              b.xV_row *= step_V_scale;
              b.xV_row = b.xV_row.cwiseMin(float(1.0));
              b.xV_row = b.xV_row.cwiseMax(float(-1.0));

              model->V.row(b.index) -= s_V_new_inv * b.xV_row;
            }
          }
        }
        break;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // MATRIX FACTORIZATION -- FACTORS ON ONLY FIRST TWO TERMS

      case model_factor_mode::matrix_factorization: {

        // In the case of squared error, for numerical stability, limit
        // the update to not go past the bottom of the quadratic curve.
        // Otherwise, things seem to go out of controll waaaay too often.

        if(!nmf_mode) {
          for(size_t j = 0; j < x_size; ++j) {
            const auto& b = buffer.v[j];

            double w_delta = l_grad  * b.xv;

            if(adagrad_mode) {
              w_adagrad_g[b.index] += ss_scaling_factor * w_delta * w_delta;
              w_delta /= std::sqrt(w_adagrad_g[b.index]);
            }

            vfloat_ptr w_ptr = (vfloat_ptr)(&(model->w[b.index]));

            // Clip for stability.

            if(using_on_the_fly_regularization)
              *w_ptr = w_shrinkage * ((*w_ptr) - (clip_1m1(step_size * w_delta) * s_w_new_inv));
            else
              *w_ptr -= clip_1m1(step_size * w_delta) * s_w_new_inv;
          }
        }

        auto GL_GCC_ONLY(__restrict__)& b0 = buffer.v[0];
        auto GL_GCC_ONLY(__restrict__)& b1 = buffer.v[1];

        b0.xV_row = l_grad * b1.V_row;
        b1.xV_row = l_grad * b0.V_row;

        // Clip for stability; apply to the temporary buffer.

        if(adagrad_mode) {
          for(size_t i = 0; i < num_factors(); ++i) {
            V_adagrad_g(b0.index, i) += ss_scaling_factor * b0.xV_row[i] * b0.xV_row[i];
            b0.xV_row[i] /= std::sqrt(V_adagrad_g(b0.index, i));
          }

          for(size_t i = 0; i < num_factors(); ++i) {
            V_adagrad_g(b1.index, i) += ss_scaling_factor * b1.xV_row[i] * b1.xV_row[i];
            b1.xV_row[i] /= std::sqrt(V_adagrad_g(b1.index, i));
          }
        }

        b0.xV_row *= step_size;
        b0.xV_row = b0.xV_row.cwiseMin(float(1.0));
        b0.xV_row = b0.xV_row.cwiseMax(float(-1.0));

        b1.xV_row *= step_size;
        b1.xV_row = b1.xV_row.cwiseMin(float(1.0));
        b1.xV_row = b1.xV_row.cwiseMax(float(-1.0));

        // Apply the gradient to the temporary buffer
        b0.V_row -= s_V_new_inv * b0.xV_row;
        b1.V_row -= s_V_new_inv * b1.xV_row;

        if(using_on_the_fly_regularization) {
          b0.V_row *= V_shrinkage;
          b1.V_row *= V_shrinkage;
        }

        if(nmf_mode) {
          // Clip to non-negative values.
          model->V.row(b0.index) = b0.V_row.cwiseMax(float(0));
          model->V.row(b1.index) = b1.V_row.cwiseMax(float(0));
        } else {
          model->V.row(b0.index) = b0.V_row;
          model->V.row(b1.index) = b1.V_row;
        }

        break;
      }

        ////////////////////////////////////////////////////////////////////////////////
        // LINEAR REGRESSION -- NO FACTORS AT ALL.

      case model_factor_mode::pure_linear_model: {

        for(size_t j = 0; j < x_size; ++j) {
          const auto& b = buffer.v[j];

          double w_delta = l_grad  * b.xv;

          if(adagrad_mode) {
            w_adagrad_g[b.index] += ss_scaling_factor * w_delta * w_delta;
            w_delta /= std::sqrt(w_adagrad_g[b.index]);
          }

          vfloat_ptr  w_ptr = (vfloat_ptr)(&(model->w[b.index]));

          // Clip for stability.

          if(using_on_the_fly_regularization)
            *w_ptr = w_shrinkage * ((*w_ptr) - (clip_1m1(step_size * w_delta) * s_w_new_inv));
          else
            *w_ptr -= clip_1m1(step_size * w_delta) * s_w_new_inv;
        }
        break;
      }
    }
  }

 public:

  ////////////////////////////////////////////////////////////////////////////////

  /** Calculate the gradient with respect to a single observation,
   * then applies it.  Used by the basic sgd solver; this one is used
   * for the second_order model.
   *
   * x is the observation vector, in standard ml_data_entry format,
   * formed by ml_data_iterator.fill_observation(...).
   *
   * struct ml_data_entry {
   *  size_t column_index;      // Column id
   *  size_t index;             // Local index within the column.
   *  double value;             // Value
   * };
   */
  double calculate_fx(size_t thread_idx,
                      const std::vector<v2::ml_data_entry>& x) const GL_HOT_FLATTEN {

    DASSERT_LT(thread_idx, buffers.size());

    sgd_processing_buffer& buffer = buffers[thread_idx];

    switch(regularization_type) {
      case model_regularization_type::L2: {
        size_t n = iteration_sample_count;

        double s_w, s_V; 
        
        if(n == 0) {
          s_w = s_V = 1;
        } else {
          s_w = s_w_factor_pow.pow(n);
          s_V = s_V_factor_pow.pow(n);
        }

        return _fill_buffer_calc_value(buffer, x, s_w, s_V);
      }
      case model_regularization_type::ON_THE_FLY:
      case model_regularization_type::NONE: {
        return _fill_buffer_calc_value(buffer, x, 1.0, 1.0);
      }
      default:
        return 0;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////

 /** Calculate the gradient with respect to a single observation,
   * then applies it.  Used by the basic sgd solver; this one is used
   * for the second_order model.
   *
   * x is the observation vector, in standard ml_data_entry format,
   * formed by ml_data_iterator.fill_observation(...).
   *
   * struct ml_data_entry {
   *  size_t column_index;      // Column id
   *  size_t index;             // Local index within the column.
   *  double value;             // Value
   * };
   */
  GL_HOT_INLINE_FLATTEN
      inline double apply_sgd_step(size_t thread_idx,
                                   const std::vector<v2::ml_data_entry>& x,
                                   double y,
                                   double step_size,
                                   bool apply_regularization) {

    sgd_processing_buffer& buffer = buffers[thread_idx];

    static constexpr size_t ITEM_COLUMN_INDEX = 1;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Step 1: Update the scaling of the regularization tracking
    // constants.  We need to do this atomically; otherwise, the
    // updates may not be consistent.

    auto ru = _apply_regularization_update(step_size, apply_regularization);

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Step 2: Pull in a snapshot of the current model values for the
    // linear part of the model, and calculate the fx_value from the
    // linear part as well.

    std::unique_lock<simple_spinlock> item_lock(item_locks[x[ITEM_COLUMN_INDEX].index], std::defer_lock);

    if(enable_item_locking)
      item_lock.lock();

    double fx_value = _fill_buffer_calc_value(buffer, x, ru.s_w_old, ru.s_V_old);


    // The following commented out code is kept in here to help with debugging.

    // if(!(std::fabs(fx_value) < 1000)) {

    //   logprogress_stream     << "Thread " << thread_idx
    //                          << "; user = " << x[0].index
    //                          << "; item = " << x[1].index
    //                          << "; w0 = " << model->w0
    //                          << "; buffered w0 = " << buffer.w0
    //                          << "; fx_value = " << fx_value
    //                          << "; s.s_w_new_inv = " << s.s_w_new_inv
    //                          << "; s.s_V_new_inv = " << s.s_V_new_inv
    //                          << "; step_size = " << step_size
    //                          << "; y = " << y
    //                          << "; w[user] = " << model->w[buffer.v[0].index]
    //                          << "; w[item] = " << model->w[buffer.v[1].index]
    //                          << "; item_global = " << buffer.v[1].index
    //                          << "; item_count = " << model->metadata->statistics(1)->count(x[1].index)
    //                          << std::endl;

    //   if(num_factors() != 0) {
    //     logprogress_stream << "; V[user] = " << model->V.row(buffer.v[0].index)
    //                        << "; V[item] = " << model->V.row(buffer.v[1].index) << std::endl;
    //   }


    //   for(size_t i = 0; i < size_t(model->w.size()); ++i) {
    //     float v = model->w[i];
    //     if(! (std::fabs(v) < 1000) ) {
    //       logstream(LOG_WARNING) << "Thread " << thread_idx
    //                              << "; w[" << i << "] = " << v
    //                              << std::endl;
    //     }
    //   }
    // }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Step 3: Everything needed for the prediction fx should now be
    // calculated; go ahead and use that to get the gradient of the
    // loss function at fx_value.

    double l_grad = loss_model.loss_grad(fx_value, y);

    ////////////////////////////////////////////////////////////
    //
    //  Step 4: Apply all the updates.

    // Apply the gradient to w0.
    if(!nmf_mode)
      _apply_w0_gradient(buffer, l_grad, step_size);

    const size_t x_size = x.size();

    // Apply the gradient to w and V
    _apply_w_V_gradient(buffer, l_grad, ru.s_w_new_inv, ru.s_V_new_inv, x_size, step_size);

    if(enable_item_locking)
      item_lock.unlock();

    // Flush all registers to memory.
    asm volatile("" ::: "memory");

    ////////////////////////////////////////////////////////////
    // Step 5: Return the state of the model at the old value
    // Make sure we have the most recent
    double loss_value = loss_model.loss(fx_value, y);

    // The following commented code is kept in here for possible future debugging.

    // if(!std::isfinite(loss_value)
    //    || std::fabs(model->w0) > 1000
    //    || !std::isfinite(_fill_buffer_calc_value(buffer, x, s.s_w_old, s.s_V_old) )) {

    //   logstream(LOG_WARNING) << "Thread " << thread_idx
    //                          << "; w0 = " << model->w0
    //                          << "; buffered w0 = " << buffer.w0
    //                          << "; fx_value = " << fx_value
    //                          << "; l_grad = " << l_grad
    //                          << "; s.s_w_new_inv = " << s.s_w_new_inv
    //                          << "; s.s_V_new_inv = " << s.s_V_new_inv
    //                          << "; step_size = " << step_size
    //                          << "; y = " << y
    //                          << std::endl;


    //   for(size_t i = 0; i < size_t(model->w.size()); ++i) {
    //     float v = model->w[i];
    //     if(! (std::fabs(v) < 1000) ) {
    //       logstream(LOG_WARNING) << "Thread " << thread_idx
    //                              << "; w[" << i << "] = " << v
    //                              << std::endl;
    //     }
    //   }

    // }

    return loss_value;
  }


  /** Calculate the gradient with respect to a single observation,
   * then applies it.  Used by the basic sgd solver; this one is used
   * for the second_order model.
   *
   * x is the observation vector, in standard ml_data_entry format,
   * formed by ml_data_iterator.fill_observation(...).
   *
   * struct ml_data_entry {
   *  size_t column_index;      // Column id
   *  size_t index;             // Local index within the column.
   *  double value;             // Value
   * };
   */

  GL_HOT_INLINE_FLATTEN
  inline double apply_sgd_step(size_t thread_idx,
                               const std::vector<v2::ml_data_entry>& x,
                               double y,
                               double step_size) {
    return apply_sgd_step(thread_idx, x, y, step_size, true);
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Calculate the gradient with respect to a single observation,
   * then applies it.  Used by the basic sgd solver; this one is used
   * for the first order model.
   *
   * x is the observation vector, in standard ml_data_entry format,
   * formed by ml_data_iterator.fill_observation(...).
   *
   * struct ml_data_entry {
   *  size_t column_index;      // Column id
   *  size_t index;             // Local index within the column.
   *  double value;             // Value
   * };
   */
  double apply_pairwise_sgd_step(
      size_t thread_idx,
      const std::vector<v2::ml_data_entry>& x_positive,
      const std::vector<v2::ml_data_entry>& x_negative,
      double step_size) GL_HOT_FLATTEN {
    sgd_processing_buffer& buffer_1 = buffers[thread_idx];
    sgd_processing_buffer& buffer_2 = alt_buffers[thread_idx];

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Step 1: Pull in a snapshot of the current model values.
    // Calculate the current linear predictor fx along the way.

    DASSERT_GE(buffer_1.v.size(), x_positive.size());
    DASSERT_GE(buffer_2.v.size(), x_negative.size());

    // Check that the user index is the same.  For this we can ignore that.
    DASSERT_EQ(x_positive[0].index, x_negative[0].index);
    DASSERT_NE(x_positive[1].index, x_negative[1].index);

    auto s = _apply_regularization_update(step_size);

    double fx_diff_value = (_fill_buffer_calc_value(buffer_1, x_positive, s.s_w_old, s.s_V_old)
                            - _fill_buffer_calc_value(buffer_2, x_negative, s.s_w_old, s.s_V_old));

    // Everything needed for the prediction fx should now be
    // calculated; go ahead and use that to get the gradient of the
    // loss function at fx_value.
    double l_grad = loss_model.loss_grad(fx_diff_value, 0);

    if(! (std::fabs(l_grad) < 1e-16) ) {

      // No need to apply gradient to w0; this just gets canceled out.

      // Apply gradient to the w and V terms.
      _apply_w_V_gradient(buffer_1, l_grad, s.s_w_new_inv, s.s_V_new_inv,
                          x_positive.size(), step_size);

      _apply_w_V_gradient(buffer_2, -l_grad, s.s_w_new_inv, s.s_V_new_inv,
                          x_positive.size(), step_size);
    }

    return loss_model.loss(fx_diff_value, 0);
  }

};

}}

#endif
