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


TEST_CASE("mat_plus_1")
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

  mat A_plus_B =
    "\
     0.112606   0.075245  -0.474258  -0.474258   0.075245   0.112606;\
     0.472679   0.355109  -0.194827  -0.194827   0.355109   0.472679;\
    -0.946973   0.037008   0.733889   0.733889   0.037008  -0.946973;\
     0.710185   0.276501   0.065337   0.065337   0.276501   0.710185;\
     0.498289  -0.782681  -0.697973  -0.697973  -0.782681   0.498289;\
    ";

  mat X = A + B;

  REQUIRE( X(0,0) == Approx( 0.112606) );
  REQUIRE( X(1,0) == Approx( 0.472679) );
  REQUIRE( X(2,0) == Approx(-0.946973) );
  REQUIRE( X(3,0) == Approx( 0.710185) );
  REQUIRE( X(4,0) == Approx( 0.498289) );

  REQUIRE( X(0,1) == Approx( 0.075245) );
  REQUIRE( X(1,1) == Approx( 0.355109) );
  REQUIRE( X(2,1) == Approx( 0.037008) );
  REQUIRE( X(3,1) == Approx( 0.276501) );
  REQUIRE( X(4,1) == Approx(-0.782681) );

  REQUIRE( X(0,5) == Approx( 0.112606) );
  REQUIRE( X(1,5) == Approx( 0.472679) );
  REQUIRE( X(2,5) == Approx(-0.946973) );
  REQUIRE( X(3,5) == Approx( 0.710185) );
  REQUIRE( X(4,5) == Approx( 0.498289) );


  mat Y = (2*A + 2*B)/2;

  REQUIRE( Y(0,0) == Approx( 0.112606) );
  REQUIRE( Y(1,0) == Approx( 0.472679) );
  REQUIRE( Y(2,0) == Approx(-0.946973) );
  REQUIRE( Y(3,0) == Approx( 0.710185) );
  REQUIRE( Y(4,0) == Approx( 0.498289) );

  REQUIRE( Y(0,1) == Approx( 0.075245) );
  REQUIRE( Y(1,1) == Approx( 0.355109) );
  REQUIRE( Y(2,1) == Approx( 0.037008) );
  REQUIRE( Y(3,1) == Approx( 0.276501) );
  REQUIRE( Y(4,1) == Approx(-0.782681) );

  REQUIRE( Y(0,5) == Approx( 0.112606) );
  REQUIRE( Y(1,5) == Approx( 0.472679) );
  REQUIRE( Y(2,5) == Approx(-0.946973) );
  REQUIRE( Y(3,5) == Approx( 0.710185) );
  REQUIRE( Y(4,5) == Approx( 0.498289) );


  REQUIRE( accu(abs( mat(A+B) - A_plus_B )) == Approx(0.0) );
  REQUIRE( accu(abs(    (A+B) - A_plus_B )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*(A+B) - 2*A_plus_B )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }



TEST_CASE("mat_plus_2")
  {
  mat A(5,6); A.fill(1.0);
  mat B(5,6); B.fill(2.0);
  mat C(5,6); C.fill(3.0);

  REQUIRE( accu(A + B) == Approx(double(5*6*3)) );

  REQUIRE( accu(A + B + C) == Approx(double(5*6*6)) );

  REQUIRE( accu(A + B/2 + C) == Approx(double(5*6*5)) );

  mat X(6,5);
  REQUIRE_THROWS( A+X );  // adding non-conformant matrices will throw unless ARMA_NO_DEBUG is defined
  }
