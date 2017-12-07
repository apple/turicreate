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


TEST_CASE("fn_cov_1")
  {
  vec a =     linspace<vec>(1,5,6);
  vec b = 0.5*linspace<vec>(1,5,6);
  vec c = flipud(b);

  REQUIRE( as_scalar(cov(a,b) - (+1.12)) == Approx(0.0) );
  REQUIRE( as_scalar(cov(a,c) - (-1.12)) == Approx(0.0) );
  }



TEST_CASE("fn_cov_2")
  {
  mat A =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  mat B = 0.5 * A;

  mat C = fliplr(B);

  mat AA =
    "\
     0.670783  -0.191509  -0.120822  -0.211274;\
    -0.191509   0.183669  -0.141426   0.050641;\
    -0.120822  -0.141426   0.261684   0.051254;\
    -0.211274   0.050641   0.051254   0.067270;\
    ";

  mat AB =
    "\
     0.335392  -0.095755  -0.060411  -0.105637;\
    -0.095755   0.091834  -0.070713   0.025320;\
    -0.060411  -0.070713   0.130842   0.025627;\
    -0.105637   0.025320   0.025627   0.033635;\
    ";

  mat AC =
    "\
    -0.105637  -0.060411  -0.095755   0.335392;\
     0.025320  -0.070713   0.091834  -0.095755;\
     0.025627   0.130842  -0.070713  -0.060411;\
     0.033635   0.025627   0.025320  -0.105637;\
    ";

  REQUIRE( accu(abs(cov(A)   - AA)) == Approx(0.0) );
  REQUIRE( accu(abs(cov(A,B) - AB)) == Approx(0.0) );
  REQUIRE( accu(abs(cov(A,C) - AC)) == Approx(0.0) );
  }
