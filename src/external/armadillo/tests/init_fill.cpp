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


TEST_CASE("init_fill_1")
  {
  mat Z( 5,  6, fill::zeros);
  mat O( 5,  6, fill::ones);
  mat I( 5,  6, fill::eye);
  mat U(50, 60, fill::randu);
  mat N(50, 60, fill::randn);


  REQUIRE( accu(Z != 0) == 0   );
  REQUIRE( accu(O != 0) == 5*6 );
  REQUIRE( accu(I != 0) == 5   );

  REQUIRE(   mean(vectorise(U)) == Approx(0.500).epsilon(0.05) );
  REQUIRE( stddev(vectorise(U)) == Approx(0.288).epsilon(0.05) );

  REQUIRE(   mean(vectorise(N)) == Approx(0.0).epsilon(0.05) );
  REQUIRE( stddev(vectorise(N)) == Approx(1.0).epsilon(0.05) );

  mat X(5, 6, fill::none);   // only to test instantiation
  }



TEST_CASE("init_fill_2")
  {
  cube Z( 5,  6, 2, fill::zeros);
  cube O( 5,  6, 2, fill::ones);
  cube U(50, 60, 2, fill::randu);
  cube N(50, 60, 2, fill::randn);

  REQUIRE( accu(Z != 0) == 0     );
  REQUIRE( accu(O != 0) == 5*6*2 );

  REQUIRE(   mean(vectorise(U)) == Approx(0.500).epsilon(0.05) );
  REQUIRE( stddev(vectorise(U)) == Approx(0.288).epsilon(0.05) );

  REQUIRE(   mean(vectorise(N)) == Approx(0.0).epsilon(0.05) );
  REQUIRE( stddev(vectorise(N)) == Approx(1.0).epsilon(0.05) );

  cube X(5, 6, 2, fill::none);   // only to test instantiation

  cube I;  REQUIRE_THROWS( I = cube(5, 6, 2, fill::eye) );
  }
