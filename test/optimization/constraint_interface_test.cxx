#include <pch/pch.hpp>

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <util/cityhash_tc.hpp>


// Constraints
#include <optimization/constraints-inl.hpp>
#include <numerics/armadillo.hpp>

using namespace turi;
typedef arma::vec  DenseVector;
typedef sparse_vector<double> SparseVector;



struct constraint_interface_test  {

  size_t variables;
  DenseVector init_point;
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
      double ub_dbl = 1;
      DenseVector lb(variables);
      DenseVector ub(variables);
      lb.zeros(variables);
      ub.ones(variables);
      srand(1);
      init_point = {1 , -1 , 2 , -2 , 3 , -3 , 4 , -4 , 5 , -5};
      solution_orthant = {1 ,  0 , 2 ,  0 , 3 ,  0 , 4 ,  0 , 5 ,  0};
      solution_box = {1 ,  0 , 1 ,  0 , 1 ,  0 , 1 ,  0 , 1 , 0};


      std::shared_ptr<optimization::non_negative_orthant> non_negative;
      non_negative.reset(new optimization::non_negative_orthant(variables));


      this->variables = variables;
      this->init_point = init_point;
      this->solution_box = solution_box;
      this->solution_orthant = solution_orthant;
      this->lb = lb;
      this->ub = ub;
      this->lb_dbl = lb_dbl;
      this->ub_dbl = ub_dbl;
    }

    void test_non_negative(){
      std::shared_ptr<optimization::non_negative_orthant> non_negative;
      non_negative.reset(new optimization::non_negative_orthant(variables));
      DenseVector projected_point(variables);
     
      // Projection
      projected_point = init_point;
      non_negative->project(projected_point);
      TS_ASSERT(arma::approx_equal(solution_orthant, projected_point,"absdiff", 1e-10));
      
      // Not satisfied
      TS_ASSERT(not(non_negative->is_satisfied(init_point)));

      // Satisfied
      DenseVector test_point(variables);
      test_point.zeros(variables);
      TS_ASSERT(non_negative->is_satisfied(test_point));

    }

    void test_box(){

      // Vector init
      // -------------------------------------------------------
      std::shared_ptr<optimization::box_constraints> box;
      box.reset(new optimization::box_constraints(lb, ub));
      DenseVector projected_point(variables);
      
      // Projection
      projected_point = init_point;
      box->project(projected_point);
      TS_ASSERT(arma::approx_equal(solution_box, projected_point,"absdiff", 1e-10));
      
      // Not satisfied
      TS_ASSERT(not(box->is_satisfied(init_point)));
      DenseVector test_point(variables);
      test_point.zeros(variables);

      // Satisfied
      TS_ASSERT(box->is_satisfied(test_point));

      // Double init
      // -------------------------------------------------------
      box.reset(new optimization::box_constraints(lb_dbl, ub_dbl, variables));
      
      // Projection
      projected_point = init_point;
      box->project(projected_point);
      TS_ASSERT(arma::approx_equal(solution_box, projected_point,"absdiff", 1e-10));
      
      // Not satisfied
      TS_ASSERT(not(box->is_satisfied(init_point)));
      test_point.zeros(variables);

      // Satisfied
      TS_ASSERT(box->is_satisfied(test_point));


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
