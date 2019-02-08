#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#
#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <serialization/serialization_includes.hpp>

using namespace turi;
using namespace Eigen;

template <template <typename, int, int, int, int, int> class EigenContainer,
          typename scalar_type, int rows, int cols>
void check_array_save_load() {

  typedef EigenContainer<scalar_type, rows, cols, Eigen::ColMajor, rows, cols> EigenArray;

  std::vector<long> row_list;

  if(rows == Eigen::Dynamic)
    row_list = {0, 1, 23};
  else
    row_list = {rows};

  std::vector<long> col_list;

  if(cols == Eigen::Dynamic)
    col_list = {0, 1, 17};
  else
    col_list = {cols};

  for(size_t _rows : row_list) {
    for(size_t _cols : col_list) {

      EigenArray X;

      X.resize(_rows, _cols);

      X.setRandom();

      {
        // Save it
        dir_archive archive_write;
        archive_write.open_directory_for_write("eigen_serialize_test");

        turi::oarchive oarc(archive_write);

        size_t check = timer::usec_of_day();
        oarc << X << check;

        archive_write.close();

        // Load it
        dir_archive archive_read;
        archive_read.open_directory_for_read("eigen_serialize_test");

        turi::iarchive iarc(archive_read);

        EigenArray X2;
        size_t check_2;

        iarc >> X2 >> check_2;

        TS_ASSERT_EQUALS(X.rows(), X2.rows());
        TS_ASSERT_EQUALS(X.cols(), X2.cols());
        TS_ASSERT_EQUALS(check, check_2);

        for(long r = 0; r < X.rows(); ++r) {
          for(long c = 0; c < X.cols(); ++c) {
            TS_ASSERT_EQUALS(X(r,c), X2(r,c));
          }
        }
      }
    }
  }
}


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Array Types


  /********   double   ***********/

BOOST_AUTO_TEST_CASE(test_array_simple_double) {
    check_array_save_load<Eigen::Array, double, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_double) {
    check_array_save_load<Eigen::Array, double, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_col_double) {
    check_array_save_load<Eigen::Array, double, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_col_double) {
    check_array_save_load<Eigen::Array, double, 8, 8>();
  }


  /********   float   ***********/

BOOST_AUTO_TEST_CASE(test_array_simple_float) {
    check_array_save_load<Eigen::Array, float, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_float) {
    check_array_save_load<Eigen::Array, float, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_col_float) {
    check_array_save_load<Eigen::Array, float, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_col_float) {
    check_array_save_load<Eigen::Array, float, 8, 8>();
  }


  /********   int   ***********/

BOOST_AUTO_TEST_CASE(test_array_simple_int) {
    check_array_save_load<Eigen::Array, int, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_int) {
    check_array_save_load<Eigen::Array, int, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_col_int) {
    check_array_save_load<Eigen::Array, int, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_array_fix_row_col_int) {
    check_array_save_load<Eigen::Array, int, 8, 8>();
  }


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Matrix Types


  /********   double   ***********/

BOOST_AUTO_TEST_CASE(test_matrix_simple_double) {
    check_array_save_load<Eigen::Matrix, double, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_double) {
    check_array_save_load<Eigen::Matrix, double, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_col_double) {
    check_array_save_load<Eigen::Matrix, double, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_col_double) {
    check_array_save_load<Eigen::Matrix, double, 8, 8>();
  }


  /********   float   ***********/

BOOST_AUTO_TEST_CASE(test_matrix_simple_float) {
    check_array_save_load<Eigen::Matrix, float, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_float) {
    check_array_save_load<Eigen::Matrix, float, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_col_float) {
    check_array_save_load<Eigen::Matrix, float, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_col_float) {
    check_array_save_load<Eigen::Matrix, float, 8, 8>();
  }


  /********   int   ***********/

BOOST_AUTO_TEST_CASE(test_matrix_simple_int) {
    check_array_save_load<Eigen::Matrix, int, -1, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_int) {
    check_array_save_load<Eigen::Matrix, int, -1, 8>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_col_int) {
    check_array_save_load<Eigen::Matrix, int, 8, -1>();
  }

BOOST_AUTO_TEST_CASE(test_matrix_fix_row_col_int) {
    check_array_save_load<Eigen::Matrix, int, 8, 8>();
  }

