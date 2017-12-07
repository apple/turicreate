// Copyright 2011-2017 Ryan Curtin (http://www.ratml.org/)
// Copyright 2017 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------

#include <numerics/armadillo.hpp>

#include "catch.hpp"

using namespace arma;

TEST_CASE("sp_subview_tests")
  {
  Mat<double> ref(4,4);
  ref.eye(4,4);

  SpMat<double> X(4,4);
  X.eye(4,4);

  /*
   * [[1,0,0,0]     [[2,0,0,0]
   *  [0,1,0,0]  ->  [0,2,0,0]
   *  [0,0,1,0]      [0,0,2,0]
   *  [0,0,0,1]]     [0,0,0,1]]
   */
  ref.submat(0, 0, 2, 2) *= 2;
  X.submat(0, 0, 2, 2) *= 2;

  for (uword i = 0; i < 4; i++)
    {
    for (uword j = 0; j < 4; j++)
      {
      REQUIRE( (double) ref(i, j) == Approx((double) X(i, j)) );
      }
    }

  /*
   * [[2,0,0,0]     [[2,0,0,0]
   *  [0,2,0,0]  ->  [0,1,0,0]
   *  [0,0,2,0]      [0,0,1,0]
   *  [0,0,0,1]]     [0,0,0,.5]]
   */
  ref.submat(1, 1, 3, 3) /= 2;
  X.submat(1, 1, 3, 3) /= 2;

  for (uword i = 0; i < 4; i++)
    {
    for (uword j = 0; j < 4; j++)
      {
      REQUIRE( (double) ref(i, j) == Approx((double) X(i, j)) );
      }
    }

  span s(1, 2);
  ref.submat(s, s) += 10;
  X.submat(s, s) += 10;

  for (uword i = 0; i < 4; i++)
    {
    for (uword j = 0; j < 4; j++)
      {
      REQUIRE( (double) ref(i, j) == Approx((double) X(i, j)) );
      }
    }
  }



TEST_CASE("sp_subview_const_test")
  {
  Mat<double> ref(4, 4);
  ref.eye(4, 4);

  SpMat<double> X(4, 4);
  X.eye(4, 4);

  const SpSubview<double> subX = X.submat(span(0, 2), span::all);
/*
  X.print("x");
  for (size_t i = 0; i < 3; ++i)
  {
    for (size_t j = 0; j < 4; ++j)
    {
      printf("%f ", subX(i, j));
    }
    printf("\n");
  }
*/
  }



TEST_CASE("sp_subview_multiplication_test")
  {
  // Ensure matrix multiplication with subviews works correctly.
  SpMat<double> a(2, 5);
  SpMat<double> b(5, 2);

  a(1, 3) = 1;
  a(0, 0) = 2;
  a(1, 2) = 1.5;

  b(4, 1) = 3;
  b(3, 0) = 2;
  b(1, 0) = 1;
  b(0, 0) = 0.6;

  b *= a;

  REQUIRE( b.n_cols == 5 );
  REQUIRE( b.n_rows == 5 );
  REQUIRE( b.n_elem == 25 );
  REQUIRE( b.n_nonzero == 5 );

  REQUIRE( (double) b(0, 0) == Approx(1.2) );
  REQUIRE( (double) b(0, 1) == Approx(1e-5) );
  REQUIRE( (double) b(0, 2) == Approx(1e-5) );
  REQUIRE( (double) b(0, 3) == Approx(1e-5) );
  REQUIRE( (double) b(0, 4) == Approx(1e-5) );
  REQUIRE( (double) b(1, 0) == Approx(2.0) );
  REQUIRE( (double) b(1, 1) == Approx(1e-5) );
  REQUIRE( (double) b(1, 2) == Approx(1e-5) );
  REQUIRE( (double) b(1, 3) == Approx(1e-5) );
  REQUIRE( (double) b(1, 4) == Approx(1e-5) );
  REQUIRE( (double) b(2, 0) == Approx(1e-5) );
  REQUIRE( (double) b(2, 1) == Approx(1e-5) );
  REQUIRE( (double) b(2, 2) == Approx(1e-5) );
  REQUIRE( (double) b(2, 3) == Approx(1e-5) );
  REQUIRE( (double) b(2, 4) == Approx(1e-5) );
  REQUIRE( (double) b(3, 0) == Approx(4.0) );
  REQUIRE( (double) b(3, 1) == Approx(1e-5) );
  REQUIRE( (double) b(3, 2) == Approx(1e-5) );
  REQUIRE( (double) b(3, 3) == Approx(1e-5) );
  REQUIRE( (double) b(3, 4) == Approx(1e-5) );
  REQUIRE( (double) b(4, 0) == Approx(1e-5) );
  REQUIRE( (double) b(4, 1) == Approx(1e-5) );
  REQUIRE( (double) b(4, 2) == Approx(4.5) );
  REQUIRE( (double) b(4, 3) == Approx(3.0) );
  REQUIRE( (double) b(4, 4) == Approx(1e-5) );
  }



