/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_OPTIMIZATION_INTERFACE_H_
#define TURI_OPTIMIZATION_INTERFACE_H_

#include <string>
#include <flexible_type/flexible_type.hpp>
#include <numerics/armadillo.hpp>
#include <numerics/armadillo.hpp>
#include <sframe/sframe.hpp>

// TODO: List of todo's for this file
//------------------------------------------------------------------------------
// 

namespace turi {

// Typedefs for matrices and vectors
typedef arma::vec  DenseVector;
typedef arma::mat DenseMatrix;
typedef arma::vec DiagonalMatrix;  // The "diaganol matrix" aspect of this is declared using a diagmat(...) expression operating on a vector.
typedef sparse_vector<double> SparseVector;
typedef arma::sp_mat SparseMatrix;

namespace optimization { 


/**
 * \ingroup group_optimization
 * \addtogroup optimization_config Optimization Model Types And Config
 * \{
 */

/// Solver options type is a map from string to flexible_type.  
const std::map<std::string, flexible_type> default_solver_options {
  {"convergence_threshold", 1e-2},
  {"step_size", 1.0},
  {"lbfgs_memory_level", 3},
  {"mini_batch_size", 1000},
  {"max_iterations", 10},
  {"auto_tuning", true},
};

/// Types of the solver options
const std::map<std::string, flex_type_enum> default_solver_option_types {
  {"convergence_threshold", flex_type_enum::FLOAT},
  {"step_size", flex_type_enum::FLOAT},
  {"lbfgs_memory_level", flex_type_enum::INTEGER},
  {"mini_batch_size", flex_type_enum::INTEGER},
  {"max_iterations", flex_type_enum::INTEGER},
  {"auto_tuning", flex_type_enum::INTEGER},
};


// Structures & Enums
// ---------------------------------------------------------------------------

/// Optimization status
enum class OPTIMIZATION_STATUS { 
OPT_UNSET= 0,             ///< Optimizer wasn't called 
OPT_LOADED = 1,           ///< Model was loaded but the solution was not found.	
OPT_OPTIMAL = 2,          ///< Optimal solution found
OPT_ITERATION_LIMIT = 3,  ///< Iteration limit reached.
OPT_TIME_LIMIT = 4,       ///< Time limit reached.
OPT_INTERRUPTED = 5,     	///< Optimization terminated by user.
OPT_NUMERIC_ERROR = 6,    ///< Numerical underflow (not enough progress).
OPT_NUMERIC_OVERFLOW = 7, ///< Numerical overflow. Step size parameter may be too large.
OPT_LS_FAILURE= 8,        ///< Line search iteration limit hit.
}; 



// Global constants for the optimization library
const double OPTIMIZATION_INFTY = 1.0e20;  ///< Optimization method infinity
const double OPTIMIZATION_ZERO = 1.0e-10;  ///< Optimization method zero.

// Line search parameters
const double LS_INFTY = 1.0e20;       ///< No steps that are too large.
const double LS_ZERO = 1.0e-9;        ///< Smallest allowable step length.
const double LS_C1 = 1.0e-4;          ///< Line search sufficient decrease parameters.
const double LS_C2 = 0.7;           ///< Line search curvature approximation.
const int LS_MAX_ITER = 20;        ///< Num func evals before a failed line search.
const double LS_SAFE_GUARD = 5.0e-2;  ///< Safe guarding tolerance for line search.
const double LS_MAX_STEP_SIZE = 25.0; ///< Max allowable step size.

/// Finite difference parameters (required for gradient checking)
const double FINITE_DIFFERENCE_EPSILON = 1e-5; 

/**
 * Solver return type structure.
 * \note The number of passes over the data need not be the same thing 
 * as the number of iterations. Each iteration could require multiple passes
 * over the data (Eg. for line search).
*/
struct _solver_return{
  
  int iters = -1;                               /*!< Iterations taken */
  double solve_time = -1;                       /*!< Wall clock time (s) */
  DenseVector solution;                         /*!< Solution */
  DenseVector gradient;                         /*!< Terminal gradient */
  DenseMatrix hessian;                          /*!< Terminal hessian */
  double residual;                              /*!< Residual norm */
  double func_value;                            /*!< Function value */
  int func_evals = 0;                           /*!< Function evals */
  int gradient_evals = 0;                       /*!< Gradient evals */
  int num_passes = 0;                           /*!< Number of passes */
  OPTIMIZATION_STATUS status = OPTIMIZATION_STATUS::OPT_UNSET;  /*!< Status */
  sframe progress_table = sframe();

};
typedef struct _solver_return solver_return;

/**
 * Line search return type.
*/
struct _ls_return{
  
