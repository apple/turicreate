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


TEST_CASE("mat_minus_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat B = fliplr(A);

  mat A_minus_B =
    "\
     0.0097900   0.3287350   0.5136140  -0.5136140  -0.3287350  -0.0097900;\
     0.4018050  -0.2371970  -0.1038970   0.1038970   0.2371970  -0.4018050;\
    -0.0379750  -0.0996260  -0.1055770   0.1055770   0.0996260   0.0379750;\
    -0.0374810   0.5465810   0.8516150  -0.8516150  -0.5465810   0.0374810;\
    -0.0191190  -0.0751450  -0.1159330   0.1159330   0.0751450   0.0191190;\
    ";

  mat neg_of_A_minus_B =
    "\
    -0.0097900  -0.3287350  -0.5136140  +0.5136140  +0.3287350  +0.0097900;\
    -0.4018050  +0.2371970  +0.1038970  -0.1038970  -0.2371970  +0.4018050;\
    +0.0379750  +0.0996260  +0.1055770  -0.1055770  -0.0996260  -0.0379750;\
    +0.0374810  -0.5465810  -0.8516150  +0.8516150  +0.5465810  -0.0374810;\
    +0.0191190  +0.0751450  +0.1159330  -0.1159330  -0.0751450  -0.0191190;\
    ";

  mat X = A - B;

  REQUIRE( X(0,0) == Approx( 0.0097900) );
  REQUIRE( X(1,0) == Approx( 0.4018050) );
  REQUIRE( X(2,0) == Approx(-0.0379750) );
  REQUIRE( X(3,0) == Approx(-0.0374810) );
  REQUIRE( X(4,0) == Approx(-0.0191190) );

  REQUIRE( X(0,1) == Approx( 0.3287350) );
  REQUIRE( X(1,1) == Approx(-0.2371970) );
  REQUIRE( X(2,1) == Approx(-0.0996260) );
  REQUIRE( X(3,1) == Approx( 0.5465810) );
  REQUIRE( X(4,1) == Approx(-0.0751450) );

  REQUIRE( X(0,5) == Approx(-0.0097900) );
  REQUIRE( X(1,5) == Approx(-0.4018050) );
  REQUIRE( X(2,5) == Approx( 0.0379750) );
  REQUIRE( X(3,5) == Approx( 0.0374810) );
  REQUIRE( X(4,5) == Approx( 0.0191190) );


  mat Y = (2*A - 2*B) / 2;

  REQUIRE( Y(0,0) == Approx( 0.0097900) );
  REQUIRE( Y(1,0) == Approx( 0.4018050) );
  REQUIRE( Y(2,0) == Approx(-0.0379750) );
  REQUIRE( Y(3,0) == Approx(-0.0374810) );
  REQUIRE( Y(4,0) == Approx(-0.0191190) );

  REQUIRE( Y(0,1) == Approx( 0.3287350) );
  REQUIRE( Y(1,1) == Approx(-0.2371970) );
  REQUIRE( Y(2,1) == Approx(-0.0996260) );
  REQUIRE( Y(3,1) == Approx( 0.5465810) );
  REQUIRE( Y(4,1) == Approx(-0.0751450) );

  REQUIRE( Y(0,5) == Approx(-0.0097900) );
  REQUIRE( Y(1,5) == Approx(-0.4018050) );
  REQUIRE( Y(2,5) == Approx( 0.0379750) );
  REQUIRE( Y(3,5) == Approx( 0.0374810) );
  REQUIRE( Y(4,5) == Approx( 0.0191190) );


  REQUIRE( accu(mat(A-B) + (neg_of_A_minus_B)) == Approx(0.0) );
  REQUIRE( accu(   (A-B) + (neg_of_A_minus_B)) == Approx(0.0) );

  REQUIRE( accu(abs( 2*(A-B) + 2*neg_of_A_minus_B )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
