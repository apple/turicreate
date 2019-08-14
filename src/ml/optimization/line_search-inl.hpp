/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LINE_SEARCH_H_
#define TURI_LINE_SEARCH_H_

// Types
#include <core/data/flexible_type/flexible_type.hpp>

// Optimization
#include <ml/optimization/optimization_interface.hpp>
#include <ml/optimization/regularizer_interface.hpp>
#include <Eigen/Core>
#include <core/logging/assertions.hpp>

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 1. Feature: Armijo cubic interpolation line search.
// 2. Feature: Wolfe cubic interpolation line search.
// 3. Optimization: Add accepted function value to the ls_return structure
//    (reduces 1 func eval per iteration)
//

namespace turi {

namespace optimization {


/**
 * \ingroup group_optimization
 * \addtogroup line_search Line Search
 * \{
 */


/** "Zoom" phase for More and Thuente line seach.
 *
 * \note Applicable for smooth functions only.
 *
 * This code is a C++ port of Jorge Nocedal's implementaiton of More and
 * Thuente [2] line search. This code was availiable at
 * http://www.ece.northwestern.edu/~nocedal/lbfgs.html
 *
 * Nocedal's Condition for Use: This software is freely available for
 * educational or commercial purposes. We expect that all publications
 * describing work using this software quote at least one of the references
 * given below. This software is released under the BSD License
 *
 * The purpose of cstep is to compute a safeguarded step for a linesearch and
 * to update an interval of uncertainty for a minimizer of the function.
 *
 * The parameter stx contains the step with the least function value. The
 * parameter stp contains the current step. It is assumed that the derivative
 * at stx is negative in the direction of the step. If brackt is set true then
 * a minimizer has been bracketed in an interval of uncertainty with endpoints
 * stx and sty.
 *
 *
 * \param[in] stx  Best step obtained so far.
 * \param[in] fx   Function value at the best step so far.
 * \param[in] dx   Directional derivative at the best step so far.
 * \param[in] sty  Step at the end point of the uncertainty interval.
 * \param[in] fy   Function value at end point of uncertainty interval.
 * \param[in] dy   Directional derivative at end point of uncertainty interval.
 * \param[in] stp  Current step.
 * \param[in] fp   Function value at current step.
 * \param[in] dp   Directional derivative at the current step.
 * \param[in] brackt Has the minimizer has been bracketed?
 * \param[in] stpmin Min step size
 * \param[in] stpmax Max step size.
 *
 *
 */
inline bool cstep(double &stx, double &fx, double &dx,
           double &sty, double &fy, double &dy,
           double &stp, double &fp,  double &dp,
           bool &brackt, double stpmin, double stpmax){


  const double p66 = 0.66;
  bool info = false;

  // Check the input parameters for errors.
  // This should not happen.
  if ((brackt && (stp <= std::min(stx,sty) || stp >= std::max(stx,sty)))
      || (dx*(stp-stx) >= LS_ZERO) || (stpmax < stpmin)){
     return info;
  }


  // Determine if the derivatives have opposite sign.
  double sgnd = dp*(dx/std::abs(dx));

  // Local variables
  bool bound;
  double theta, s, gamma, p, q, r, stpc, stpq, stpf;

  // First case.
  // -------------------------------------------------------------------------
  // A higher function value. The minimum is bracketed. If the cubic step is
  // closer to stx than the quadratic step, the cubic step is taken, else the
  // average of the cubic and quadratic steps is taken.
  if (fp > fx){

    info = true;
    bound = true;
    theta = 3*(fx - fp)/(stp - stx) + dx + dp;
    s = std::max(std::abs(theta), std::max(std::abs(dx),std::abs(dp)));
    gamma = s*sqrt(pow(theta/s,2) - (dx/s)*(dp/s));

    if (stp < stx)
     gamma = -gamma;

    p = (gamma - dx) + theta;
    q = ((gamma - dx) + gamma) + dp;
    r = p/q;
    stpc = stx + r*(stp - stx);
    stpq = stx + ((dx/((fx-fp)/(stp-stx)+dx))/2)*(stp - stx);

    if (std::abs(stpc-stx) < std::abs(stpq-stx)){
       stpf = stpc;
    } else{
      stpf = stpc + (stpq - stpc)/2;
    }

    brackt = true;

  }
  // Second case.
  // -------------------------------------------------------------------------
  // A lower function value and derivatives of opposite sign. The
  // minimum is bracketed. If the cubic step is closer to stx than the
  // quadratic (secant) step, the cubic step is taken, else the quadratic step
  // is taken.
  else if(sgnd <= 0){

    info = true;
    bound = false;

    theta = 3*(fx - fp)/(stp - stx) + dx + dp;
    s = std::max(std::abs(theta), std::max(std::abs(dx),std::abs(dp)));
    gamma = s*sqrt(pow(theta/s,2) - (dx/s)*(dp/s));

    if (stp > stx)
       gamma = -gamma;

    p = (gamma - dp) + theta;
    q = ((gamma - dp) + gamma) + dx;
    r = p/q;
    stpc = stp + r*(stx - stp);
    stpq = stp + (dp/(dp-dx))*(stx - stp);
    if (std::abs(stpc-stp) > std::abs(stpq-stp)){
       stpf = stpc;
    } else {
       stpf = stpq;
    }
    brackt = true;
  }
  // Third case.
  // -------------------------------------------------------------------------
  // A lower function value, derivatives of the same sign, and the
  // magnitude of the derivative decreases.  The cubic step is only used if the
  // cubic tends to infinity in the direction of the step or if the minimum of
  // the cubic is beyond stp. Otherwise the cubic step is defined to be either
  // stpmin or stpmax. The quadratic (secant) step is also computed and if the
  // minimum is bracketed then the the step closest to stx is taken, else the
  // step farthest away is taken.
  else if (std::abs(dp) < std::abs(dx)){

    info = true;
    bound = true;
    theta = 3*(fx - fp)/(stp - stx) + dx + dp;
    s = std::max(std::abs(theta), std::max(std::abs(dx),std::abs(dp)));

    // The case gamma = 0 only arises if the cubic does not tend to infinity
    // in the direction of the step.

    gamma = s*sqrt(std::max(0., pow((theta/s),2) - (dx/s)*(dp/s)));
    if (stp > stx){
        gamma = -gamma;
    }

    p = (gamma - dp) + theta;
    q = (gamma + (dx - dp)) + gamma;
    r = p/q;
    if((r < 0.0) && (std::abs(gamma) <= OPTIMIZATION_ZERO)){
       stpc = stp + r*(stx - stp);
    }else if (stp > stx){
       stpc = stpmax;
    }else{
       stpc = stpmin;
    }

    stpq = stp + (dp/(dp-dx))*(stx - stp);
    if (brackt){
       if (std::abs(stp-stpc) < std::abs(stp-stpq)){
          stpf = stpc;
       }else{
          stpf = stpq;
       }
    }else{
       if (std::abs(stp-stpc) > std::abs(stp-stpq)){
          stpf = stpc;
       }else{
          stpf = stpq;
       }
    }

  }
  // Fourth case.
  // -------------------------------------------------------------------------
  // A lower function value, derivatives of the same sign, and the magnitude of
  // the derivative does not decrease. If the minimum is not bracketed, the
  // step is either stpmin or stpmax, else the cubic step is taken.
  else{
     info = true;
     bound = false;
     if (brackt){
        theta = 3*(fp - fy)/(sty - stp) + dy + dp;
        s = std::max(std::abs(theta), std::max(std::abs(dy),std::abs(dp)));
        gamma = s*sqrt(pow(theta/s,2) - (dy/s)*(dp/s));

        if (stp > sty){
            gamma = -gamma;
        }
        p = (gamma - dp) + theta;
        q = ((gamma - dp) + gamma) + dy;
        r = p/q;
        stpc = stp + r*(sty - stp);
        stpf = stpc;
     }else if (stp > stx){
        stpf = stpmax;
     }else {
        stpf = stpmin;
     }
  }

  // Update the interval of uncertainty.
  // This update does not depend on the new step or the case analysis above.
  if (fp > fx){
     sty = stp;
     fy = fp;
     dy = dp;
  }else{

     if (sgnd < 0.0){
        sty = stx;
        fy = fx;
        dy = dx;
     }
     stx = stp;
     fx = fp;
     dx = dp;
  }

  // Compute the new step and safeguard it.
  stpf = std::min(stpmax,stpf);
  stpf = std::max(stpmin,stpf);
  stp = stpf;
  if (brackt && bound){
     if (sty > stx){
        stp = std::min(stx+p66*(sty-stx),stp);
     }else{
        stp = std::max(stx+p66*(sty-stx),stp);
     }
  }

  // cstep-failed
  if (info == false){
     logprogress_stream << "Warning:"
                        << " Unable to interpolate step size intervals."
                        << std::endl;
  }
  return info;

}



/**
 *
 * Compute step sizes for line search methods to satisfy Strong Wolfe conditions.
 *
 * \note Applicable for smooth functions only.
 *
 * This code is a C++ port of Jorge Nocedal's implementaiton of More and
 * Thuente [2] line search. This code was availiable at
 * http://www.ece.northwestern.edu/~nocedal/lbfgs.html
 *
 *   Line search based on More' and Thuente [1] to find a step which satisfies
 *   a Wolfe conditions for sufficient decrease condition and a curvature
 *   condition.
 *
 *   At each stage the function updates an interval of uncertainty with
 *   endpoints. The interval of uncertainty is initially chosen so that it
 *   contains a minimizer of the modified function
 *
 *        f(x+stp*s) - f(x) - ftol*stp*(gradf(x)'s).
 *
 *   If a step is obtained for which the modified function has a nonpositive
 *   function value and nonnegative derivative, then the interval of
 *   uncertainty is chosen so that it contains a minimizer of f(x+stp*s).
 *
 *   The algorithm is designed to find a step which satisfies the sufficient
 *   decrease condition
 *         f(x+stp*s) .le. f(x) + ftol*stp*(gradf(x)'s),          [W1]
 *   and the curvature condition
 *         std::abs(gradf(x+stp*s)'s)) .le. gtol*std::abs(gradf(x)'s).      [W2]
 *
 *  References:
 *
 *  (1) More, J. J. and D. J. Thuente. "Line Search Algorithms with
 *  Guaranteed Sufficient Decrease." ACM Transactions on Mathematical Software
 *  20, no. 3 (1994): 286-307.
 *
 * (2) Wright S.J  and J. Nocedal. Numerical optimization. Vol. 2.
 *                         New York: Springer, 1999.
 * \param[in] model Any model with a first order optimization interface.
 * \param[in] init_step Initial step size
 * \param[in] point Starting point for the solver.
 * \param[in] gradient Gradient value at this point (saves computation)
 * \param[in]    reg   Shared ptr to an interface to a smooth regularizer.
 * \returns stats Line searnorm ch return object
 *
*/
template <typename Vector>
inline ls_return more_thuente(
    first_order_opt_interface& model,
    double init_step,
    double init_func_value,
    DenseVector point,
    Vector gradient,
    DenseVector direction,
    double function_scaling = 1.0,
    const std::shared_ptr<smooth_regularizer_interface> reg=NULL,
    size_t max_function_evaluations = LS_MAX_ITER){


    // Initialize the return object
    ls_return stats;

    // Input checking: Initial step size can't be zero.
    if (init_step <= LS_ZERO){
      logprogress_stream << " Error:"
                         <<" \nInitial step step less than "<< LS_ZERO
                         << "." << std::endl;
      return stats;
    }


    // Check that the initia direction is a descent direction.
    // This can only occur of your gradients were computed incorrectly
    // or the problem is non-convex.
    DenseVector x0 = point;
    double Dphi0 = gradient.dot(direction);
    if ( (init_step <= 0) | (Dphi0 >= OPTIMIZATION_ZERO) ){
      logprogress_stream << " Error: Search direction is not a descent direction."
                          <<" \nDetected numerical difficulties." << std::endl;
    }

    // Initializing local variables
    // stx, fx, dgx: Values of the step, function, and derivative at the best
    //               step.
    //
    // sty, fy, dgy: Values of the step, function, and derivative at the other
    //               endpoint of the interval of uncertainty.
    //
    // st, f, dg   : Values of the step, function, and derivative at current
    //               step
    //
    // g           : Gradient w.r.t x and step (vector) at the current point
    //
    double stx = LS_ZERO;
    double fx = init_func_value;
    double dgx = Dphi0;                   // Derivative of f(x + s d) w.r.t s

    double sty = LS_ZERO;
    double fy = init_func_value;
    double dgy = Dphi0;

    double stp = init_step;
    double f = init_func_value;
    double dg = Dphi0;
    Vector g = gradient;
    DenseVector reg_gradient(gradient.size());

    // Interval [stmax, stmin] of uncertainty
    double stmax = LS_ZERO;
    double stmin = LS_MAX_STEP_SIZE;

    // Flags and local variables
    bool brackt = false;                 // Bracket or Zoon phase?
    bool stage1 = true;

    double wolfe_func_dec  = LS_C1*Dphi0;         // Wolfe condition [W1]
    double wolfe_curvature = LS_C2*Dphi0;         // Wolfe condition [W2]
    double width = stmax - stmin;
    double width2 = 2*width;


    // Constants used in this code.
    // (Based on http://www.ece.northwestern.edu/~nocedal/lbfgs.html)
    const double p5 = 0.5;
    const double p66 = 0.66;
    const double xtrapf = 4;
    bool infoc = true;                            // Zoom status

    // Start searching
    while (true){

      // Set the min and max steps based on the current interval of uncertainty.
      if (brackt == true){
        stmin = std::min(stx,sty);
        stmax = std::max(stx,sty);
      }else{
        stmin = stx;
        stmax = stp + xtrapf*(stp - stx);
      }

      // Force the step to be within the bounds
      stp = std::max(stp,LS_ZERO);
      stp = std::min(stp,LS_MAX_STEP_SIZE);

      // If an unusual termination is to occur then let 'stp' be the lowest point
      // obtained so far.
      if ( (infoc == false)
          || (brackt && (stmax-stmin <= LS_ZERO))){

         logprogress_stream << "Warning:"
            << " Unusual termination criterion reached."
            << "\nReturning the best step found so far."
            << " This typically happens when the number of features is much"
            << " larger than the number of training samples. Consider pruning"
            << " features manually or increasing the regularization value."
            << std::endl;
         stp = stx;
      }

      // Reached func evaluation limit -- return the best one so far.
      if (size_t(stats.func_evals) >= max_function_evaluations){
        stats.step_size = stx;
        stats.status = true;
        return stats;
      }

      // Evaluate the function and gradient at stp and compute the directional
      // derivative.
      point = x0 + stp * direction;
      model.compute_first_order_statistics(point, g, f);
      stats.num_passes++;
      stats.func_evals++;
      stats.gradient_evals++;
      if (reg != NULL){
        reg->compute_gradient(point, reg_gradient);
        f += reg->compute_function_value(point);
        g += reg_gradient;
      }

      if(function_scaling != 1.0) {
        f *= function_scaling;
        g *= function_scaling;
      }

      dg = g.dot(direction);

      double ftest = init_func_value + stp*wolfe_func_dec;

      // Termination checking
      // Note: There are many good checks and balances used in Nocedal's code
      // Some of them are overly defensive and should not happen.

      // Rounding errors
      if ( (brackt && ((stp <= stmin) || (stp >= stmax))) || (infoc == false)){
          logprogress_stream << "Warning: Rounding errors"
            << " prevent further progress. \nThere may not be a step which"
            << " satisfies the sufficient decrease and curvature conditions."
            << " \nTolerances may be too small or dataset may be poorly scaled."
            << " This typically happens when the number of features is much"
            << " larger than the number of training samples. Consider pruning"
            << " features manually or increasing the regularization value."
            << std::endl;
          stats.step_size = stp;
          stats.status = false;
          return stats;
      }

      // Step is more than LS_MAX_STEP_SIZE
      if ((stp >= LS_MAX_STEP_SIZE) && (f <= ftest) && (dg <= wolfe_func_dec)){
        logprogress_stream << "Warning: Reached max step size."
                           << std::endl;
        stats.step_size = stp;
        stats.status = true;
        return stats;
      }

      // Step is smaller than LS_ZERO
      if ((stp <= LS_ZERO) && ((f > ftest) || (dg >= wolfe_func_dec))){
        logprogress_stream << "Error: Reached min step size."
                           << " Cannot proceed anymore."
                           << std::endl;
        stats.step_size = stp;
        stats.status = false;
        return stats;
      }



      // Relative width of the interval of uncertainty is reached.
      if (brackt && (stmax-stmin <= LS_ZERO)){
        logprogress_stream << "Error: \nInterval of uncertainty"
                           << "lower than step size limit." << std::endl;
        stats.status = false;
        return stats;
      }

      // Wolfe conditions W1 and W2 are satisfied! Woo!
      if ((f <= ftest) && (std::abs(dg) <= -wolfe_curvature)){
        stats.step_size = stp;
        stats.status = true;
        return stats;
      }

      // Stage 1 is a search for steps for which the modified function has
      // a nonpositive value and nonnegative derivative.
      if ( stage1 && (f <= ftest) && (dg >= wolfe_curvature)){
             stage1 = false;
      }


      // A modified function is used to predict the step only if we have not
      // obtained a step for which the modified function has a nonpositive
      // function value and nonnegative derivative, and if a lower function
      // value has been  obtained but the decrease is not sufficient.

      if (stage1 && (f <= fx) && (f > ftest)){

         // Define the modified function and derivative values.
         double fm = f - stp*wolfe_func_dec;
         double fxm = fx - stx*wolfe_func_dec;
         double fym = fy - sty*wolfe_func_dec;
         double dgm = dg - wolfe_func_dec;
         double dgxm = dgx - wolfe_func_dec;
         double dgym = dgy - wolfe_func_dec;

         // Call cstep to update the interval of uncertainty and to compute the
         // new step.
         infoc = cstep(stx,fxm, dgxm,
                       sty, fym, dgym,
                       stp, fm, dgm,
                       brackt,
                       stmin,stmax);

         // Reset the function and gradient values for f.
         fx = fxm + stx*wolfe_func_dec;
         fy = fym + sty*wolfe_func_dec;
         dgx = dgxm + wolfe_func_dec;
         dgy = dgym + wolfe_func_dec;

      }else{

         // Call cstep to update the interval of uncertainty and to compute the
         // new step.
         infoc = cstep(stx,fx, dgx,
                       sty, fy, dgy,
                       stp, f, dg,
                       brackt,
                       stmin,stmax);

      }


      // Force a sufficient decrease in the size of the interval of uncertainty.
      if (brackt){
         if (std::abs(sty-stx) >= p66*width2){
           stp = stx + p5*(sty - stx);
         }

         width2 = width;
         width = std::abs(sty-stx);
      }

    } // end-of-while

    return stats;

}


/**
 *
 * Armijo backtracking to compute step sizes for line search methods.
 *
 * \note Applicable for smooth functions only.
 *
 * Line search based on an backtracking strategy to ensure sufficient
 * decrease in function values at each iteration.
 *
 * References:
 * (1) Wright S.J  and J. Nocedal. Numerical optimization. Vol. 2.
 *                         New York: Springer, 1999.
 *
 * \param[in] model Any model with a first order optimization interface.
 * \param[in] init_step Initial step size.
 * \param[in] init_func Initial function value.
 * \param[in] point     Starting point for the solver.
 * \param[in] gradient  Gradient value at this point.
 * \param[in] direction Direction of the next step .
 *
 * \returns stats Line searnorm ch return object
 *
*/
template <typename Vector>
inline ls_return armijo_backtracking(
    first_order_opt_interface& model,
    double init_step,
    double init_func_value,
    DenseVector point,
    Vector gradient,
    DenseVector direction){

    // Step 1: Initialize the function
    // ------------------------------------------------------------------------
    // Min function decrease according to Armijo conditions.
    // The choice of constants are based on Nocedal and Wright [1].
    double sufficient_decrease = LS_C1*(gradient.dot(direction));
    ls_return stats;
    double step_size = init_step;
    DenseVector new_point = point;

    // Step 2: Backtrack
    // -----------------------------------------------------------------------
    while (stats.func_evals <= LS_MAX_ITER && step_size >= LS_ZERO){

      // Check for sufficient decrease
      new_point = point + step_size * direction;
      if (model.compute_function_value(new_point) <= init_func_value
          + step_size * sufficient_decrease){

        stats.step_size = step_size;
        stats.status = true;
        return stats;

      }

      step_size *= 0.5;
      stats.func_evals += 1;

    }

    return stats;
}


/**
 *
 * Backtracking to compute step sizes for line search methods.
 *
 * \note Applicable for non-smooth functions.
 *
 * Line search based on an backtracking strategy to "some" decrease in function
 * values at each iteration.
 *
 * References:
 * (1) Wright S.J  and J. Nocedal. Numerical optimization. Vol. 2.
 *                         New York: Springer, 1999.
 *
 * \param[in] model Any model with a first order optimization interface.
 * \param[in] init_step Initial step size.
 * \param[in] init_func Initial function value.
 * \param[in] point     Starting point for the solver.
 * \param[in] gradient  Gradient value at this point.
 * \param[in] direction Direction of the next step .
 * \param[in] reg       Regularizer interface!
 *
 * \returns stats Line searnorm ch return object
 *
*/
template <typename Vector>
inline ls_return backtracking(
    first_order_opt_interface& model,
    double init_step,
    double init_func_value,
    DenseVector point,
    Vector gradient,
    DenseVector direction,
    const std::shared_ptr<regularizer_interface> reg=NULL){

    // Step 1: Initialize the function
    // ------------------------------------------------------------------------
    ls_return stats;
    double step_size = init_step;
    DenseVector delta_point(point.size());;
    DenseVector new_point(point.size());;

    // Step 2: Backtrack
    // -----------------------------------------------------------------------
    while (stats.func_evals <= LS_MAX_ITER && step_size >= LS_ZERO){

      // Check for sufficient decrease
      new_point = point + step_size * direction;
      if(reg != NULL){
        reg->apply_proximal_operator(new_point, step_size);
      }

      delta_point = new_point - point;
      if (model.compute_function_value(new_point) <=
             init_func_value + gradient.dot(delta_point)
                            + 0.5 * delta_point.squaredNorm()/step_size){

        stats.step_size = step_size;
        stats.status = true;
        return stats;

      }

      step_size *= 0.5;
      stats.func_evals += 1;

    }

    return stats;
}

/** This function estimates a minumum point between
 *  two function values with known gradients.
 *
 *  The minimum value must lie between the two function values, f1 and f2, and the
 *  gradients must indicate the minimum lies between the two values and that the
 *  function is convex.
 *
 *  Under these situations, a third degree polynomial / cubic spline can be fit
 *  to the two function points, and this is gauranteed to have a single minimum
 *  between f1 and f2.  This is what this function returns.
 *
 *  \param[in] dist  The distance between the points f1 and f2.
 *  \param[in] f1    The value of the function at the left point.
 *  \param[in] f2    The value of the function at the right point.
 *  \param[in] g1    The derivative of the function at the left point.
 *  \param[in] g2    The derivative of the function at the right point.
 *
 */
inline double gradient_bracketed_linesearch(double dist, double f1, double f2,
                                            double g1, double g2) {
  // Assume f1 is evaluated at 0, f2 is evaluated at 1.  To make the latter true,
  // we need to multiply the gradients by the appropriate scaling factor.
  g1 *= dist;
  g2 *= dist;

  // We have to have this such that a minimum point is between f1 and f2, which
  DASSERT_LE(g1, 1e-4);   // Condition 1
  DASSERT_GE(g2, -1e-4);  // Condition 2

  // Also make sure the function is convex;
  DASSERT_LE(f2 - g2, f1 + 1e-4);  // Condition 3
  DASSERT_LE(f1 + g1, f2 + 1e-4);  // Condition 4


  // Now, we have 4 known variables, so construct a 3rd order polynomial to
  // approximate the solution between the two points.  Then find the minimum of
  // that.

  // With the convexity coefficients above, a third order polynomial, given as
  //
  //   p(t) = a*t^3 + b*t^2 + c*t + d
  //
  // will have exactly one minima between 0 and 1 by the conditions above.
  //
  // The coefficients can be easily derived by:
  // p(0) = f1
  // p(1) = f2
  // p'(0) = g1
  // p'(1) = g2
  //
  // Some algebra yields:
  //
  const double a = -2*(f2 - f1) + (g2 + g1);
  const double b = 3*(f2 - f1) - g2 - 2*g1;
  const double c = g1;
  const double d = f1;

  // std::cout << "coeff = (" << a << ", " << b << ", " << c << std::endl;

  // Starting iterate
  double left = 0;
  double right = 1;

  double best_value = std::min(f1, f2);
  double best_loc = f1 < f2 ? 0 : 1;


  for(size_t m_iter = 0; m_iter < 32; ++m_iter) {

    double t = 0.5 * (right + left);

    // Make sure that the recent value is indeed better
    double v = d + c*t + b*t*t + a*t*t*t;
    // double vpp = 2*b + 6*a*t;

    if(v < best_value) {
      best_value = v;
      best_loc = t;
    }

    if(right - left < 1e-6) {
      break;
    }

    double vp = c + 2*b*t + 3*a*t*t;

    if(vp > 0) {
      right = t;
    } else {
      left = t;
    }

  }

  return dist * best_loc;
}




/// \}

} // optimizaiton

} // turicreate

#endif
