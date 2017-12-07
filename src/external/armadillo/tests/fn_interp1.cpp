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


TEST_CASE("fn_interp1_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  vec x = vectorise(A(0,0,size(5,3)));
  vec y = vectorise(A(0,3,size(5,3)));

  vec xi_a = linspace<vec>( x.min(), x.max(), 10 );
  vec xi_b = flipud( linspace<vec>( x.min(), x.max(), 11 ) );

  vec yi_a;
  vec yi_b;

  interp1(x, y, xi_a, yi_a);
  interp1(x, y, xi_b, yi_b);

  vec yi_a_gt = { 0.419733, 0.241248, 0.149666, 0.058084, 0.057588, 0.152062, -0.284524, -0.307613, -0.336627, 0.373833 };
  vec yi_b_gt = { 0.373833, -0.300357, -0.353940, -0.201854, -0.449865, 0.063571, 0.045817, 0.085559, 0.167982, 0.250406, 0.419733 };

  REQUIRE( accu(abs( yi_a - yi_a_gt )) == Approx(0.0) );
  REQUIRE( accu(abs( yi_b - yi_b_gt )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
