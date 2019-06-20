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
#include <cmath>

// Eigen
#include <Eigen/Core>
#include <Eigen/SparseCore>

// SFrame and Flex type
#include <model_server/lib/flex_dict_view.hpp>

// ML-Data Utils
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <core/storage/sframe_data/sframe_iterators.hpp>
#include <toolkits/ml_data_2/ml_data_entry.hpp>
#include <toolkits/ml_data_2/metadata.hpp>
#include <toolkits/ml_data_2/sframe_index_mapping.hpp>

// Testing utils common to all of ml_data
#include <core/storage/sframe_data/testing_utils.hpp>
#include <core/util/testing_utils.hpp>

// Testing utils common to all of ml_data
#include <toolkits/ml_data_2/testing_utils.hpp>


typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

using namespace turi;

struct missing_values  {
 public:

  void run_mv_test(sframe X_no_mv, sframe X_mv, bool mv_should_throw_errors) {

    size_t n = X_no_mv.num_rows();

    ASSERT_EQ(X_no_mv.num_rows(), X_mv.num_rows());

    std::vector<std::vector<flexible_type> > y_mv_data(n);
    std::vector<std::vector<flexible_type> > y_no_mv_data(n);

    for(size_t i = 0; i < n; ++i) {
      y_mv_data[i] = {double(i * i) / n};
      y_no_mv_data[i] = {double(i * i) / n};
    }

    y_mv_data[n/2] = {FLEX_UNDEFINED};


    sframe y_mv = make_testing_sframe({"target"}, {flex_type_enum::FLOAT}, y_mv_data);
    sframe y_no_mv = make_testing_sframe({"target"}, {flex_type_enum::FLOAT}, y_no_mv_data);

    ////////////////////////////////////////
    // This throws, as by default things during train time, with a
    // missing value, cause an error.

    std::cerr << "CHECK: On Train, throws with missing values in data, no target." << std::endl;
    {
      v2::ml_data data;

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data.fill(X_mv);
                                  );
      } else {
        data.fill(X_mv);
      }
    }

    //
    std::cerr << "CHECK: On Train, throws with missing values in data, none in target." << std::endl;
    {
      v2::ml_data data;

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data.fill(X_mv, y_no_mv);
                                  );
      } else {
        data.fill(X_mv, y_no_mv);
      }
    }

    std::cerr << "CHECK: On Train, throws with no missing values in data, some in target." << std::endl;
    {
      v2::ml_data data;

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data.fill(X_no_mv, y_mv);
                                  );
      } else {
        data.fill(X_no_mv, y_mv);
      }
    }

    ////////////////////////////////////////////////////////////
    // Imputation

    std::cerr << "CHECK: On predict, imputation works : no target." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "impute"}});
      data.fill(X_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());
      data2.fill(X_mv);
    }

    std::cerr << "CHECK: On predict, imputation works : target." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "impute"}});
      data.fill(X_no_mv, y_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());
      data2.fill(X_mv);
    }


    std::cerr << "CHECK: On predict, imputation works : target with no mv." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "impute"}});
      data.fill(X_no_mv, y_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());
      data2.fill(X_mv, y_no_mv);
    }

    std::cerr << "CHECK: On predict, imputation works : target and data with no mv." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "impute"}});
      data.fill(X_no_mv, y_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());
      data2.fill(X_mv, y_mv);
    }


    std::cerr << "CHECK: On predict, error is thrown on action=error : no target." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "error"}});
      data.fill(X_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data2.fill(X_mv);
                                  );
      } else {
        data2.fill(X_mv);
      }
    }

    std::cerr << "CHECK: On predict, error is thrown on action=error : target." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "error"}});
      data.fill(X_no_mv, y_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data2.fill(X_no_mv, y_mv);
                                  );
      } else {
        data2.fill(X_no_mv, y_mv);
      }
    }


    std::cerr << "CHECK: On predict, error is thrown on action=error : target with mv." << std::endl;
    {
      v2::ml_data data({{"missing_value_action_on_predict", "error"}});
      data.fill(X_no_mv, y_no_mv);

      // Would throw in train mode
      v2::ml_data data2(data.metadata());

      if(mv_should_throw_errors) {
        TS_ASSERT_THROWS_ANYTHING(
            data2.fill(X_mv, y_no_mv);
                                  );
      } else {
        data2.fill(X_mv, y_no_mv);
      }
    }
  }

  void test_numeric() {

    sframe X_no_mv = make_random_sframe(500, "n", false);

    std::vector<std::vector<flexible_type> > data = testing_extract_sframe_data(X_no_mv);

    data.back()[0] = FLEX_UNDEFINED;

    sframe X_mv = make_testing_sframe(
        X_no_mv.column_names(), X_no_mv.column_types(), data);

    run_mv_test(X_no_mv, X_mv, true);
  }

  void test_vector_full() {

    sframe X_no_mv = make_random_sframe(500, "v", false);

    std::vector<std::vector<flexible_type> > data = testing_extract_sframe_data(X_no_mv);

    data.back()[0] = FLEX_UNDEFINED;

    sframe X_mv = make_testing_sframe(
        X_no_mv.column_names(), X_no_mv.column_types(), data);

    run_mv_test(X_no_mv, X_mv, true);
  }

  void test_dictionary_full() {

    sframe X_no_mv = make_random_sframe(500, "d", false);

    std::vector<std::vector<flexible_type> > data = testing_extract_sframe_data(X_no_mv);

    data.back()[0] = FLEX_UNDEFINED;

    sframe X_mv = make_testing_sframe(
        X_no_mv.column_names(), X_no_mv.column_types(), data);

    run_mv_test(X_no_mv, X_mv, true);
  }

  void test_dictionary_value() {

    sframe X_no_mv = make_random_sframe(500, "d", false);

    std::vector<std::vector<flexible_type> > data = testing_extract_sframe_data(X_no_mv);

    flex_dict d = data.back()[0];

    d.back().second = FLEX_UNDEFINED;

    data.back()[0] = d;

    sframe X_mv = make_testing_sframe(
        X_no_mv.column_names(), X_no_mv.column_types(), data);

    run_mv_test(X_no_mv, X_mv, true);
  }
};

BOOST_FIXTURE_TEST_SUITE(_missing_values, missing_values)
BOOST_AUTO_TEST_CASE(test_numeric) {
  missing_values::test_numeric();
}
BOOST_AUTO_TEST_CASE(test_vector_full) {
  missing_values::test_vector_full();
}
BOOST_AUTO_TEST_CASE(test_dictionary_full) {
  missing_values::test_dictionary_full();
}
BOOST_AUTO_TEST_CASE(test_dictionary_value) {
  missing_values::test_dictionary_value();
}
BOOST_AUTO_TEST_SUITE_END()
