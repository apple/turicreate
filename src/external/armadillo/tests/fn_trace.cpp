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


TEST_CASE("fn_trace_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  vec diagonal = { 0.061198, 0.058956, 0.314156, -0.393139, -0.353768 };

  REQUIRE( accu( trace(A) - accu(diagonal) ) == Approx(0.0) );

  REQUIRE( accu( trace(2*A) - accu(2*diagonal) ) == Approx(0.0) );

  REQUIRE( accu( trace(A+A) - accu(diagonal+diagonal) ) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }



TEST_CASE("fn_trace_spmat")
  {
  SpMat<double> a(6, 6);
  a(0, 0) = 3.0;
  a(2, 1) = 4.4;
  a(4, 1) = 1.2;
  a(0, 2) = 3.1;
  a(1, 2) = 3.2;
  a(2, 2) = 3.3;
  a(3, 3) = 4.0;
  a(5, 3) = 6.0;
  a(5, 4) = 5.9;
  a(5, 5) = 1.2;

  REQUIRE( trace(a) == Approx(11.5) );

  REQUIRE( trace(a.submat(2, 2, 4, 4)) == Approx(7.3) );
  }
