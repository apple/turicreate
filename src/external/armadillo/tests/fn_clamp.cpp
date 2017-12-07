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


TEST_CASE("fn_clamp_1")
  {
  mat A = randu<mat>(5,6);

  mat B = clamp(A, 0.2, 0.8);
  REQUIRE( B.min() == Approx(0.2) );
  REQUIRE( B.max() == Approx(0.8) );

  mat C = clamp(A, A.min(), 0.8);
  REQUIRE( C.min() == A.min()     );
  REQUIRE( C.max() == Approx(0.8) );

  mat D = clamp(A, 0.2, A.max());
  REQUIRE( D.min() == Approx(0.2) );
  REQUIRE( D.max() == A.max()     );

  REQUIRE_THROWS( clamp(A, A.max(), A.min() ) );
  }
