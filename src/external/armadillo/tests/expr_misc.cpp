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


TEST_CASE("expr_misc_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat B = A( span(0,min(size(A))-1), span(0,min(size(A))-1) );

  colvec q = B.tail_cols(1);
  rowvec r = B.head_rows(1);

  B = B + q*r + B + B.col(1)*B.row(2) + inv(B.t() + B);

  mat C =
    "\
    -0.598176493690805   1.743720221389917  -0.464434209123318  -0.578107329514025  -0.466519088609519;\
     2.235239222999917   0.352055300390581   0.130383508730418   0.178723856228643   0.315212838210605;\
    -1.530100759221318   0.356920033171418   0.660107612169934   1.456138259553199  -0.459039415535322;\
     0.724145463141975   1.174109038919643   1.707140663038199  -0.861429259650926  -0.384555300447272;\
     0.425855872233481  -1.159439708059395  -1.540488679549322  -0.747036381125272  -1.722286962209877;\
    ";

  REQUIRE( accu(abs(B - C)) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
