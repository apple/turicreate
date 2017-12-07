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


TEST_CASE("fn_accu_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  REQUIRE( accu(A)          == Approx( 0.240136) );
  REQUIRE( accu(abs(A))     == Approx( 7.845382) );
  REQUIRE( accu(A-A)        == Approx( 0.0     ) );
  REQUIRE( accu(A+A)        == Approx( 0.480272) );
  REQUIRE( accu(2*A)        == Approx( 0.480272) );
  REQUIRE( accu( -A)        == Approx(-0.240136) );
  REQUIRE( accu(2*A+3*A)    == Approx( 1.200680) );
  REQUIRE( accu(fliplr(A))  == Approx( 0.240136) );
  REQUIRE( accu(flipud(A))  == Approx( 0.240136) );
  REQUIRE( accu(A.col(1))   == Approx( 0.212265) );
  REQUIRE( accu(A.row(1))   == Approx( 0.632961) );
  REQUIRE( accu(2*A.col(1)) == Approx( 0.424530) );
  REQUIRE( accu(2*A.row(1)) == Approx( 1.265922) );

  REQUIRE( accu(A%A)        == Approx(2.834218657806) );
  REQUIRE( accu(A*A.t())    == Approx(1.218704694166) );
  REQUIRE( accu(A.t()*A)    == Approx(2.585464740700) );

  REQUIRE( accu(vectorise(A)) == Approx(0.240136) );

  REQUIRE( accu(   A(span(1,3), span(1,4)) ) == Approx(1.273017) );
  REQUIRE( accu( 2*A(span(1,3), span(1,4)) ) == Approx(2.546034) );

  REQUIRE_THROWS( accu(A*A) );
  }



TEST_CASE("fn_accu_2")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  cx_mat C = cx_mat(A, 2*fliplr(A));
  cx_mat D = cx_mat(2*fliplr(A), A);

  REQUIRE( abs(accu(C) - cx_double(0.240136, +0.480272)) == Approx(0.0) );

  REQUIRE( abs(accu(cx_double(2,3)*C) - cx_double(-0.960544000000001, +1.680951999999999)) == Approx(0.0) );

  REQUIRE( abs(accu(C*D.t() ) - cx_double(-0.710872588088, +3.656114082498002)) == Approx(0.0) );
  REQUIRE( abs(accu(C*D.st()) - cx_double(0.0,             +6.093523470830000)) == Approx(0.0) );

  REQUIRE( abs(accu(C.t() *D) - cx_double(10.341858962800, -7.756394222100000)) == Approx(0.0) );
  REQUIRE( abs(accu(C.st()*D) - cx_double(0.0,             +1.29273237035e+01)) == Approx(0.0) );
  }



TEST_CASE("fn_accu_3")
  {
  vec a =  linspace<vec>(1,5,5);
  vec b =  linspace<vec>(1,5,6);
  vec c = -linspace<vec>(1,5,6);

  REQUIRE(accu(a) == Approx( 15.0));
  REQUIRE(accu(b) == Approx( 18.0));
  REQUIRE(accu(c) == Approx(-18.0));
  }



TEST_CASE("fn_accu_4")
  {
  mat A(5,6);  A.fill(2.0);
  mat B(5,6);  B.fill(4.0);
  mat C(6,5);  C.fill(6.0);

  REQUIRE( accu(A + B)                                           == Approx(double((2+4)*(A.n_rows*A.n_cols))) );
  REQUIRE( accu(A(span::all,span::all) + B(span::all,span::all)) == Approx(double((2+4)*(A.n_rows*A.n_cols))) );

  REQUIRE( accu(A % B)                                           == Approx(double((2*4)*(A.n_rows*A.n_cols))) );
  REQUIRE( accu(A(span::all,span::all) % B(span::all,span::all)) == Approx(double((2*4)*(A.n_rows*A.n_cols))) );

  // A and C matrices are non-conformat, so accu() will throw unless ARMA_NO_DEBUG is defined
  REQUIRE_THROWS( accu(A % C) );
  REQUIRE_THROWS( accu(A(span::all,span::all) % C(span::all,span::all)) );
  }



TEST_CASE("fn_accu_spmat")
  {
  SpMat<unsigned int> b(4, 4);
  b(0, 1) = 6;
  b(1, 3) = 15;
  b(3, 1) = 14;
  b(2, 0) = 5;
  b(3, 3) = 12;

  REQUIRE( accu(b) == 52 );
  REQUIRE( accu(b.submat(1, 1, 3, 3)) == 41 );
  }
