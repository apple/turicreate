#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// Optimization Interface
#include <optimization/optimization_interface.hpp>

// Solvers
#include <optimization/utils.hpp>
#include <optimization/newton_method-inl.hpp>
#include <optimization/gradient_descent-inl.hpp>
#include <optimization/accelerated_gradient-inl.hpp>
#include <optimization/lbfgs.hpp>


using namespace turi;
typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;


/**
 * Solver interface for a sample problem. (Logistic regression)
 *
 * In this test case, we check all the algorithms using logistic regression
 * as a sample problem.
 *
 * Algorithms tested:
 *
 * (1) LBFGS
 * (2) FISTA
 * (3) GD
 * (4) Newton
 *
 *
 */
class opt_interface: public optimization::second_order_opt_interface
  {

  protected:


  size_t examples;                /**< Number of examples */
  size_t variables;               /**< Number of variables */
  size_t n_threads=4;             /** < Num threads */
  size_t it = 0;                  /** < Current example being used in SGD */
  size_t coordinate = 0;          /** < Current coordinate being used in SCD */

  DenseMatrix A;
  DenseVector b;

  public:


  /**
   * Default constructor.
   */
  opt_interface(DenseMatrix _A, DenseVector _b){
    examples = _A.rows();
    variables = _A.cols();
    A = _A;
    b = _b;
  }

  /**
   * Default destructor.
   */
  ~opt_interface(){
  }

  /**
   * Get the number of examples in the model
   *
   * \returns Number of examples
   */
  size_t num_examples() const{
    return examples;
  }

  /**
   * Get the number of variables in the model
   *
   * \returns Number of examples
   */
  size_t num_variables() const{
    return variables;
  }

  /**
   * Reset the source of randomness in the model.
   *
   * \param[in]  seed  Currently, this seed is the "start_index"
   */
  void reset(int seed){
    it = 0;
    coordinate = 0;
  }

  /**
   * Compute the hessian (for logistic regression) at the given point
   *
   * \param[in]  point   Point at which we are computing the function value.
   * \param[out] hessian Returns a dense hessian matrix.
   *
   * See logistic_regression.cpp for details
   */
  void compute_hessian(const DenseVector& point, DenseMatrix& hessian){
    // Code copied from logistic_regression.cpp
    hessian.setZero();
    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double kernel = exp(-1.0 * (x.dot(point)));
      double row_prob = exp(-log1p(kernel));
      if (row_prob > optimization::OPTIMIZATION_ZERO &&
        row_prob < (1 - optimization::OPTIMIZATION_ZERO)) {
        DenseMatrix H = ((row_prob * (1.0 - row_prob)) * x) * x.transpose();
        hessian += H;
      }
    }
  }
  
  /**
   * Compute the first order stats at a random-coordinate (for
   * logistic_regression)
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   *
   * \warning: This implementation is STUPID and is only for testing purposes.
   * Do not copy it over for logistic_regression
   *
   */
  void compute_first_order_stats_per_coordinate(const DenseVector &point, const
      size_t block_size, DenseVector& gradient, DenseVector& hessian_diag,
      size_t& block_start){


    // Code copied from logistic_regression.cpp
    DenseVector full_grad(variables);
    DenseVector full_diag_hessian(variables);
    full_grad.setZero();
    full_diag_hessian.setZero();

    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double kernel = exp(-1.0 * (x.dot(point)));
      double row_prob = exp(-log1p(kernel));
      DenseVector G = x * (row_prob - b(i));
      full_grad += G;
      if (row_prob > optimization::OPTIMIZATION_ZERO &&
        row_prob < (1 - optimization::OPTIMIZATION_ZERO)) {
        full_diag_hessian  += ((row_prob * (1.0 - row_prob))
            * x).cwiseProduct(x);
      }
    }

    gradient = full_grad.segment(coordinate, block_size);
    hessian_diag = full_diag_hessian.segment(coordinate, block_size);

    block_start = coordinate;
    coordinate  = std::min(coordinate + block_size, variables);
    if (coordinate == variables){
      reset(0);
    }
  }

  /**
   * Compute the gradient at a "random" point. (Not tested yet)
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   *
   * \warning: Currently, we do not test SGD in this test case. Will add
   * these ASAP.
   */
  void compute_gradient_per_example(const DenseVector& point, DenseVector&
      gradient){
  }

  /**
   * Compute the gradient at the given point. (for logistic_regression)
   *
   * \param[in]  point    Point at which we are computing the gradient.
   * \param[out] gradient Dense gradient
   * \param[in]  mbStart  Minibatch start index
   * \param[in]  mbSize   Minibatch size (-1 implies all)
   *
   * See logistic_regression.cpp for details
   */
  void compute_gradient(const DenseVector& point, DenseVector& gradient, const
      size_t mbStart=0, const size_t mbSize=-1){
    gradient.setZero();
    // Code copied from logistic_regression.cpp
    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double kernel = exp(-1.0 * (x.dot(point)));
      double row_prob = exp(-log1p(kernel));
      DenseVector G = x * (row_prob - b(i));
      gradient += G;
    }
  }

  /**
   * Compute the function value at the given point. (for logistic_regression)
   *
   * \param[in]  point   Point at which we are computing the gradient.
   * \param[in]  mbStart Minibatch start index
   * \param[in]  mbSize  Minibatch size (-1 implies all)
   * \returns            Function value at "point"
   *
   * \note Code copied from logistic_regression.cpp
   */
  double compute_function_value(const DenseVector& point, const size_t
      mbStart=0, const size_t mbSize =-1){
    double func = 0;
    
    // Code copied from logistic_regression.cpp
    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double margin = x.dot(point);
      double row_func;

      if (margin >= 0.0) {
        row_func = (1.0 - b(i)) * margin + log1p(exp(-margin));
      }
      else {
        row_func = log1p(exp(margin)) - b(i) * margin;
      }
      func += row_func;
    }
    return func;
  }
  
  /**
   * Compute first order statistics at the given point.
   *
   * \param[in]  point   Point at which we are computing the gradient.
   * \param[in,out]  gradient Gradient value
   * \param[in,out]  func_value   Function value.
   * \param[in]  mbStart Minibatch start index
   * \param[in]  mbSize  Minibatch size (-1 implies all)
   * \returns            Function value at "point"
   */
  void compute_first_order_statistics( const DenseVector& point, DenseVector&
      gradient, double& func, const size_t mbStart, const size_t
      mbSize) {
    func = 0;
    gradient.setZero();

    // Code copied from logistic_regression.cpp
    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double margin = x.dot(point);
      double row_func;

      if (margin >= 0.0) {
        row_func = (1.0 - b(i)) * margin + log1p(exp(-margin));
      }
      else {
        row_func = log1p(exp(margin)) - b(i) * margin;
      }
      double kernel = exp(-1.0 * margin);
      double row_prob = exp(-log1p(kernel));
      func += row_func;
      DenseVector G = x * (row_prob - b(i));
      gradient += G;
    }
  }
  
  /**
   * Compute second order statistics at the given point.
   *
   * \param[in]  point        Point at which we are computing the gradient.
   * \param[in,out]  hessian  Hessian value
   * \param[in,out]  gradient Gradient value
   * \param[in,out]  func_value   Function value.
   * \param[in]  mbStart Minisizebatch start index
   * \param[in]  mbSize  Minibatch size (-1 implies all)
   * \returns            Function value at "point"
   */
  void compute_second_order_statistics(const DenseVector& point, DenseMatrix&
      hessian, DenseVector& gradient, double& func) {
    func = 0;
    gradient.setZero();
    hessian.setZero();
    
    // Code copied from logistic_regression.cpp
    for (size_t i=0; i < examples; i++){
      DenseVector x = A.row(i);
      double margin = x.dot(point);
      double row_func;

      if (margin >= 0.0) {
        row_func = (1.0 - b(i)) * margin + log1p(exp(-margin));
      }
      else {
        row_func = log1p(exp(margin)) - b(i) * margin;
      }
      func += row_func;
      double kernel = exp(-1.0 * (margin));
      double row_prob = exp(-log1p(kernel));
      gradient += x * (row_prob - b(i));
      if (row_prob > optimization::OPTIMIZATION_ZERO &&
        row_prob < (1 - optimization::OPTIMIZATION_ZERO)) {
        DenseMatrix H = ((row_prob * (1.0 - row_prob)) * x) * x.transpose();
        hessian += H;
      }
    }
  }


};

