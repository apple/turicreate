// Copyright 2015 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2015 National ICT Australia (NICTA)
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


TEST_CASE("fn_sum_1")
  {
  vec a = linspace<vec>(1,5,5);
  vec b = linspace<vec>(1,5,6);

  REQUIRE(sum(a) == Approx(15.0));
  REQUIRE(sum(b) == Approx(18.0));
  }



TEST_CASE("sum2")
  {
  mat A =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  rowvec colsums = { 0.44080, 1.09382, 0.97808, 1.83429 };

  colvec rowsums =
    {
    1.21686,
    1.69436,
    1.43577
    };

  REQUIRE( accu(abs(colsums - sum(A  ))) == Approx(0.0) );
  REQUIRE( accu(abs(colsums - sum(A,0))) == Approx(0.0) );
  REQUIRE( accu(abs(rowsums - sum(A,1))) == Approx(0.0) );
  }


TEST_CASE("sum3")
  {
  mat AA =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  cx_mat A = cx_mat(AA, 0.5*AA);

  rowvec re_colsums = { 0.44080, 1.09382, 0.97808, 1.83429 };

  cx_rowvec cx_colsums = cx_rowvec(re_colsums, 0.5*re_colsums);

  colvec re_rowsums =
    {
    1.21686,
    1.69436,
    1.43577
    };

  cx_colvec cx_rowsums = cx_colvec(re_rowsums, 0.5*re_rowsums);

  REQUIRE( accu(abs(cx_colsums - sum(A  ))) == Approx(0.0) );
  REQUIRE( accu(abs(cx_colsums - sum(A,0))) == Approx(0.0) );
  REQUIRE( accu(abs(cx_rowsums - sum(A,1))) == Approx(0.0) );
  }


TEST_CASE("sum4")
  {
  mat X(100,101, fill::randu);

  REQUIRE( (sum(sum(X))/X.n_elem)                      == Approx(0.5).epsilon(0.01) );
  REQUIRE( (sum(sum(X(span::all,span::all)))/X.n_elem) == Approx(0.5).epsilon(0.01) );
  }



TEST_CASE("sum_spmat")
  {
  SpCol<double> a(5);
  a(0) = 3.0;
  a(2) = 1.5;
  a(3) = 1.0;

  double res = sum(a);
  REQUIRE( res == Approx(5.5) );

  SpRow<double> b(5);
  b(1) = 1.3;
  b(2) = 4.4;
  b(4) = 1.0;

  res = sum(b);
  REQUIRE( res == Approx(6.7) );

  SpMat<double> c(8, 8);
  c(0, 0) = 3.0;
  c(1, 0) = 2.5;
  c(6, 0) = 2.1;
  c(4, 1) = 3.2;
  c(5, 1) = 1.1;
  c(1, 2) = 1.3;
  c(2, 3) = 4.1;
  c(5, 5) = 2.3;
  c(6, 5) = 3.1;
  c(7, 5) = 1.2;
  c(7, 7) = 3.4;

  SpMat<double> result = sum(c, 0);

  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 8 );
  REQUIRE( result.n_nonzero == 6 );
  REQUIRE( (double) result(0, 0) == Approx(7.6) );
  REQUIRE( (double) result(0, 1) == Approx(4.3) );
  REQUIRE( (double) result(0, 2) == Approx(1.3) );
  REQUIRE( (double) result(0, 3) == Approx(4.1) );
  REQUIRE( (double) result(0, 4) == Approx(0.0) );
  REQUIRE( (double) result(0, 5) == Approx(6.6) );
  REQUIRE( (double) result(0, 6) == Approx(0.0) );
  REQUIRE( (double) result(0, 7) == Approx(3.4) );

  result = sum(c, 1);

  REQUIRE( result.n_rows == 8 );
  REQUIRE( result.n_cols == 1 );
  REQUIRE( result.n_nonzero == 7 );
  REQUIRE( (double) result(0, 0) == Approx(3.0) );
  REQUIRE( (double) result(1, 0) == Approx(3.8) );
  REQUIRE( (double) result(2, 0) == Approx(4.1) );
  REQUIRE( (double) result(3, 0) == Approx(0.0) );
  REQUIRE( (double) result(4, 0) == Approx(3.2) );
  REQUIRE( (double) result(5, 0) == Approx(3.4) );
  REQUIRE( (double) result(6, 0) == Approx(5.2) );
  REQUIRE( (double) result(7, 0) == Approx(4.6) );
  }
