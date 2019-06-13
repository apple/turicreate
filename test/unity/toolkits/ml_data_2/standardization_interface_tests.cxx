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

// ML-Data Utils
#include <toolkits/ml_data_2/standardization-inl.hpp>

// Testing utils common to all of ml_data
#include <toolkits/ml_data_2/testing_utils.hpp>

typedef Eigen::Matrix<double,Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

typedef Eigen::Matrix<double, Eigen::Dynamic,1>  DenseVector;
typedef Eigen::SparseVector<double> SparseVector;

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> DenseMatrix;

using namespace turi;

struct standardization  {

  public:

  /*
   * Test the L2-scaler by generating random points and then projecting them
   * and inverse projecting back to get the same point back.
   */
  void _run_l2_scaling_test(size_t n, const std::string& run_string) {

    sframe X;
    v2::ml_data data;
    std::tie(X, data) = v2::make_random_sframe_and_ml_data(n, run_string);
    TS_ASSERT(n == X.size());
    TS_ASSERT(n == data.size());

    // Take a snapshot of the created metadata
    std::shared_ptr<v2::ml_metadata> metadata = data.metadata();
    size_t total_size;
    std::shared_ptr<l2_rescaling> scaler;
    DenseVector x, ans, sp1, sp2;
    SparseVector sp_x, sp_ans;
    DenseMatrix Xmat;


    scaler.reset(new l2_rescaling(metadata, true));

    // Test for the reference encoding section.
    // ---------------------------------------------------------------------
    total_size = scaler->get_total_size();

    x.resize(total_size);
    sp_x.resize(total_size);
    sp1.resize(total_size);
    sp2.resize(total_size);
    ans.resize(total_size);
    sp_ans.resize(total_size);
    Xmat.resize(n, total_size);

    for(auto it = data.get_iterator(0,1,false,true); !it.done(); ++it){
      // Densevector
      x.setZero();
      it.fill_observation(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x;
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(x.isApprox(ans, 1e-5));

      // Sparse vector
      sp_x.setZero();
      it.fill_observation(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(sp1.isApprox(sp2, 1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(sp_x.isApprox(sp_ans,1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < size_t(Xmat.cols()-1); i++){
      TS_ASSERT(std::abs(Xmat.col(i).norm()/std::sqrt(n) - 1) < 3e-1);
    }

    // Test without reference encoding.
    // ---------------------------------------------------------------------
    scaler.reset(new l2_rescaling(metadata, false));
    total_size = scaler->get_total_size();
    x.resize(total_size);
    sp_x.resize(total_size);
    sp1.resize(total_size);
    sp2.resize(total_size);
    ans.resize(total_size);
    sp_ans.resize(total_size);
    Xmat.resize(n, total_size);

    for(auto it = data.get_iterator(); !it.done(); ++it){
      x.setZero();
      it.fill_observation(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x;
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(x.isApprox(ans, 1e-5));

      // Sparse vector
      sp_x.setZero();
      it.fill_observation(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(sp1.isApprox(sp2, 1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(sp_x.isApprox(sp_ans, 1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < size_t(Xmat.cols()-1); i++){
      TS_ASSERT(std::abs(Xmat.col(i).norm()/std::sqrt(n) - 1) < 3e-1);
    }

    // Save and Load
    dir_archive archive_write;
    archive_write.open_directory_for_write("standardization_tests");
    turi::oarchive oarc(archive_write);
    oarc << *scaler;
    archive_write.close();
    dir_archive archive_read;
    archive_read.open_directory_for_read("standardization_tests");
    turi::iarchive iarc(archive_read);
    iarc >> *scaler;

    // Test after save and load
    // ---------------------------------------------------------------------
    TS_ASSERT(total_size == scaler->get_total_size());
    total_size = scaler->get_total_size();
    x.resize(total_size);
    sp_x.resize(total_size);
    sp1.resize(total_size);
    sp2.resize(total_size);
    ans.resize(total_size);
    sp_ans.resize(total_size);
    Xmat.resize(n, total_size);

    for(auto it = data.get_iterator(); !it.done(); ++it){
      // Densevector
      x.setZero();
      it.fill_observation(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x;
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(x.isApprox(ans, 1e-5));

      // Sparse vector
      sp_x.setZero();
      it.fill_observation(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(sp1.isApprox(sp2, 1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(sp_x.isApprox(sp_ans, 1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < size_t(Xmat.cols()-1); i++){
      TS_ASSERT(std::abs(Xmat.col(i).norm()/std::sqrt(n) - 1) < 3e-1);
    }
  }

  void test_standardization_n() {
    _run_l2_scaling_test(100, "n");
  }

  void test_standardization_v() {
    _run_l2_scaling_test(100, "v");
  }

};

BOOST_FIXTURE_TEST_SUITE(_standardization, standardization)
BOOST_AUTO_TEST_CASE(test_standardization_n) {
  standardization::test_standardization_n();
}
BOOST_AUTO_TEST_CASE(test_standardization_v) {
  standardization::test_standardization_v();
}
BOOST_AUTO_TEST_SUITE_END()
