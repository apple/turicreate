#include <pch/pch.hpp>

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <util/cityhash_tc.hpp>

#include <numerics/armadillo.hpp>

// ML-Data Utils
#include <unity/toolkits/ml_data_2/standardization-inl.hpp>

// Testing utils common to all of ml_data
#include <unity/toolkits/ml_data_2/testing_utils.hpp>

using namespace turi;

typedef arma::vec DenseVector;
typedef sparse_vector<double, size_t> SparseVector;

typedef arma::mat DenseMatrix;

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
      x.zeros();
      it.fill_row_expr(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x.t();
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(approx_equal(x, ans, "absdiff",  1e-5));

      // Sparse vector
      sp_x.zeros();
      it.fill_row_expr(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(approx_equal(sp1, sp2, "absdiff",  1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(approx_equal(sp_x, sp_ans, "absdiff", 1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < Xmat.n_cols-1; i++){
      TS_ASSERT(std::abs(arma::norm(Xmat.col(i), 2)/std::sqrt(n) - 1) < 3e-1);
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
      x.zeros();
      it.fill_row_expr(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x.t();
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(approx_equal(x, ans, "absdiff",  1e-5));

      // Sparse vector
      sp_x.zeros();
      it.fill_row_expr(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(approx_equal(sp1, sp2, "absdiff",  1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(approx_equal(sp_x, sp_ans, "absdiff",  1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < Xmat.n_cols-1; i++){
      TS_ASSERT(std::abs(arma::norm(Xmat.col(i), 2)/std::sqrt(n) - 1) < 3e-1);
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
      x.zeros();
      it.fill_row_expr(x);
      ans = x;
      scaler->transform(x);
      Xmat.row(it.row_index()) = x.t();
      sp1 = x;
      scaler->inverse_transform(x);
      TS_ASSERT(approx_equal(x, ans, "absdiff",  1e-5));

      // Sparse vector
      sp_x.zeros();
      it.fill_row_expr(sp_x);
      sp_ans = sp_x;
      scaler->transform(sp_x);
      sp2 = sp_x;
      TS_ASSERT(approx_equal(sp1, sp2, "absdiff",  1e-5));
      scaler->inverse_transform(sp_x);
      TS_ASSERT(approx_equal(sp_x, sp_ans, "absdiff",  1e-5));
    }

    // Check that each col has norm 1
    for(size_t i = 0; i < Xmat.n_cols-1; i++){
      TS_ASSERT(std::abs(arma::norm(Xmat.col(i))/std::sqrt(n) - 1) < 3e-1);
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