TEST_CASE("sp_subview_multiplication_test_2")
  {
  // Ensure matrix multiplication with subviews works correctly.
  SpMat<double> a(4, 5);
  SpMat<double> b(5, 2);

  a(2, 3) = 1;
  a(3, 1) = 1.4;
  a(0, 0) = 2;
  a(1, 0) = 2;
  a(2, 2) = 1.5;

  b(4, 1) = 3;
  b(3, 0) = 2;
  b(1, 0) = 1;
  b(0, 0) = 0.6;

  b *= a.rows(1, 2);

  REQUIRE( b.n_cols == 5 );
  REQUIRE( b.n_rows == 5 );
  REQUIRE( b.n_elem == 25 );
  REQUIRE( b.n_nonzero == 5 );

  REQUIRE( (double) b(0, 0) == Approx(1.2) );
  REQUIRE( (double) b(0, 1) == Approx(1e-5) );
  REQUIRE( (double) b(0, 2) == Approx(1e-5) );
  REQUIRE( (double) b(0, 3) == Approx(1e-5) );
  REQUIRE( (double) b(0, 4) == Approx(1e-5) );
  REQUIRE( (double) b(1, 0) == Approx(2.0) );
  REQUIRE( (double) b(1, 1) == Approx(1e-5) );
  REQUIRE( (double) b(1, 2) == Approx(1e-5) );
  REQUIRE( (double) b(1, 3) == Approx(1e-5) );
  REQUIRE( (double) b(1, 4) == Approx(1e-5) );
  REQUIRE( (double) b(2, 0) == Approx(1e-5) );
  REQUIRE( (double) b(2, 1) == Approx(1e-5) );
  REQUIRE( (double) b(2, 2) == Approx(1e-5) );
  REQUIRE( (double) b(2, 3) == Approx(1e-5) );
  REQUIRE( (double) b(2, 4) == Approx(1e-5) );
  REQUIRE( (double) b(3, 0) == Approx(4.0) );
  REQUIRE( (double) b(3, 1) == Approx(1e-5) );
  REQUIRE( (double) b(3, 2) == Approx(1e-5) );
  REQUIRE( (double) b(3, 3) == Approx(1e-5) );
  REQUIRE( (double) b(3, 4) == Approx(1e-5) );
  REQUIRE( (double) b(4, 0) == Approx(1e-5) );
  REQUIRE( (double) b(4, 1) == Approx(1e-5) );
  REQUIRE( (double) b(4, 2) == Approx(4.5) );
  REQUIRE( (double) b(4, 3) == Approx(3.0) );
  REQUIRE( (double) b(4, 4) == Approx(1e-5) );
  }



TEST_CASE("sp_subview_unary_operators_test")
  {
  SpMat<int> a(3, 3);
  SpMat<int> b(5, 5);

  a(0, 0) = 1;
  a(1, 2) = 4;
  a(2, 2) = 5;

  b(2, 3) = 1;
  b(3, 2) = 2;
  b(3, 4) = -4;
  b(4, 4) = 5;

  SpMat<int> c = a + b.submat(2, 2, 4, 4);

  REQUIRE( c.n_nonzero == 4 );

  REQUIRE( (double) c(0, 0) == 1 );
  REQUIRE( (double) c(1, 0) == 2 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 1 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == 0 );
  REQUIRE( (double) c(2, 2) == 10 );

  c = a - b.submat(2, 2, 4, 4);

  REQUIRE( c.n_nonzero == 4 );

  REQUIRE( (double) c(0, 0) == 1 );
  REQUIRE( (double) c(1, 0) == -2 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == -1 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == 8 );
  REQUIRE( (double) c(2, 2) == 0 );

  c = a % b.submat(2, 2, 4, 4);

  REQUIRE( c.n_nonzero == 2 );

  REQUIRE( (double) c(0, 0) == 0 );
  REQUIRE( (double) c(1, 0) == 0 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 0 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == -16 );
  REQUIRE( (double) c(2, 2) == 25 );

  a(0, 0) = 4;
  b(2, 2) = 2;
/*
  c = a / b.submat(2, 2, 4, 4);

  REQUIRE( c.n_nonzero == 3 );

  REQUIRE( (double) c(0, 0) == 2 );
  REQUIRE( (double) c(1, 0) == 0 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 0 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == -1 );
  REQUIRE( (double) c(2, 2) == 1 );
*/
  }


TEST_CASE("sp_subview_mat_operator_tests")
  {
  SpMat<double> a(6, 10);
  a(2, 2) = 2.0;
  a(3, 4) = 3.5;
  a(4, 3) = -2.0;
  a(4, 4) = 4.5;
  a(5, 1) = 3.2;
  a(0, 1) = 1.3;
  a(1, 1) = -4.0;
  a(5, 3) = 5.3;

  mat b(3, 3);
  b.fill(2.0);

  mat c(b);

  c += a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.5) );
  REQUIRE( (double) c(2, 2) == Approx(6.5) );

  c = b + a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.5) );
  REQUIRE( (double) c(2, 2) == Approx(6.5) );

  c = b;
  c -= a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(1e-5) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(4.0) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(-1.5) );
  REQUIRE( (double) c(2, 2) == Approx(-2.5) );

  c = b - a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(1e-5) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(4.0) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(-1.5) );
  REQUIRE( (double) c(2, 2) == Approx(-2.5) );

  c = b;
  c *= a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 0) == Approx(4.0) );
  REQUIRE( (double) c(0, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 1) == Approx(-4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(0, 2) == Approx(16.0) );
  REQUIRE( (double) c(1, 2) == Approx(16.0) );
  REQUIRE( (double) c(2, 2) == Approx(16.0) );

  mat e = b * a.submat(2, 2, 4, 4);

  REQUIRE( (double) e(0, 0) == Approx(4.0) );
  REQUIRE( (double) e(1, 0) == Approx(4.0) );
  REQUIRE( (double) e(2, 0) == Approx(4.0) );
  REQUIRE( (double) e(0, 1) == Approx(-4.0) );
  REQUIRE( (double) e(1, 1) == Approx(-4.0) );
  REQUIRE( (double) e(2, 1) == Approx(-4.0) );
  REQUIRE( (double) e(0, 2) == Approx(16.0) );
  REQUIRE( (double) e(1, 2) == Approx(16.0) );
  REQUIRE( (double) e(2, 2) == Approx(16.0) );

  c = b;
  c %= a.submat(2, 2, 4, 4);

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(1e-5) );
  REQUIRE( (double) c(2, 0) == Approx(1e-5) );
  REQUIRE( (double) c(0, 1) == Approx(1e-5) );
  REQUIRE( (double) c(1, 1) == Approx(1e-5) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(0, 2) == Approx(1e-5) );
  REQUIRE( (double) c(1, 2) == Approx(7.0) );
  REQUIRE( (double) c(2, 2) == Approx(9.0) );

  SpMat<double> d = b % a.submat(2, 2, 4, 4);

  REQUIRE( d.n_nonzero == 4 );
  REQUIRE( (double) d(0, 0) == Approx(4.0) );
  REQUIRE( (double) d(2, 1) == Approx(-4.0) );
  REQUIRE( (double) d(1, 2) == Approx(7.0) );
  REQUIRE( (double) d(2, 2) == Approx(9.0) );

  c = b;
  c /= a.submat(2, 2, 4, 4);

  REQUIRE( c(0, 0) == Approx(1.0) );
  REQUIRE( std::isinf(c(1, 0)) );
  REQUIRE( std::isinf(c(2, 0)) );
  REQUIRE( std::isinf(c(0, 1)) );
  REQUIRE( std::isinf(c(1, 1)) );
  REQUIRE( c(2, 1) == Approx(-1.0) );
  REQUIRE( std::isinf(c(0, 2)) );
  REQUIRE( c(1, 2) == Approx(2.0 / 3.5) );
  REQUIRE( c(2, 2) == Approx(2.0 / 4.5) );
  }



