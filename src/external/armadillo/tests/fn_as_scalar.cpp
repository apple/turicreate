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


TEST_CASE("fn_as_scalar_1")
  {
  mat A(1,1); A.fill(2.0);
  mat B(2,2); B.fill(2.0);

  REQUIRE( as_scalar(A) == Approx(2.0) );

  REQUIRE( as_scalar(2+A) == Approx(4.0) );

  REQUIRE( as_scalar(B(span(0,0), span(0,0))) == Approx(2.0) );

  REQUIRE_THROWS( as_scalar(B) );
  }



TEST_CASE("fn_as_scalar_2")
  {
  rowvec r = linspace<rowvec>(1,5,6);
  colvec q = linspace<colvec>(1,5,6);
  mat    X = 0.5*toeplitz(q);

  REQUIRE( as_scalar(r*q) == Approx(65.2) );

  REQUIRE( as_scalar(r*X*q) == Approx(380.848) );

  REQUIRE( as_scalar(r*diagmat(X)*q) == Approx(32.6) );
  REQUIRE( as_scalar(r*inv(diagmat(X))*q) == Approx(130.4) );
  }



TEST_CASE("fn_as_scalar_3")
  {
  cube A(1,1,1); A.fill(2.0);
  cube B(2,2,2); B.fill(2.0);

  REQUIRE( as_scalar(A) == Approx(2.0) );

  REQUIRE( as_scalar(2+A) == Approx(4.0) );

  REQUIRE( as_scalar(B(span(0,0), span(0,0), span(0,0))) == Approx(2.0) );

  REQUIRE_THROWS( as_scalar(B) );
  }
