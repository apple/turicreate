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


TEST_CASE("fn_cor_1")
  {
  vec a =     linspace<vec>(1,5,6);
  vec b = 0.5*linspace<vec>(1,5,6);
  vec c = flipud(b);

  REQUIRE( as_scalar(cor(a,b) - (+1.0)) == Approx(0.0) );
  REQUIRE( as_scalar(cor(a,c) - (-1.0)) == Approx(0.0) );
  }



TEST_CASE("fn_cor_2")
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
     1.00000  -0.54561  -0.28838  -0.99459;\
    -0.54561   1.00000  -0.64509   0.45559;\
    -0.28838  -0.64509   1.00000   0.38630;\
    -0.99459   0.45559   0.38630   1.00000;\
    ";

  mat AB =
    "\
     1.00000  -0.54561  -0.28838  -0.99459;\
    -0.54561   1.00000  -0.64509   0.45559;\
    -0.28838  -0.64509   1.00000   0.38630;\
    -0.99459   0.45559   0.38630   1.00000;\
    ";

  mat AC =
    "\
    -0.99459  -0.28838  -0.54561   1.00000;\
     0.45559  -0.64509   1.00000  -0.54561;\
     0.38630   1.00000  -0.64509  -0.28838;\
     1.00000   0.38630   0.45559  -0.99459;\
    ";

  REQUIRE( accu(abs(cor(A)   - AA)) == Approx(0.0).epsilon(0.0001) );
  REQUIRE( accu(abs(cor(A,B) - AA)) == Approx(0.0).epsilon(0.0001) );
  REQUIRE( accu(abs(cor(A,C) - AC)) == Approx(0.0).epsilon(0.0001) );
  }