TEST_CASE("sp_subview_base_test")
  {
  SpMat<double> a(6, 10);
  a(2, 2) = 2.0;
  a(3, 4) = 3.5;
  a(4, 3) = -2.0;
  a(4, 4) = 4.5;
  a(5, 1) = 3.2;
  a(0, 1) = 1.3;
  a(1, 1) = -4.0;
  a(5, 3) = 5.3;

  mat b(3, 3);
  b.fill(2.0);

  SpMat<double> c = a;
  c.submat(2, 2, 4, 4) = b;

  REQUIRE( c.n_nonzero == 13 );
  REQUIRE( (double) c(2, 2) == Approx(2.0) );
  REQUIRE( (double) c(3, 2) == Approx(2.0) );
  REQUIRE( (double) c(4, 2) == Approx(2.0) );
  REQUIRE( (double) c(2, 3) == Approx(2.0) );
  REQUIRE( (double) c(3, 3) == Approx(2.0) );
  REQUIRE( (double) c(4, 3) == Approx(2.0) );
  REQUIRE( (double) c(2, 4) == Approx(2.0) );
  REQUIRE( (double) c(3, 4) == Approx(2.0) );
  REQUIRE( (double) c(4, 4) == Approx(2.0) );

  c = a;
  c.submat(2, 2, 4, 4) += b;

  REQUIRE( c.n_nonzero == 12 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 2) == Approx(2.0) );
  REQUIRE( (double) c(4, 2) == Approx(2.0) );
  REQUIRE( (double) c(2, 3) == Approx(2.0) );
  REQUIRE( (double) c(3, 3) == Approx(2.0) );
  REQUIRE( (double) c(4, 3) == Approx(1e-5) );
  REQUIRE( (double) c(2, 4) == Approx(2.0) );
  REQUIRE( (double) c(3, 4) == Approx(5.5) );
  REQUIRE( (double) c(4, 4) == Approx(6.5) );

  Mat<double> d = a.submat(2, 2, 4, 4) + b;
  c = a.submat(2, 2, 4, 4) + b;

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.5) );
  REQUIRE( (double) c(2, 2) == Approx(6.5) );

  c = a;
  c.submat(2, 2, 4, 4) -= b;

  REQUIRE( c.n_nonzero == 12 );
  REQUIRE( (double) c(2, 2) == Approx(1e-5) );
  REQUIRE( (double) c(3, 2) == Approx(-2.0) );
  REQUIRE( (double) c(4, 2) == Approx(-2.0) );
  REQUIRE( (double) c(2, 3) == Approx(-2.0) );
  REQUIRE( (double) c(3, 3) == Approx(-2.0) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(2, 4) == Approx(-2.0) );
  REQUIRE( (double) c(3, 4) == Approx(1.5) );
  REQUIRE( (double) c(4, 4) == Approx(2.5) );

  c = a.submat(2, 2, 4, 4) - b;

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(0, 0) == Approx(1e-5) );
  REQUIRE( (double) c(1, 0) == Approx(-2.0) );
  REQUIRE( (double) c(2, 0) == Approx(-2.0) );
  REQUIRE( (double) c(0, 1) == Approx(-2.0) );
  REQUIRE( (double) c(1, 1) == Approx(-2.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(0, 2) == Approx(-2.0) );
  REQUIRE( (double) c(1, 2) == Approx(1.5) );
  REQUIRE( (double) c(2, 2) == Approx(2.5) );

  c = a;
  c.submat(2, 2, 4, 4) *= b;

  REQUIRE( c.n_nonzero == 13 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 2) == Approx(7.0) );
  REQUIRE( (double) c(4, 2) == Approx(5.0) );
  REQUIRE( (double) c(2, 3) == Approx(4.0) );
  REQUIRE( (double) c(3, 3) == Approx(7.0) );
  REQUIRE( (double) c(4, 3) == Approx(5.0) );
  REQUIRE( (double) c(2, 4) == Approx(4.0) );
  REQUIRE( (double) c(3, 4) == Approx(7.0) );
  REQUIRE( (double) c(4, 4) == Approx(5.0) );

  c = a.submat(2, 2, 4, 4) * b;

  REQUIRE( c.n_nonzero == 9 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(7.0) );
  REQUIRE( (double) c(2, 0) == Approx(5.0) );
  REQUIRE( (double) c(0, 1) == Approx(4.0) );
  REQUIRE( (double) c(1, 1) == Approx(7.0) );
  REQUIRE( (double) c(2, 1) == Approx(5.0) );
  REQUIRE( (double) c(0, 2) == Approx(4.0) );
  REQUIRE( (double) c(1, 2) == Approx(7.0) );
  REQUIRE( (double) c(2, 2) == Approx(5.0) );

  c = a.submat(2, 2, 4, 4) % b;

  REQUIRE( c.n_nonzero == 4 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(7.0) );
  REQUIRE( (double) c(2, 2) == Approx(9.0) );

  c = a;
  c.submat(2, 2, 4, 4) %= b;

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(3, 4) == Approx(7.0) );
  REQUIRE( (double) c(4, 4) == Approx(9.0) );

  c = a.submat(2, 2, 4, 4) / b;

  REQUIRE( c.n_nonzero == 4 );
  REQUIRE( (double) c(0, 0) == Approx(1.0) );
  REQUIRE( (double) c(2, 1) == Approx(-1.0) );
  REQUIRE( (double) c(1, 2) == Approx(3.5 / 2.0) );
  REQUIRE( (double) c(2, 2) == Approx(4.5 / 2.0) );

  c = a;
  c.submat(2, 2, 4, 4) /= b;

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(2, 2) == Approx(1.0) );
  REQUIRE( (double) c(4, 3) == Approx(-1.0) );
  REQUIRE( (double) c(3, 4) == Approx(3.5 / 2.0) );
  REQUIRE( (double) c(4, 4) == Approx(4.5 / 2.0) );
  }



