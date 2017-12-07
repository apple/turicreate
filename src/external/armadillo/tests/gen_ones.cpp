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


TEST_CASE("gen_ones_1")
  {
  mat A(5,6,fill::ones);

  REQUIRE( accu(A)  == Approx(double(5*6)) );
  REQUIRE( A.n_rows == 5 );
  REQUIRE( A.n_cols == 6 );

  mat B(5,6,fill::randu);

  B.ones();

  REQUIRE( accu(B)  == Approx(double(5*6)) );
  REQUIRE( B.n_rows == 5 );
  REQUIRE( B.n_cols == 6 );

  mat C = ones<mat>(5,6);

  REQUIRE( accu(C)  == Approx(double(5*6)) );
  REQUIRE( C.n_rows == 5 );
  REQUIRE( C.n_cols == 6 );

  mat D; D = ones<mat>(5,6);

  REQUIRE( accu(D)  == Approx(double(5*6)) );
  REQUIRE( D.n_rows == 5 );
  REQUIRE( D.n_cols == 6 );

  mat E; E = 2*ones<mat>(5,6);

  REQUIRE( accu(E)  == Approx(double(2*5*6)) );
  REQUIRE( E.n_rows == 5 );
  REQUIRE( E.n_cols == 6 );
  }



TEST_CASE("gen_ones_2")
  {
  mat A(5,6,fill::zeros);

  A.col(1).ones();

  REQUIRE( accu(A.col(0)) == Approx(0.0)              );
  REQUIRE( accu(A.col(1)) == Approx(double(A.n_rows)) );
  REQUIRE( accu(A.col(2)) == Approx(0.0)              );

  mat B(5,6,fill::zeros);

  B.row(1).ones();

  REQUIRE( accu(B.row(0)) == Approx(0.0)              );
  REQUIRE( accu(B.row(1)) == Approx(double(B.n_cols)) );
  REQUIRE( accu(B.row(2)) == Approx(0.0)              );

  mat C(5,6,fill::zeros);

  C(span(1,3),span(1,4)).ones();

  REQUIRE( accu(C.head_cols(1)) == Approx(0.0) );
  REQUIRE( accu(C.head_rows(1)) == Approx(0.0) );

  REQUIRE( accu(C.tail_cols(1)) == Approx(0.0) );
  REQUIRE( accu(C.tail_rows(1)) == Approx(0.0) );

  REQUIRE( accu(C(span(1,3),span(1,4))) == Approx(double(3*4)) );

  mat D(5,6,fill::zeros);

  D.diag().ones();

  REQUIRE( accu(D.diag()) == Approx(double(5)) );
  }



TEST_CASE("gen_ones_3")
  {
  mat A(5,6,fill::zeros);

  uvec indices = { 2, 4, 6 };

  A(indices).ones();

  REQUIRE( accu(A) == Approx(double(3)) );

  REQUIRE( A(0)          == Approx(0.0) );
  REQUIRE( A(A.n_elem-1) == Approx(0.0) );

  REQUIRE( A(indices(0)) == Approx(1.0) );
  REQUIRE( A(indices(1)) == Approx(1.0) );
  REQUIRE( A(indices(2)) == Approx(1.0) );
  }
