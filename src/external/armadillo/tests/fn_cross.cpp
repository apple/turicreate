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


TEST_CASE("fn_cross_1")
  {
  vec a = { 0.1,  2.3,  4.5 };
  vec b = { 6.7,  8.9, 10.0 };

  vec c = {-17.050, 29.150, -14.520 };

  REQUIRE( accu(abs(cross(a,b) - c)) == Approx(0.0) );

  vec x;

  REQUIRE_THROWS( x = cross(randu<vec>(4), randu<vec>(4)) );
  }