TEST_CASE("sp_subview_sp_mat_test")
  {
  SpMat<double> a(6, 10);
  a(2, 2) = 2.0;
  a(3, 4) = 3.5;
  a(4, 3) = -2.0;
  a(4, 4) = 4.5;
  a(5, 1) = 3.2;
  a(0, 1) = 1.3;
  a(1, 1) = -4.0;
  a(5, 3) = 5.3;

  SpMat<double> b(3, 3);
  b(0, 0) = 2.0;
  b(1, 2) = 1.5;
  b(2, 1) = 2.0;

  SpMat<double> c = a;
  c.submat(2, 2, 4, 4) = b;

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(2.0) );
  REQUIRE( (double) c(3, 4) == Approx(1.5) );
  REQUIRE( (double) c(4, 3) == Approx(2.0) );

  c = a;
  c.submat(2, 2, 4, 4) += b;

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 4) == Approx(5.0) );
  REQUIRE( (double) c(4, 4) == Approx(4.5) );

  c = a.submat(2, 2, 4, 4) + b;

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.0) );
  REQUIRE( (double) c(2, 2) == Approx(4.5) );

  c = a;
  c.submat(2, 2, 4, 4) -= b;

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(1e-5) );
  REQUIRE( (double) c(3, 2) == Approx(1e-5) );
  REQUIRE( (double) c(4, 2) == Approx(1e-5) );
  REQUIRE( (double) c(2, 3) == Approx(1e-5) );
  REQUIRE( (double) c(3, 3) == Approx(1e-5) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(2, 4) == Approx(1e-5) );
  REQUIRE( (double) c(3, 4) == Approx(2.0) );
  REQUIRE( (double) c(4, 4) == Approx(4.5) );

  c = a.submat(2, 2, 4, 4) - b;

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(2.0) );
  REQUIRE( (double) c(2, 2) == Approx(4.5) );

  c = a;
  c.submat(2, 2, 4, 4) *= b;

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 3) == Approx(7.0) );
  REQUIRE( (double) c(4, 3) == Approx(9.0) );
  REQUIRE( (double) c(4, 4) == Approx(-3.0) );

  c = a.submat(2, 2, 4, 4) * b;

  REQUIRE( c.n_nonzero == 4 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 1) == Approx(7.0) );
  REQUIRE( (double) c(2, 1) == Approx(9.0) );
  REQUIRE( (double) c(2, 2) == Approx(-3.0) );
  c = a.submat(2, 2, 4, 4) % b;

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.25) );
  REQUIRE( (double) c(2, 2) == Approx(1e-5) );

  c = a;
  c.submat(2, 2, 4, 4) %= b;

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(3, 4) == Approx(5.25) );
  REQUIRE( (double) c(4, 4) == Approx(1e-5) );

//  c = a.submat(2, 2, 4, 4) / b;

//  REQUIRE( c.n_nonzero == 9 );
//  REQUIRE( (double) c(0, 0) == Approx(1.0) );
//  REQUIRE( (double) c(1, 0) != (double) c(1, 0) );
//  REQUIRE( (double) c(2, 0) != (double) c(2, 0) );
//  REQUIRE( (double) c(0, 1) != (double) c(0, 1) );
//  REQUIRE( (double) c(1, 1) != (double) c(1, 1) );
//  REQUIRE( (double) c(2, 1) == Approx(-1.0) );
//  REQUIRE( (double) c(0, 2) != (double) c(0, 2) );
//  REQUIRE( (double) c(1, 2) == Approx((3.5 / 1.5)) );
//  REQUIRE( std::isinf((double) c(2, 2)) );

  c = a;
  c.submat(2, 2, 4, 4) /= b;

  REQUIRE( c.n_nonzero == 13 );
  REQUIRE( (double) c(2, 2) == Approx(1.0) );
  REQUIRE( (double) c(3, 2) != (double) c(3, 2));
  REQUIRE( (double) c(4, 2) != (double) c(4, 2));
  REQUIRE( (double) c(2, 3) != (double) c(2, 3));
  REQUIRE( (double) c(3, 3) != (double) c(3, 3));
  REQUIRE( (double) c(4, 3) == Approx(-1.0) );
  REQUIRE( (double) c(2, 4) != (double) c(2, 4) );
  REQUIRE( (double) c(3, 4) == Approx((3.5 / 1.5)) );
  REQUIRE( std::isinf((double) c(4, 4)) );
  }



