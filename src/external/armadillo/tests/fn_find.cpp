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


TEST_CASE("fn_find_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  A(2,2) = 0.0;

  uvec indices_nonzero = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 };

  uvec indices_zero = { 12 };

  uvec indices_greaterthan_00 = { 0, 1, 3, 4, 5, 6, 8, 10, 13, 17, 21, 22, 25, 26, 28, 29 };

  uvec indices_lessthan_00 = { 2, 7, 9, 11, 14, 15, 16, 18, 19, 20, 23, 24, 27 };

  uvec indices_greaterthan_04 = { 1, 8, 13, 17 };

  uvec indices_lessthan_neg04 = { 2, 9, 14, 15, 27 };

  REQUIRE( accu(abs( conv_to<vec>::from(find(A       )) - conv_to<vec>::from(indices_nonzero       ) )) == Approx(0.0) );

  REQUIRE( accu(abs( conv_to<vec>::from(find(A == 0.0)) - conv_to<vec>::from(indices_zero          ) )) == Approx(0.0) );

  REQUIRE( accu(abs( conv_to<vec>::from(find(A >  0.0)) - conv_to<vec>::from(indices_greaterthan_00) )) == Approx(0.0) );

  REQUIRE( accu(abs( conv_to<vec>::from(find(A <  0.0)) - conv_to<vec>::from(indices_lessthan_00   ) )) == Approx(0.0) );

  REQUIRE( accu(abs( conv_to<vec>::from(find(A >  0.4)) - conv_to<vec>::from(indices_greaterthan_04) )) == Approx(0.0) );

  REQUIRE( accu(abs( conv_to<vec>::from(find(A < -0.4)) - conv_to<vec>::from(indices_lessthan_neg04) )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
