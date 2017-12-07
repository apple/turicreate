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


TEST_CASE("fn_intersect_1")
  {
  ivec A = regspace<ivec>(5, 1);  // 5, 4, 3, 2, 1
  ivec B = regspace<ivec>(3, 7);  // 3, 4, 5, 6, 7

  ivec C = intersect(A,B);       // 3, 4, 5

  REQUIRE( C(0) == 3 );
  REQUIRE( C(1) == 4 );
  REQUIRE( C(2) == 5 );

  REQUIRE( accu(C) == 12 );

  ivec CC;
  uvec iA;
  uvec iB;

  intersect(CC, iA, iB, A, B);

  REQUIRE( accu(abs(C-CC)) == 0 );

  REQUIRE( iA(0) == 2 );
  REQUIRE( iA(1) == 1 );
  REQUIRE( iA(2) == 0 );

  REQUIRE( accu(iA) == 3 );

  REQUIRE( iB(0) == 0 );
  REQUIRE( iB(1) == 1 );
  REQUIRE( iB(2) == 2 );

  REQUIRE( accu(iB) == 3 );
  }


TEST_CASE("fn_intersect_2")
  {
  irowvec A = regspace<irowvec>(5, 1);  // 5, 4, 3, 2, 1
  irowvec B = regspace<irowvec>(3, 7);  // 3, 4, 5, 6, 7

  irowvec C = intersect(A,B);       // 3, 4, 5

  REQUIRE( C(0) == 3 );
  REQUIRE( C(1) == 4 );
  REQUIRE( C(2) == 5 );

  REQUIRE( accu(C) == 12 );

  irowvec CC;
  uvec iA;
  uvec iB;

  intersect(CC, iA, iB, A, B);

  REQUIRE( accu(abs(C-CC)) == 0 );

  REQUIRE( iA(0) == 2 );
  REQUIRE( iA(1) == 1 );
  REQUIRE( iA(2) == 0 );

  REQUIRE( accu(iA) == 3 );

  REQUIRE( iB(0) == 0 );
  REQUIRE( iB(1) == 1 );
  REQUIRE( iB(2) == 2 );

  REQUIRE( accu(iB) == 3 );
  }


TEST_CASE("fn_intersect_3")
  {
  irowvec A = regspace<irowvec>(5, 1);
  irowvec B = regspace<irowvec>(3, 7);

  ivec C;

  REQUIRE_THROWS( C = intersect(A,B) );
  }