TEST_CASE("sp_subview_sp_subview_tests")
  {
  SpMat<double> a(6, 10);
  a(2, 2) = 2.0;
  a(3, 4) = 3.5;
  a(4, 3) = -2.0;
  a(4, 4) = 4.5;
  a(5, 1) = 3.2;
  a(0, 1) = 1.3;
  a(1, 1) = -4.0;
  a(5, 3) = 5.3;

  SpMat<double> b(5, 5);
  b(0, 0) = 1.0;
  b(0, 1) = 1.0;
  b(0, 2) = 1.0;
  b(0, 3) = 1.0;
  b(0, 4) = 1.0;
  b(1, 0) = 1.0;
  b(2, 0) = 1.0;
  b(3, 0) = 1.0;
  b(4, 0) = 1.0;
  b(4, 1) = 1.0;
  b(4, 2) = 1.0;
  b(4, 3) = 1.0;
  b(4, 4) = 1.0;
  b(3, 4) = 1.0;
  b(2, 4) = 1.0;
  b(1, 4) = 1.0;
  b(1, 1) = 2.0;
  b(2, 3) = 1.5;
  b(3, 2) = 2.0;

  SpMat<double> c = a;
  c.submat(2, 2, 4, 4) = b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(2.0) );
  REQUIRE( (double) c(3, 4) == Approx(1.5) );
  REQUIRE( (double) c(4, 3) == Approx(2.0) );

  c = a;
  c.submat(2, 2, 4, 4) += b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 4) == Approx(5.0) );
  REQUIRE( (double) c(4, 4) == Approx(4.5) );

  c = a.submat(2, 2, 4, 4) + b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.0) );
  REQUIRE( (double) c(2, 2) == Approx(4.5) );

  c = a;
  c.submat(2, 2, 4, 4) -= b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(1e-5) );
  REQUIRE( (double) c(3, 2) == Approx(1e-5) );
  REQUIRE( (double) c(4, 2) == Approx(1e-5) );
  REQUIRE( (double) c(2, 3) == Approx(1e-5) );
  REQUIRE( (double) c(3, 3) == Approx(1e-5) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(2, 4) == Approx(1e-5) );
  REQUIRE( (double) c(3, 4) == Approx(2.0) );
  REQUIRE( (double) c(4, 4) == Approx(4.5) );

  c = a.submat(2, 2, 4, 4) - b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(2.0) );
  REQUIRE( (double) c(2, 2) == Approx(4.5) );

  c = a;
  c.submat(2, 2, 4, 4) *= b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 8 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(3, 3) == Approx(7.0) );
  REQUIRE( (double) c(4, 3) == Approx(9.0) );
  REQUIRE( (double) c(4, 4) == Approx(-3.0) );
  c = a.submat(2, 2, 4, 4) * b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 4 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 1) == Approx(7.0) );
  REQUIRE( (double) c(2, 1) == Approx(9.0) );
  REQUIRE( (double) c(2, 2) == Approx(-3.0) );

  c = a.submat(2, 2, 4, 4) % b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 3 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.25) );
  REQUIRE( (double) c(2, 2) == Approx(1e-5) );

  c = a;
  c.submat(2, 2, 4, 4) %= b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 7 );
  REQUIRE( (double) c(2, 2) == Approx(4.0) );
  REQUIRE( (double) c(4, 3) == Approx(-4.0) );
  REQUIRE( (double) c(3, 4) == Approx(5.25) );
  REQUIRE( (double) c(4, 4) == Approx(1e-5) );

//  c = a.submat(2, 2, 4, 4) / b.submat(1, 1, 3, 3);

//  REQUIRE( c.n_nonzero == 9 );
//  REQUIRE( (double) c(0, 0) == Approx(1.0) );
//  REQUIRE( (double) c(1, 0) != (double) c(1, 0) );
//  REQUIRE( (double) c(2, 0) != (double) c(2, 0) );
//  REQUIRE( (double) c(0, 1) != (double) c(0, 1) );
//  REQUIRE( (double) c(1, 1) != (double) c(1, 1) );
//  REQUIRE( (double) c(2, 1) == Approx(-1.0) );
//  REQUIRE( (double) c(0, 2) != (double) c(0, 2) );
//  REQUIRE( (double) c(1, 2) == Approx((3.5 / 1.5)) );
//  REQUIRE( std::isinf((double) c(2, 2)) );

  c = a;
  c.submat(2, 2, 4, 4) /= b.submat(1, 1, 3, 3);

  REQUIRE( c.n_nonzero == 13 );
  REQUIRE( (double) c(2, 2) == Approx(1.0) );
  REQUIRE( (double) c(3, 2) != (double) c(3, 2) );
  REQUIRE( (double) c(4, 2) != (double) c(4, 2) );
  REQUIRE( (double) c(2, 3) != (double) c(2, 3) );
  REQUIRE( (double) c(3, 3) != (double) c(3, 3) );
  REQUIRE( (double) c(4, 3) == Approx(-1.0) );
  REQUIRE( (double) c(2, 4) != (double) c(2, 4) );
  REQUIRE( (double) c(3, 4) == Approx((3.5 / 1.5)) );
  REQUIRE( std::isinf((double) c(4, 4)) );
  }



