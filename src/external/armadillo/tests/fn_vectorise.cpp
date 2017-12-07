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


TEST_CASE("fn_vectorise_1")
  {
  mat A =
    "\
     0.061198   0.201990;\
     0.437242   0.058956;\
    -0.492474  -0.031309;\
     0.336352   0.411541;\
    ";

  vec a =
    {
     0.061198,
     0.437242,
    -0.492474,
     0.336352,
     0.201990,
     0.058956,
    -0.031309,
     0.411541,
    };

  rowvec b = { 0.061198, 0.201990, 0.437242, 0.058956, -0.492474, -0.031309, 0.336352, 0.411541 };

  REQUIRE( accu(abs(a - vectorise(A  ))) == Approx(0.0) );
  REQUIRE( accu(abs(a - vectorise(A,0))) == Approx(0.0) );
  REQUIRE( accu(abs(b - vectorise(A,1))) == Approx(0.0) );
  }
