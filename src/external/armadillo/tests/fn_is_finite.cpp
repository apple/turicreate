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


TEST_CASE("fn_is_finite_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat B = A;  B(1,1) = datum::inf;

  mat C = A;  C(2,4) = datum::nan;

  REQUIRE( is_finite(A) == true  );
  REQUIRE( is_finite(B) == false );
  REQUIRE( is_finite(C) == false );

  REQUIRE( is_finite(A+A) == true  );
  REQUIRE( is_finite(B+B) == false );
  REQUIRE( is_finite(C+C) == false );

  REQUIRE( is_finite(2*A) == true  );
  REQUIRE( is_finite(2*B) == false );
  REQUIRE( is_finite(2*C) == false );

  // REQUIRE_THROWS(  );
  }
