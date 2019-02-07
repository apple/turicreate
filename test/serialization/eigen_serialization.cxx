/*
* Copyright (C) 2016 Turi
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * Copyright (c) 2009 Carnegie Mellon University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.turicreate.ml.cmu.edu
 *
 */


#include <Eigen/Core>
#include <Eigen/SparseCore>

#include <cxxtest/TestSuite.h>

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

class EigenTestSuite : public CxxTest::TestSuite {
public:


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Array Types


  /********   double   ***********/

  void test_array_simple_double() {
    check_array_save_load<Eigen::Array, double, -1, -1>();
  }

  void test_array_fix_row_double() {
    check_array_save_load<Eigen::Array, double, -1, 8>();
  }

  void test_array_fix_col_double() {
    check_array_save_load<Eigen::Array, double, 8, -1>();
  }

  void test_array_fix_row_col_double() {
    check_array_save_load<Eigen::Array, double, 8, 8>();
  }


  /********   float   ***********/

  void test_array_simple_float() {
    check_array_save_load<Eigen::Array, float, -1, -1>();
  }

  void test_array_fix_row_float() {
    check_array_save_load<Eigen::Array, float, -1, 8>();
  }

  void test_array_fix_col_float() {
    check_array_save_load<Eigen::Array, float, 8, -1>();
  }

  void test_array_fix_row_col_float() {
    check_array_save_load<Eigen::Array, float, 8, 8>();
  }


  /********   int   ***********/

  void test_array_simple_int() {
    check_array_save_load<Eigen::Array, int, -1, -1>();
  }

  void test_array_fix_row_int() {
    check_array_save_load<Eigen::Array, int, -1, 8>();
  }

  void test_array_fix_col_int() {
    check_array_save_load<Eigen::Array, int, 8, -1>();
  }

  void test_array_fix_row_col_int() {
    check_array_save_load<Eigen::Array, int, 8, 8>();
  }


  ////////////////////////////////////////////////////////////////////////////////
  //
  //  Matrix Types


  /********   double   ***********/

  void test_matrix_simple_double() {
    check_array_save_load<Eigen::Matrix, double, -1, -1>();
  }

  void test_matrix_fix_row_double() {
    check_array_save_load<Eigen::Matrix, double, -1, 8>();
  }

  void test_matrix_fix_col_double() {
    check_array_save_load<Eigen::Matrix, double, 8, -1>();
  }

  void test_matrix_fix_row_col_double() {
    check_array_save_load<Eigen::Matrix, double, 8, 8>();
  }


  /********   float   ***********/

  void test_matrix_simple_float() {
    check_array_save_load<Eigen::Matrix, float, -1, -1>();
  }

  void test_matrix_fix_row_float() {
    check_array_save_load<Eigen::Matrix, float, -1, 8>();
  }

  void test_matrix_fix_col_float() {
    check_array_save_load<Eigen::Matrix, float, 8, -1>();
  }

  void test_matrix_fix_row_col_float() {
    check_array_save_load<Eigen::Matrix, float, 8, 8>();
  }


  /********   int   ***********/

  void test_matrix_simple_int() {
    check_array_save_load<Eigen::Matrix, int, -1, -1>();
  }

  void test_matrix_fix_row_int() {
    check_array_save_load<Eigen::Matrix, int, -1, 8>();
  }

  void test_matrix_fix_col_int() {
    check_array_save_load<Eigen::Matrix, int, 8, -1>();
  }

  void test_matrix_fix_row_col_int() {
    check_array_save_load<Eigen::Matrix, int, 8, 8>();
  }



};