/**
 * Solver interface for a sample problem. (Logistic regression)
 *
 * In this test case, we check all the algorithms using logistic regression
 * as a sample problem.
 *
 * Algorithms tested:
 * ------------------
 *
 * (1) LBFGS
 * (2) FISTA
 * (3) GD
 * (4) Newton
 *
 * Utils tested:
 * ------------------
 *
 * (1) Gradient checker
 * (2) Hessian checker
 *
 *
 * Things not tested:
 * ------------------
 *
 * (1) Solvers + Regularizers
 * (2) Solvers + Constraints  
 * (3)
 *
 *
 */

struct optimization_interface_test  {

  size_t examples;
  size_t variables;
  DenseMatrix A;
  DenseVector b;
  DenseVector init_point;
  DenseVector solution;
  std::shared_ptr<opt_interface> solver_interface;
  std::map<std::string, flexible_type> opts;

  public:
    
    optimization_interface_test() {
      size_t examples = 100;
      size_t variables = 10;

      DenseMatrix A(examples, variables);
      DenseVector b(examples);
      DenseVector init_point(variables);

      srand(1);
      A.setRandom();
      b.setRandom();
      init_point.setZero();
      
      std::shared_ptr<opt_interface> solver_interface;
      solver_interface.reset(new opt_interface(A, b));
      std::map<std::string, flexible_type> opts =  {
        {"convergence_threshold", 1e-5},
        {"step_size", 1.0},
        {"lbfgs_memory_level", 3},
        {"mini_batch_size", 1},
        {"max_iterations", 100},
        {"auto_tuning", true},
        {"solver", "newton"},
        {"l1_penalty", 0.0},
        {"l2_penalty", 0.0},
      };

      // Use Newton method to catch the solution
      optimization::solver_return stats;
      stats = turi::optimization::newton_method(*solver_interface,
          init_point, opts);

      this->opts = opts;
      this->A = A;
      this->b = b;
      this->examples = examples;
      this->variables = variables;
      this->solution = stats.solution;
      this->init_point = init_point;
      this->solver_interface = solver_interface;
    }