TEST_CASE("sp_subview_iterators_test")
  {
  SpMat<double> b(5, 5);
  b(0, 0) = 1.0;
  b(0, 1) = 1.0;
  b(0, 2) = 1.0;
  b(0, 3) = 1.0;
  b(0, 4) = 1.0;
  b(1, 0) = 1.0;
  b(2, 0) = 1.0;
  b(3, 0) = 1.0;
  b(4, 0) = 1.0;
  b(4, 1) = 1.0;
  b(4, 2) = 1.0;
  b(4, 3) = 1.0;
  b(4, 4) = 1.0;
  b(3, 4) = 1.0;
  b(2, 4) = 1.0;
  b(1, 4) = 1.0;
  b(1, 1) = 2.0;
  b(2, 3) = 1.5;
  b(3, 2) = 2.0;

  // [[1.0 1.0 1.0 1.0 1.0]
  //  [1.0 2.0 0.0 0.0 1.0]
  //  [1.0 0.0 0.0 1.5 1.0]
  //  [1.0 0.0 2.0 0.0 1.0]
  //  [1.0 1.0 1.0 1.0 1.0]]
  SpSubview<double> s = b.submat(1, 1, 3, 3);

  SpSubview<double>::iterator it = s.begin();

  REQUIRE( it.pos() == 0 );
  REQUIRE( it.skip_pos == 6 );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  ++it;

  REQUIRE( it.pos() == 1 );
  REQUIRE( it.skip_pos == 8 );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 1 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  ++it;

  REQUIRE( it.pos() == 2 );
  REQUIRE( it.skip_pos == 10 );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  REQUIRE( (double) (*it) == Approx(1.5) );

  *it = 4.3;

  REQUIRE( (double) (*it) == Approx(4.3) );

  ++it;

  REQUIRE( it.pos() == s.n_nonzero );

  --it;

  REQUIRE( it.pos() == 2 );
  REQUIRE( it.skip_pos == 10 );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  REQUIRE( (double) (*it) == Approx(4.3) );

  --it;

  REQUIRE( it.pos() == 1 );
  REQUIRE( it.skip_pos == 8 );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 1 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  --it;

  REQUIRE( it.pos() == 0 );
  REQUIRE( it.skip_pos == 6 );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  SpMat<double> c(5, 5);
  c(1, 1) = 2.0;
  c(2, 3) = 1.5;
  c(3, 2) = 2.0;

  SpSubview<double> ss = c.submat(1, 1, 3, 3);

  SpSubview<double>::iterator sit = ss.begin();

  REQUIRE( sit.pos() == 0 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 0 );
  REQUIRE( sit.col() == 0 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  ++sit;

  REQUIRE( sit.pos() == 1 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 2 );
  REQUIRE( sit.col() == 1 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  ++sit;

  REQUIRE( sit.pos() == 2 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 1 );
  REQUIRE( sit.col() == 2 );
  REQUIRE( (double) (*sit) == Approx(1.5) );

  *sit = 4.2;

  REQUIRE( (double) (*sit) == Approx(4.2) );

  ++sit;

  REQUIRE( sit.pos() == ss.n_nonzero );

  --sit;

  REQUIRE( sit.pos() == 2 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 1 );
  REQUIRE( sit.col() == 2 );
  REQUIRE( (double) (*sit) == Approx(4.2) );

  --sit;

  REQUIRE( sit.pos() == 1 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 2 );
  REQUIRE( sit.col() == 1 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  --sit;

  REQUIRE( sit.pos() == 0 );
  REQUIRE( sit.skip_pos == 0 );
  REQUIRE( sit.row() == 0 );
  REQUIRE( sit.col() == 0 );
  REQUIRE( (double) (*sit) == Approx(2.0) );
  }


TEST_CASE("sp_subview_row_iterators_test")
  {
  SpMat<double> b(5, 5);
  b(0, 0) = 1.0;
  b(0, 1) = 1.0;
  b(0, 2) = 1.0;
  b(0, 3) = 1.0;
  b(0, 4) = 1.0;
  b(1, 0) = 1.0;
  b(2, 0) = 1.0;
  b(3, 0) = 1.0;
  b(4, 0) = 1.0;
  b(4, 1) = 1.0;
  b(4, 2) = 1.0;
  b(4, 3) = 1.0;
  b(4, 4) = 1.0;
  b(3, 4) = 1.0;
  b(2, 4) = 1.0;
  b(1, 4) = 1.0;
  b(1, 1) = 2.0;
  b(2, 3) = 1.5;
  b(3, 2) = 2.0;

  // [[1.0 1.0 1.0 1.0 1.0]
  //  [1.0 2.0 0.0 0.0 1.0]
  //  [1.0 0.0 0.0 1.5 1.0]
  //  [1.0 0.0 2.0 0.0 1.0]
  //  [1.0 1.0 1.0 1.0 1.0]]
  SpSubview<double> s = b.submat(1, 1, 3, 3);

  SpSubview<double>::row_iterator it = s.begin_row();

  REQUIRE( it.pos() == 0 );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );
  REQUIRE( it.actual_pos == 6 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  ++it;

  REQUIRE( it.pos() == 1 );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  REQUIRE( it.actual_pos == 12 );
  REQUIRE( (double) (*it) == Approx(1.5) );

  ++it;

  REQUIRE( it.pos() == 2 );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 1 );
  REQUIRE( it.actual_pos == 9 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  ++it;

  REQUIRE( it.pos() == s.n_nonzero );

  --it;

  REQUIRE( it.pos() == 2 );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 1 );
  REQUIRE( it.actual_pos == 9 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  (*it) = 4.0;

  REQUIRE( (double) (*it) == Approx(4.0) );

  --it;

  REQUIRE( it.pos() == 1 );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  REQUIRE( it.actual_pos == 12 );
  REQUIRE( (double) (*it) == Approx(1.5) );

  --it;

  REQUIRE( it.pos() == 0 );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );
  REQUIRE( it.actual_pos == 6 );
  REQUIRE( (double) (*it) == Approx(2.0) );

  // now a different matrix
  SpMat<double> c(5, 5);
  c(1, 1) = 2.0;
  c(2, 3) = 1.5;
  c(3, 2) = 2.0;

  SpSubview<double> ss = c.submat(0, 0, 3, 3);

  SpSubview<double>::row_iterator sit = ss.begin_row();

  REQUIRE( sit.pos() == 0 );
  REQUIRE( sit.row() == 1 );
  REQUIRE( sit.col() == 1 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  ++sit;

  REQUIRE( sit.pos() == 1 );
  REQUIRE( sit.row() == 2 );
  REQUIRE( sit.col() == 3 );
  REQUIRE( (double) (*sit) == Approx(1.5) );

  ++sit;

  REQUIRE( sit.pos() == 2 );
  REQUIRE( sit.row() == 3 );
  REQUIRE( sit.col() == 2 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  ++sit;

  REQUIRE( sit.pos() == ss.n_nonzero );

  --sit;

  REQUIRE( sit.pos() == 2 );
  REQUIRE( sit.row() == 3 );
  REQUIRE( sit.col() == 2 );
  REQUIRE( (double) (*sit) == Approx(2.0) );

  (*sit) = 4.0;

  REQUIRE( (double) (*sit) == Approx(4.0) );

  --sit;

  REQUIRE( sit.pos() == 1 );
  REQUIRE( sit.row() == 2 );
  REQUIRE( sit.col() == 3 );
  REQUIRE( (double) (*sit) == Approx(1.5) );

  --sit;

  REQUIRE( sit.pos() == 0 );
  REQUIRE( sit.row() == 1 );
  REQUIRE( sit.col() == 1 );
  REQUIRE( (double) (*sit) == Approx(2.0) );
  }


TEST_CASE("sp_subview_sp_base_add_subtract_modulo")
  {
  SpMat<double> m;
  m.sprandu(100, 100, 0.1);

  SpMat<double> n;
  n.sprandu(50, 50, 0.1);

  Mat<double> x(m);
  Mat<double> y(n);

  m.submat(25, 25, 74, 74) += n;
  x.submat(25, 25, 74, 74) += y;

  for (uword c = 0; c < 100; ++c)
    {
    for (uword r = 0; r < 100; ++r)
      {
      REQUIRE( (double) m(r, c) == Approx(x(r, c)) );
      }
    }

  m.sprandu(100, 100, 0.1);
  n.sprandu(50, 50, 0.1);

  x = m;
  y = n;

  m.submat(25, 25, 74, 74) -= n;
  x.submat(25, 25, 74, 74) -= y;

  for (uword c = 0; c < 100; ++c)
    {
    for (uword r = 0; r < 100; ++r)
      {
      REQUIRE( (double) m(r, c) == Approx(x(r, c)) );
      }
    }

  m.sprandu(100, 100, 0.1);
  n.sprandu(50, 50, 0.1);

  x = m;
  y = n;

  m.submat(25, 25, 74, 74) %= n;
  x.submat(25, 25, 74, 74) %= y;

  for( uword c = 0; c < 100; ++c)
    {
    for( uword r = 0; r < 100; ++r)
      {
      REQUIRE( (double) m(r, c) == Approx(x(r, c)) );
      }
    }
  }

TEST_CASE("sp_subview_hadamard")
  {
  SpMat<double> x;
  x.sprandu(100, 100, 0.1);
  Mat<double> d(x);

  SpMat<double> y;
  y.sprandu(200, 200, 0.1);
  Mat<double> dy(y);

  x %= y.submat(50, 50, 149, 149);
  d %= dy.submat(50, 50, 149, 149);

  for (uword c = 0; c < 100; ++c)
    {
    for (uword r = 0; r < 100; ++r)
      {
      REQUIRE( (double) x(r, c) == Approx(d(r, c)) );
      }
    }
  }


TEST_CASE("sp_subview_subviews_test")
  {
  SpMat<double> m(20, 20);
  m.sprandu(20, 20, 0.3);

  // Get a subview.
  SpSubview<double> s = m.submat(1, 1, 10, 10); // 10x10
  const SpSubview<double> c = m.submat(1, 1, 10, 10);

  SpSubview<double> t = s.row(1);
  const SpSubview<double> d = c.row(1);

  REQUIRE( t.n_rows == 1 );
  REQUIRE( t.n_cols == 10 );
  REQUIRE( d.n_rows == 1 );
  REQUIRE( d.n_cols == 10 );
  REQUIRE( t.aux_row1 == 2 );
  REQUIRE( t.aux_col1 == 1 );
  for (uword i = 0; i < 10; ++i)
    {
    REQUIRE( (double) t[i] == (double) m(2, i + 1) );
    REQUIRE( d[i] == (double) m(2, i + 1) );
    }

  SpSubview<double> t1 = s.col(2);
  const SpSubview<double> d1 = c.col(2);

  REQUIRE( t1.n_rows == 10 );
  REQUIRE( t1.n_cols == 1 );
  REQUIRE( d1.n_rows == 10 );
  REQUIRE( d1.n_cols == 1 );
  for (uword i = 0; i < 10; ++i)
    {
    REQUIRE( (double) t1[i] == (double) m(i + 1, 3) );
    REQUIRE( d1[i] == (double) m(i + 1, 3) );
    }

  SpSubview<double> t2 = s.rows(3, 5);
  const SpSubview<double> d2 = c.rows(3, 5);

  REQUIRE( t2.n_rows == 3 );
  REQUIRE( t2.n_cols == 10 );
  REQUIRE( d2.n_rows == 3 );
  REQUIRE( d2.n_cols == 10 );
  for (uword j = 0; j < 3; ++j)
    {
    for (uword i = 0; i < 10; ++i)
      {
      REQUIRE( (double) t2(j, i) == (double) m(4 + j, i + 1) );
      REQUIRE( d2(j, i) == (double) m(4 + j, i + 1) );
      }
    }

  SpSubview<double> t3 = s.cols(4, 6);
  const SpSubview<double> d3 = c.cols(4, 6);

  REQUIRE( t3.n_rows == 10 );
  REQUIRE( t3.n_cols == 3 );
  REQUIRE( d3.n_rows == 10 );
  REQUIRE( d3.n_cols == 3 );
  for (uword j = 0; j < 3; ++j)
    {
    for (uword i = 0; i < 10; ++i)
      {
      REQUIRE( (double) t3(i, j) == (double) m(i + 1, 5 + j) );
      REQUIRE( d3(i, j) == (double) m(i + 1, 5 + j) );
      }
    }

  SpSubview<double> t4 = s.submat(1, 1, 6, 6);
  const SpSubview<double> d4 = c.submat(1, 1, 6, 6);

  REQUIRE( t4.n_rows == 6 );
  REQUIRE( t4.n_cols == 6 );
  REQUIRE( d4.n_rows == 6 );
  REQUIRE( d4.n_cols == 6 );
  for (uword j = 0; j < 6; ++j)
    {
    for (uword i = 0; i < 6; ++i)
      {
      REQUIRE( (double) t4(i, j) == (double) m(i + 2, 2 + j) );
      REQUIRE( d4(i, j) == (double) m(i + 2, 2 + j) );
      }
    }

  SpSubview<double> t5 = s.submat(span(2, 8), span(2, 5));
  const SpSubview<double> d5 = c.submat(span(2, 8), span(2, 5));

  REQUIRE( t5.n_rows == 7 );
  REQUIRE( t5.n_cols == 4 );
  REQUIRE( d5.n_rows == 7 );
  REQUIRE( d5.n_cols == 4 );
  for (uword j = 0; j < 4; ++j)
    {
    for (uword i = 0; i < 7; ++i)
      {
      REQUIRE( (double) t5(i, j) == (double) m(i + 3, 3 + j) );
      REQUIRE( d5(i, j) == (double) m(i + 3, 3 + j) );
      }
    }

  SpSubview<double> t6 = s(4, span(1, 5));
  const SpSubview<double> d6 = c(4, span(1, 5));

  REQUIRE( t6.n_rows == 1 );
  REQUIRE( t6.n_cols == 5 );
  REQUIRE( d6.n_rows == 1 );
  REQUIRE( d6.n_cols == 5 );
  for (uword i = 0; i < 5; ++i)
    {
    REQUIRE( (double) t6(i) == (double) m(5, 2 + i) );
    REQUIRE( d6(i) == (double) m(5, 2 + i) );
    }

  SpSubview<double> t7 = s(span(1, 5), 4);
  const SpSubview<double> d7 = c(span(1, 5), 4);

  REQUIRE( t7.n_rows == 5 );
  REQUIRE( t7.n_cols == 1 );
  REQUIRE( d7.n_rows == 5 );
  REQUIRE( d7.n_cols == 1 );
  for (uword i = 0; i < 5; ++i)
    {
    REQUIRE( (double) t7(i) == (double) m(2 + i, 5) );
    REQUIRE( d7(i) == (double) m(2 + i, 5) );
    }

  SpSubview<double> t8 = s(span(1, 9), span(7, 8));
  const SpSubview<double> d8 = c(span(1, 9), span(7, 8));

  REQUIRE( t8.n_rows == 9 );
  REQUIRE( t8.n_cols == 2 );
  REQUIRE( d8.n_rows == 9 );
  REQUIRE( d8.n_cols == 2 );
  for (uword j = 0; j < 2; ++j)
    {
    for (uword i = 0; i < 9; ++i)
      {
      REQUIRE( (double) t8(i, j) == (double) m(i + 2, 8 + j) );
      REQUIRE( d8(i, j) == (double) m(i + 2, 8 + j) );
      }
    }
  }



TEST_CASE("sp_subview_assignment_sp_base")
  {
  mat d(51, 51);
  d.fill(7.0); // Why not?
  mat dd(d);

  sp_mat e;
  e.sprandu(50, 50, 0.3);
  mat ed(e); // Dense copy.

  d.submat(0, 0, 49, 49) = e;
  dd.submat(0, 0, 49, 49) = ed;

  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx(dd[i]) );
    }
  }



TEST_CASE("sp_subview_addition_sp_base")
  {
  mat d(51, 51);
  d.fill(7.0); // Why not?
  mat dd(d);

  sp_mat e;
  e.sprandu(50, 50, 0.3);
  mat ed(e); // Dense copy.

  d.submat(0, 0, 49, 49) += e;
  dd.submat(0, 0, 49, 49) += ed;

  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx(dd[i]) );
    }
  }