  double step_size = 1.0;                       /*!< Step size found */
  bool status = false;                          /*!< Line search status*/
  int func_evals = 0;                           /*!< Function evals */
  int gradient_evals = 0;                       /*!< Gradient evals */
  int num_passes = 0;                           /*!< Number of passes */

};
typedef struct _ls_return ls_return;


// Classes
// ---------------------------------------------------------------------------

/**
 * The interface to inherit from to describe a first order optimization model.
 *
 * This model must implement methods to compute mini-batch gradient, and
 * function value.
 */
class first_order_opt_interface {
 
  public:
  
  /**
   * Default desctuctor. 
   */
  virtual ~first_order_opt_interface();
  

  
  /**
   * Get the number of examples in the dataset (Required for SGD).
   *
   * \returns Number of examples in the data. 
   */
  virtual size_t num_examples() const = 0;

  /**
   * Get the number of variables in the optimization problem.
   *
   * \returns Number of variables (features) in the optimization. 
   *
   * \note Statisticians beware. Bias terms are variables in the
   * optimization problem.
   */
  virtual size_t num_variables() const = 0;

  /**
   * Compute first order statistics at the given point. (Gradient & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   * \param[in]  mbStart         Minibatch start index
   * \param[in]  mbSize          Minibatch size (-1 implies all)
   *
   * \warning DO NOT USE this if your model is truly sparse.
   */
  virtual void compute_first_order_statistics(const DenseVector &point,
      DenseVector& gradient, double & function_value, const size_t mbStart = 0,
      const size_t mbSize = -1) = 0;
  
  /** 
   * Optimizations for performance reasons.
   * -------------------------------------------------------------------------
   */	
  
  /**
   * Compute the function value at a given point.
   * \param[in]  point   Point at which we are computing the gradient.
   * \param[in]  mbStart Minibatch start index
   * \param[in]  mbSize  Minibatch size (-1 implies all)
   * \returns            Function value at "point"
   */
  virtual double compute_function_value(const DenseVector &point, const size_t
      mbStart = 0, const size_t mbSize = -1);

  /**
   * Compute a gradient at the given point.
   *
   * \param[in]  point   Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   * \param[in]  mbStart Minibatch start index
   * \param[in]  mbSize  Minibatch size (-1 implies all)
   *
   * \warning See warning on "sparse" compute_gradient to see a potential danger
   * of an infinite loop if you don't imeplement at least one of "dense"
   * or "sparse" versions of compute_gradient.
   *
   * \warning DO NOT USE this if your model is truly sparse.
   */
  virtual void compute_gradient(const DenseVector &point, DenseVector&
      gradient, const size_t mbStart = 0, const size_t mbSize = -1);


  
  /**
   * Reset the state of the model's "randomness" source.
   *
   * \param[in] seed Seed that is the source of randomness.
   *
   */
  virtual void reset(int seed);
  
  /**
   * Get strings needed to print the header for the progress table.
   *
   * \param[in] a vector of strings to print at the beginning of the header.
   */
  virtual std::vector<std::pair<std::string,size_t>> 
      get_status_header(const std::vector<std::string>& stats);

  /**
   * Get strings needed to print a row of the progress table.
   *
   * \param[in] a vector of model coefficients.
   * \param[in] a vector of stats to print at the beginning of each row
   */
  virtual std::vector<std::string> get_status(const DenseVector& coefs, 
                                              const std::vector<std::string>& stats);


};


/**
 *
 * The interface to inherit from to describe a second order optimization model.
 *
 * This model must implement methods to compute hessian as well as gradient.
 */
class second_order_opt_interface: public first_order_opt_interface {

  public:
  
  /**
   * Default desctuctor. 
   */
  virtual ~second_order_opt_interface();
  
  /**
   * Compute second order statistics at the given point. (Hessian, Gradient
   * & Function value)
   *
   * \param[in]  point           Point at which we are computing the stats.
   * \param[out] hessian         Hessian computation (dense)
   * \param[out] gradient        Dense gradient
   * \param[out] function_value  Function value
   *
   */
  virtual void compute_second_order_statistics(const DenseVector &point,
      DenseMatrix& Hessian, DenseVector& gradient, double & function_value) = 0;


  /** 
   * Optimizations for performance reasons.
   * -------------------------------------------------------------------------
   */	

  /**
   *
   * Compute the hessian at the given point
   * \param[in]  point   Point at which we are computing the function value.
   * \param[out] hessian Returns a dense hessian matrix.
   *
   * \note Hessians are always dense matrices. Even if they are sparse, it is
   * still really hard to factorize them by make sure the factors stay sparse.
   *
   */
  virtual void compute_hessian(const DenseVector& point, DenseMatrix& hessian);
  

};

/// \}
}

} // turicreate

#endif 

