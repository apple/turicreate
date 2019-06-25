#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <vector>
#include <algorithm>
#include <core/util/cityhash_tc.hpp>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// Constraints
#include <ml/optimization/constraints-inl.hpp>


using namespace turi;
typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;



struct constraint_interface_test  {

  size_t variables;
  DenseVector solution_orthant;
  DenseVector solution_box;
  double lb_dbl;
  double ub_dbl;
  DenseVector lb;
  DenseVector ub;

  public:
    
    constraint_interface_test() {

      size_t variables = 10;

      DenseVector init_point(variables);
      DenseVector solution_orthant(variables);
      DenseVector solution_box(variables);
      double lb_dbl = 0;
      double up_dbl = 1;
      DenseVector lb(variables);
      DenseVector ub(variables);
      lb.setZeros();
      ub.setOnes():
      srand(1);
      init_point    << 1 << -1 << 2 << -2 << 3 << -3 << 4 << -4 << 5 << -5;
      solution_orthant << 1 <<  0 << 2 <<  0 << 3 <<  0 << 4 <<  0 << 5 <<  0;
      solution_box  << 1 <<  0 << 1 <<  0 << 1 <<  0 << 1 <<  0 << 1 << 0;

      
      std::shared_ptr<non_negative_orthant> non_negative;
      non_negative.reset(new non_negative_orthant(variables));


      this->variables = variables;
      this->init_point = init_point;
      this->solution_box = solution_box;
      this->solution_orthant = solution_orthant;
      this->lb = lb;
      this->ub = lb;
      this->lb_dbl = lb_dbl;
      this->ub_dbl = ub_dbl;
    }


    void test_non_negative(){
      std::shared_ptr<non_negative_orthant> non_negative;
      non_negative.reset(new non_negative_orthant(variables));
      TS_ASSERT(solution_orthant.isApprox(non_negative_orthant.project(init_point),
            1e-10));
      TS_ASSERT_FALSE(non_negative_orthant.is_satisfied(init_point));

      DenseVector test_point(variables);
      test_point.setZeros();
      TS_ASSERT_TRUE(non_negative_orthant.is_satisfied(test_point));

    }

    void test_box(){

      // Vector init
      std::shared_ptr<box_constraints> box;
      box.reset(new box_constraints(lb, ub));
      TS_ASSERT(solution_box.isApprox(box.project(init_point),
            1e-10));
      TS_ASSERT_FALSE(box.is_satisfied(init_point));
      DenseVector test_point(variables);
      test_point.setZeros();
      TS_ASSERT_TRUE(box.is_satisfied(test_point));
      box.reset();

      // Double init
      std::shared_ptr<box_constraints> box;
      box.reset(new box_constraints(lb_dbl, ub_dbl, variables));
      TS_ASSERT(solution_box.isApprox(box.project(init_point),
            1e-10));
      TS_ASSERT_FALSE(box.is_satisfied(init_point));
      DenseVector test_point(variables);
      test_point.setZeros();
      TS_ASSERT_TRUE(box.is_satisfied(test_point));


    }
    
};

BOOST_FIXTURE_TEST_SUITE(_constraint_interface_test, constraint_interface_test)
BOOST_AUTO_TEST_CASE(test_non_negative) {
  constraint_interface_test::test_non_negative();
}
BOOST_AUTO_TEST_CASE(test_box) {
  constraint_interface_test::test_box();
}
BOOST_AUTO_TEST_SUITE_END()
