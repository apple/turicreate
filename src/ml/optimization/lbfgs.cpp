#include <ml/optimization/lbfgs.hpp>
#include <core/logging/table_printer/table_printer.hpp>

namespace turi {
namespace optimization {

// This version is provided to ease the transition between the non-iterative
// solver (which includes significant printing stuff) and the iterative solver.
solver_return lbfgs_compat(
    std::shared_ptr<first_order_opt_interface> model,
    const DenseVector& init_point,
    const std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface>& reg) {
  timer t;
  t.start();

  // Get the max number of iterations to perform.
  auto it_n = opts.find("max_iterations");

  size_t num_iterations = (it_n != opts.end())
                              ? it_n->second
                              : default_solver_options.at("max_iterations");

  lbfgs_solver solver(model);

  ///////////////////////
  // ALL the setup stuff
  solver.setup(init_point, opts, reg);

  bool simple_mode = opts.count("simple_mode") && (opts.at("simple_mode"));

  if (!simple_mode) {
    logprogress_stream << "Starting L-BFGS " << std::endl;
    logprogress_stream
        << "--------------------------------------------------------"
        << std::endl;
    std::stringstream ss;
    ss.str("");
  } else {
    logprogress_stream << "Calibrating solver; this may take some time."
                       << std::endl;
  }

  // Print status
  auto header =
      (simple_mode ? model->get_status_header({"Iteration", "Elapsed Time"})
                   : model->get_status_header(
                         {"Iteration", "Passes", "Step size", "Elapsed Time"}));

  table_printer printer(header);
  printer.print_header();

  auto make_solver_return = [&](const solver_status& stats) {
    solver_return sr;
    sr.iters = stats.iteration;
    sr.solve_time = stats.solver_time;
    sr.solution = stats.solution;
    sr.gradient = stats.gradient;
    sr.hessian = stats.hessian;
    sr.residual = stats.residual;
    sr.func_value = stats.function_value;
    sr.func_evals = stats.num_function_evaluations;
    sr.gradient_evals = stats.num_gradient_evaluations;
    sr.num_passes = stats.num_function_evaluations;
    sr.status = stats.status;
    sr.progress_table = printer.get_tracked_table();
    return sr;
  };

  for (size_t i = 0; i < num_iterations; ++i) {
    bool result = solver.next_iteration();

    // Print progress
    std::vector<std::string> stat_info =
        (simple_mode
             ? std::vector<std::string>{std::to_string(i),
                                        std::to_string(t.current_time())}
             : std::vector<std::string>{
                   std::to_string(i),
                   std::to_string(solver.status().num_function_evaluations),
                   std::to_string(solver.status().step_size),
                   std::to_string(t.current_time())});

    std::vector<std::string> row =
        model->get_status(solver.status().solution, stat_info);
    printer.print_progress_row_strs(i + 1, row);

    if (result) {
      printer.print_footer();
      return make_solver_return(solver.status());
    }
  }

  solver_status status = solver.status();
  status.status = OPTIMIZATION_STATUS::OPT_ITERATION_LIMIT;

  printer.print_footer();

  return make_solver_return(status);
}

////////////////////////////////////////////////////////////////////////////////
//  The non-printing simple version of this.

solver_status lbfgs(std::shared_ptr<first_order_opt_interface> model,
                    const DenseVector& init_point,
                    const std::map<std::string, flexible_type>& opts,
                    const std::shared_ptr<smooth_regularizer_interface>& reg) {
  // Get the max number of iterations to perform.
  auto it_n = opts.find("max_iterations");
  size_t num_iterations = (it_n != opts.end())
                              ? it_n->second
                              : default_solver_options.at("max_iterations");

  lbfgs_solver solver(model);

  solver.setup(init_point, opts, reg);

  for (size_t i = 0; i < num_iterations; ++i) {
    bool result = solver.next_iteration();

    if (result) {
      return solver.status();
    }
  }

  solver_status status = solver.status();
  status.status = OPTIMIZATION_STATUS::OPT_ITERATION_LIMIT;
  return status;
}

////////////////////////////////////////////////////////////////////////////////

void lbfgs_solver::setup(
    const DenseVector& init_point,
    const std::map<std::string, flexible_type>& opts,
    const std::shared_ptr<smooth_regularizer_interface>& _reg) {
  m_status = solver_status();
  reg = _reg;

  auto get_param = [&](const std::string& n) {
    auto it = opts.find(n);
    return (it != opts.end()) ? it->second : default_solver_options.at(n);
  };

  lbfgs_memory_level = get_param("lbfgs_memory_level");
  convergence_threshold = get_param("convergence_threshold");
  m_status.step_size = 1.0;

  num_variables = model->num_variables();  // Dimension of point
  DASSERT_EQ(num_variables, init_point.size());

  // Set up the internal parts of the LBFGS information.
  y.resize(num_variables, lbfgs_memory_level);
  y.setZero();

  s.resize(num_variables, lbfgs_memory_level);
  s.setZero();

  q.resize(num_variables);
  q.setZero();

  rho.resize(lbfgs_memory_level);
  rho.setZero();

  alpha.resize(lbfgs_memory_level);
  alpha.setZero();

  m_status.solution = init_point;

  m_status.gradient.resize(num_variables);
  m_status.gradient.setZero();

  gradient.resize(num_variables);
  previous_gradient.resize(num_variables);

  // Initialize the statistics
  m_status.status = OPTIMIZATION_STATUS::OPT_IN_PROGRESS;
  m_status.iteration = 0;
  m_status.function_value = NAN;

  function_scaling_factor = 1.0;
}

/////////////////////////////////////////////////////////////////////////////////

bool lbfgs_solver::next_iteration() {
  compute_timer.start();

  // Set up some convenience notations to make the expressions below more
  // compact.
  const size_t m = lbfgs_memory_level;

  bool force_step_size_recompute = false;

  // A function to fill out the status before return.
  auto fill_current_status = [&](OPTIMIZATION_STATUS status) {
    m_status.status = status;
    m_status.solver_time += compute_timer.current_time();
  };

  // Set up references to the containers already held in the stats
  DenseVector& point = m_status.solution;
  size_t current_iteration = m_status.iteration;

  // Record the previous gradient and function_value.
  std::swap(
      previous_gradient,
      gradient);  // Fine to swap; scaled_gradient is about to be rewritten

  double previous_function_value = function_value;

  size_t recompute_count = 0;
RECOMPUTE_AT_NEW_POINT:;
  // Computing the gradient and value of the current point.  These get scaled
  // for use in our problem.
  model->compute_first_order_statistics(point, m_status.gradient,
                                        m_status.function_value);
  ++m_status.num_function_evaluations;
  ++m_status.num_gradient_evaluations;

  // Check for nan's in the function value.
  if (!std::isfinite(m_status.function_value)) {
    fill_current_status(OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR);
    return true;
  }

  // If it's the first go, set the function scaling factor to something that
  // is a good guess for some level of numerical stability.
  if (current_iteration == 0) {
    function_scaling_factor =
        1.0 / (1.0 + m_status.gradient.cwiseAbs().maxCoeff() +
               std::abs(m_status.function_value));
    convergence_threshold *= function_scaling_factor;
  }

  // Compute
  if (reg != nullptr) {
    // Use the local gradient as the intermediate buffer, as it will get
    // overwritten in just a bit.
    reg->compute_gradient(point, gradient);
    double reg_func_value = reg->compute_function_value(point);

    gradient = function_scaling_factor * (gradient + m_status.gradient);
    function_value =
        function_scaling_factor * (m_status.function_value + reg_func_value);
  } else {
    gradient = function_scaling_factor * m_status.gradient;
    function_value = function_scaling_factor * m_status.function_value;
  }

  // After the first two initial iterations, which call the more_thuente step
  // size routines to set the step sizes, we test to see if we should back up
  // from the current point towards the previous point.  Specifically, we:
  //
  // 1. Agressively back up if the previous value was better than this one.
  //
  // 2. Somewhat back up if the projection of the gradients onto the line between
  //    the previous point and the current point shows that there was a point
  //    inbetween with a better solution than the new one.  In that case, see if that better
  //    point is sufficiently different than the current one.
  //
  if (current_iteration >= 2
      && ( (previous_function_value > function_value && recompute_count < 3)
          || recompute_count == 0)) {
    // Is the projection of the gradient on the line from the previous point
    // to the current point pointing back towards the original point?
    // If it is, then there is a more minimal point between this point and the
    // previous that would be a better solution. Thus we should do a quick
    // bisection on an approximation of the 1d function given by the values and projected gradients
    // on either side of this quadrant.  This will, under
    // convexity, be a better solution than the current one.
    double dist = delta_point.norm();
    double g_cur = gradient.dot(delta_point) / dist;

    if (g_cur > 0) {

      double g_prev = previous_gradient.dot(delta_point) / dist;
      DASSERT_LE(g_prev, 1e-4);
      DASSERT_GE(g_cur, -1e-4);

      // How far should have we moved along this path?
      double t = gradient_bracketed_linesearch(dist, previous_function_value,
                                               function_value, g_prev, g_cur);

      DASSERT_LE(t, dist);
      DASSERT_GE(t, 0);

      // If it seems that this reduction is too drastic, then trigger a
      // full recomputation of the step size, or we've already tried this method
      // once with no avail, then force a rigorous recomputation of the step
      // size.
      if (t < 0.1 * dist || recompute_count == 2) {
        force_step_size_recompute = true;
      }

      // It's only really worth it to recompute things again if the new point
      // will be far enough away from the current.
      if (t < 0.8 * dist || previous_function_value > function_value) {

        // Readjust the step size.
        m_status.step_size *= (t / dist);

        // Back the point up the appropriate amount.
        // (point = previous_point + (t/dist)*delta_point = point - (1 - t/dist)
        // * delta_point;
        point -= (1.0 - t / dist) * delta_point;
        delta_point *= (t / dist);

        // Go back and restart at the new location.
        ++recompute_count;
        goto RECOMPUTE_AT_NEW_POINT;
      }
    }
  }

  double residual = compute_residual(gradient);

  // Nan Checking!
  if (!std::isfinite(residual)) {
    fill_current_status(OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW);
    return true;
  }

  // Have we converged yet?
  if (residual < convergence_threshold) {
    fill_current_status(OPTIMIZATION_STATUS::OPT_OPTIMAL);
    return true;
  }

  if (current_iteration == 0) {
    // Initialize with a line search
    ls_return lsm_status =
        more_thuente(*model, m_status.step_size, function_value, point,
                     gradient, -gradient, function_scaling_factor, reg);

    m_status.step_size = lsm_status.step_size;

    // Add info from line search
    m_status.num_function_evaluations += lsm_status.func_evals;
    m_status.num_gradient_evaluations += lsm_status.gradient_evals;

    // Line search failed
    if (lsm_status.status == false) {
      fill_current_status(OPTIMIZATION_STATUS::OPT_LS_FAILURE);
      return true;
    }

    // Store this delta for use in the next iteration of the algorithm
    point += (delta_point = (-m_status.step_size) * gradient);

  } else {
    size_t store_point = (current_iteration - 1) % m;

    // Store the gradient differences, step difference and rho for the next
    // iteration.
    s.col(store_point) = delta_point;
    y.col(store_point) = gradient - previous_gradient;
    rho(store_point) = 1.0 / (delta_point.dot(y.col(store_point)));

    // Two loop recursion to compute the direction
    // Algorithm 7.4 of Reference [1]

    q = gradient;

    /**
     *  Data is stored in a cyclic format using the following indexiing:
     *
     *   Iteration              Storage location
     *  *****************************************************
     *     iter-1               store_point
     *     iter-2               (store_point + 1) % m
     *      ...                  ...
     *     iter-m               (store_point + m - 1) % m
     *
     **/

    for (size_t j = 0; j < std::min(current_iteration, m); ++j) {
      size_t i = (store_point + m - j) % m;
      alpha(i) = rho(i) * s.col(i).dot(q);
      q -= alpha(i) * y.col(i);
    }

    // Scaling factor according to Pg 178 of [1]. This ensures that the
    // problem is better scaled and that a step size of 1 is mostly accepted.
    q *= 1.0 / (y.col(store_point).squaredNorm() * rho(store_point));

    for (size_t j = std::min(current_iteration, m); (j--) > 0;) {
      size_t i = (store_point + m - j) % m;
      double beta = rho(i) * y.col(i).dot(q);
      q += s.col(i) * (alpha(i) - beta);
    }

    // Check if we need to retune the step size.  Doing this is a bit
    // arbitrary -- essentially, if the improvement is "good enough",
    // then we should keep running with the current step size instead of
    // spending time recomputing the step size.
    //
    // The criteria we use here is to look at the difference of the gradient
    // between the current point and the previous point.  If these are too
    // similar, it indicates that the step size should have been larger.  If
    // they are antialigned then it indicates that it should have been smaller.
    if (current_iteration == 1
        || (function_value > previous_function_value)
        || force_step_size_recompute
        || (std::pow(gradient.dot(previous_gradient), 2)
            > 0.9 * gradient.squaredNorm() * previous_gradient.squaredNorm())) {
      // Reset the step size.
      ls_return lsm_status =
          more_thuente(*model, m_status.step_size, function_value, point,
                       gradient, -q, function_scaling_factor, reg);

      // Line search failed
      if (lsm_status.status == false) {
        fill_current_status(OPTIMIZATION_STATUS::OPT_LS_FAILURE);
        return true;
      }

      m_status.step_size = lsm_status.step_size;

      // Record statistics from line search
      m_status.num_function_evaluations += lsm_status.func_evals;
      m_status.num_gradient_evaluations += lsm_status.gradient_evals;

    } else {
      // If the step size is less than 1, which is usually okay per the scaling
      // above, then we should increase it slowly to get back towards 1 so that
      // we don't hit a bad spot and then forever set the step size to be
      // unnecessarily tiny and never retrigger more_thuente to reset it to
      // something where the conditions are good. In the worst case, we'll
      // trigger more_thuente again, which isn't the worst thing.
      if (m_status.step_size < 1) {
        m_status.step_size = std::min(1.0, 1.25 * m_status.step_size);
      }
    }

    point += (delta_point = -m_status.step_size * q);
  }

  // Check up on the statisics of how we moved.

  // Numerical error: Insufficient progress.
  if (delta_point.squaredNorm() <= OPTIMIZATION_ZERO * OPTIMIZATION_ZERO) {
    fill_current_status(OPTIMIZATION_STATUS::OPT_NUMERIC_ERROR);
    return true;
  }

  // Numerical error: Numerical overflow. (Step size was too large)
  if (!delta_point.array().isFinite().all()) {
    fill_current_status(OPTIMIZATION_STATUS::OPT_NUMERIC_OVERFLOW);
    return true;
  }

  // Now, report that all is well and return.
  fill_current_status(OPTIMIZATION_STATUS::OPT_IN_PROGRESS);

  m_status.residual = residual;
  ++m_status.iteration;

  return false;
}

}  // namespace optimization
}  // namespace turi
