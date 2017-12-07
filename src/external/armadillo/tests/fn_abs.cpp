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


TEST_CASE("fn_abs_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat abs_A =
    "\
     0.061198   0.201990   0.019678   0.493936   0.126745   0.051408;\
     0.437242   0.058956   0.149362   0.045465   0.296153   0.035437;\
     0.492474   0.031309   0.314156   0.419733   0.068317   0.454499;\
     0.336352   0.411541   0.458476   0.393139   0.135040   0.373833;\
     0.239585   0.428913   0.406953   0.291020   0.353768   0.258704;\
    ";

  mat X = abs(A);

  REQUIRE( X(0,0) == Approx(0.061198) );
  REQUIRE( X(1,0) == Approx(0.437242) );
  REQUIRE( X(2,0) == Approx(0.492474) );
  REQUIRE( X(3,0) == Approx(0.336352) );
  REQUIRE( X(4,0) == Approx(0.239585) );

  REQUIRE( X(0,1) == Approx(0.201990) );
  REQUIRE( X(1,1) == Approx(0.058956) );
  REQUIRE( X(2,1) == Approx(0.031309) );
  REQUIRE( X(3,1) == Approx(0.411541) );
  REQUIRE( X(4,1) == Approx(0.428913) );

  REQUIRE( X(0,5) == Approx(0.051408) );
  REQUIRE( X(1,5) == Approx(0.035437) );
  REQUIRE( X(2,5) == Approx(0.454499) );
  REQUIRE( X(3,5) == Approx(0.373833) );
  REQUIRE( X(4,5) == Approx(0.258704) );

  mat Y = abs(2*A) / 2;

  REQUIRE( Y(0,0) == Approx(0.061198) );
  REQUIRE( Y(1,0) == Approx(0.437242) );
  REQUIRE( Y(2,0) == Approx(0.492474) );
  REQUIRE( Y(3,0) == Approx(0.336352) );
  REQUIRE( Y(4,0) == Approx(0.239585) );

  REQUIRE( Y(0,1) == Approx(0.201990) );
  REQUIRE( Y(1,1) == Approx(0.058956) );
  REQUIRE( Y(2,1) == Approx(0.031309) );
  REQUIRE( Y(3,1) == Approx(0.411541) );
  REQUIRE( Y(4,1) == Approx(0.428913) );

  REQUIRE( Y(0,5) == Approx(0.051408) );
  REQUIRE( Y(1,5) == Approx(0.035437) );
  REQUIRE( Y(2,5) == Approx(0.454499) );
  REQUIRE( Y(3,5) == Approx(0.373833) );
  REQUIRE( Y(4,5) == Approx(0.258704) );

  REQUIRE( accu(   abs(A) -   abs_A ) == Approx(0.0) );
  REQUIRE( accu( 2*abs(A) - 2*abs_A ) == Approx(0.0) );

  REQUIRE( accu(   abs(-A) -   abs_A ) == Approx(0.0) );
  REQUIRE( accu( 2*abs(-A) - 2*abs_A ) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }



TEST_CASE("fn_abs_2")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  cx_mat C = cx_mat(A,fliplr(A));

  mat abs_C =
    "\
     0.079925   0.238462   0.494328   0.494328   0.238462   0.079925;\
     0.438676   0.301964   0.156128   0.156128   0.301964   0.438676;\
     0.670149   0.075150   0.524280   0.524280   0.075150   0.670149;\
     0.502876   0.433130   0.603952   0.603952   0.433130   0.502876;\
     0.352603   0.555984   0.500303   0.500303   0.555984   0.352603;\
    ";

  mat X = abs(C);

  REQUIRE( X(0,0) == Approx(0.079925) );
  REQUIRE( X(1,0) == Approx(0.438676) );
  REQUIRE( X(2,0) == Approx(0.670149) );
  REQUIRE( X(3,0) == Approx(0.502876) );
  REQUIRE( X(4,0) == Approx(0.352603) );

  REQUIRE( X(0,1) == Approx(0.238462) );
  REQUIRE( X(1,1) == Approx(0.301964) );
  REQUIRE( X(2,1) == Approx(0.075150) );
  REQUIRE( X(3,1) == Approx(0.433130) );
  REQUIRE( X(4,1) == Approx(0.555984) );

  REQUIRE( X(0,5) == Approx(0.079925) );
  REQUIRE( X(1,5) == Approx(0.438676) );
  REQUIRE( X(2,5) == Approx(0.670149) );
  REQUIRE( X(3,5) == Approx(0.502876) );
  REQUIRE( X(4,5) == Approx(0.352603) );

  REQUIRE( accu(   abs(C) -   abs_C ) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }



TEST_CASE("fn_abs_3")
  {
  vec re =  2*linspace<vec>(1,5,6);
  vec im = -4*linspace<vec>(1,5,6);

  cx_vec a(re,im);

  vec b =
    {
     4.47213595499958,
     8.04984471899924,
    11.62755348299891,
    15.20526224699857,
    18.78297101099824,
    22.36067977499790
    };


  vec c = abs(a);

  REQUIRE( accu(c      - b) == Approx(0.0) );
  REQUIRE( accu(abs(a) - b) == Approx(0.0) );
  }


TEST_CASE("fn_abs_4")
  {
  vec a = -2*linspace<vec>(1,5,6);
  vec b = +2*linspace<vec>(1,5,6);

  REQUIRE( accu(abs(a) - b) == Approx(0.0) );
  REQUIRE( accu(abs(a(span::all)) - b(span::all)) == Approx(0.0) );
  }



TEST_CASE("fn_abs_5")
  {
  mat A = randu<mat>(5,6);

  REQUIRE( accu(abs(-2*A) - (2*A)) == Approx(0.0) );
  REQUIRE( accu(abs(-2*A(span::all,span::all)) - (2*A(span::all,span::all))) == Approx(0.0) );
  }



TEST_CASE("fn_abs_sp_mat")
  {
  SpMat<double> a(3, 3);
  a(0, 2) = 4.3;
  a(1, 1) = -5.5;
  a(2, 2) = -6.3;

  SpMat<double> b = abs(a);

  REQUIRE( (double) b(0, 0) == 0 );
  REQUIRE( (double) b(1, 0) == 0 );
  REQUIRE( (double) b(2, 0) == 0 );
  REQUIRE( (double) b(0, 1) == 0 );
  REQUIRE( (double) b(1, 1) == Approx(5.5) );
  REQUIRE( (double) b(2, 1) == 0 );
  REQUIRE( (double) b(0, 2) == Approx(4.3) );
  REQUIRE( (double) b(1, 2) == 0 );
  REQUIRE( (double) b(2, 2) == Approx(6.3) );

  // Just test that these compile and run.
  b = abs(a);
  b *= abs(a);
  b %= abs(a);
  b /= abs(a);
  }



TEST_CASE("fn_abs_sp_mat_2")
  {
  mat x = randu<mat>(100, 100);
  x -= 0.5;

  SpMat<double> y(x);

  mat xr = abs(x);
  SpMat<double> yr = abs(y);

  for(size_t i = 0; i < xr.n_elem; ++i)
    {
    REQUIRE( xr[i] == Approx((double) yr[i]) );
    }
  }



TEST_CASE("fn_abs_sp_cx_mat")
  {
  cx_mat x = randu<cx_mat>(100, 100);
  x -= cx_double(0.5, 0.5);

  sp_cx_mat y(x);

  mat xr = abs(x);
  SpMat<double> yr = abs(y);

  for(size_t i = 0; i < xr.n_elem; ++i)
    {
    REQUIRE( xr[i] == Approx((double) yr[i]) );
    }
  }