    /**
    * Tests with no-regularizer.
    * ------------------------------------------------------------------------
    */
    void test_gd(){
      optimization::solver_return stats;
      stats = turi::optimization::gradient_descent(*solver_interface,
          init_point, opts);
      TS_ASSERT(stats.solution.isApprox(solution, 1e-2));
    }
    
    void test_newton(){
      optimization::solver_return stats;
      stats = turi::optimization::newton_method(*solver_interface,
          init_point, opts);

      check_gradient_checker();
      check_hessian_checker();

      TS_ASSERT(std::abs(stats.residual) < 1e-5);
    }

    void test_lbfgs(){
      optimization::solver_return stats;
      stats = turi::optimization::lbfgs_compat(solver_interface,
          init_point, opts);
      TS_ASSERT(stats.solution.isApprox(solution, 1e-2));
    }

    void test_fista(){
      optimization::solver_return stats;
      stats = turi::optimization::accelerated_gradient(*solver_interface,
          init_point, opts);
      TS_ASSERT(stats.solution.isApprox(solution, 1e-2));
    }


    /**
    * Tests optimization utils.
    * ------------------------------------------------------------------------
    */
    
    void check_gradient_checker(){
      for(size_t i=0; i < 10; i++){
        DenseVector point(variables);
        point.setRandom();
        DenseVector gradient(variables);
        solver_interface->compute_gradient(point, gradient);
        TS_ASSERT(check_gradient(*solver_interface, point, gradient));
      }
    }
    
    void check_hessian_checker(){
      for(size_t i=0; i < 10; i++){
        DenseVector point(variables);
        point.setRandom();
        DenseMatrix hessian(variables, variables);
        solver_interface->compute_hessian(point, hessian);
        TS_ASSERT(check_hessian(*solver_interface, point, hessian));
      }
    }

};

BOOST_FIXTURE_TEST_SUITE(_optimization_interface_test, optimization_interface_test)
BOOST_AUTO_TEST_CASE(test_gd) {
  optimization_interface_test::test_gd();
}
BOOST_AUTO_TEST_CASE(test_newton) {
  optimization_interface_test::test_newton();
}
BOOST_AUTO_TEST_CASE(test_lbfgs) {
  optimization_interface_test::test_lbfgs();
}
BOOST_AUTO_TEST_CASE(test_fista) {
  optimization_interface_test::test_fista();
}
BOOST_AUTO_TEST_SUITE_END()