TEST_CASE("sp_subview_subtraction_sp_base")
  {
  mat d(51, 51);
  d.fill(7.0); // Why not?
  mat dd(d);

  sp_mat e;
  e.sprandu(50, 50, 0.3);
  mat ed(e); // Dense copy.

  d.submat(0, 0, 49, 49) -= e;
  dd.submat(0, 0, 49, 49) -= ed;

  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx(dd[i]) );
    }
  }



TEST_CASE("sp_subview_schur_sp_base")
  {
  mat d(51, 51);
  d.fill(7.0); // Why not?
  mat dd(d);

  sp_mat e;
  e.sprandu(50, 50, 0.3);
  mat ed(e); // Dense copy.

  d.submat(0, 0, 49, 49) %= e;
  dd.submat(0, 0, 49, 49) %= ed;

  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx(dd[i]) );
    }
  }



TEST_CASE("sp_subview_division_sp_base")
  {
  mat d(51, 51);
  d.fill(7.0); // Why not?
  mat dd(d);

  sp_mat e;
  e.sprandu(50, 50, 0.3);
  mat ed(e); // Dense copy.

  d.submat(0, 0, 49, 49) /= e;
  dd.submat(0, 0, 49, 49) /= ed;

  for (uword i = 0; i < d.n_elem; ++i)
    {
    if (std::isinf(d[i]))
      REQUIRE( std::isinf(dd[i]) );
    else
      REQUIRE( d[i] == Approx(dd[i]) );
    }
  }
