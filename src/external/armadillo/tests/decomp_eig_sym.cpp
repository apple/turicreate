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


TEST_CASE("decomp_eig_sym_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768;\
    ";

  A = A*A.t();

  vec eigvals1 =
    {
    0.0044188,
    0.0697266,
    0.3364172,
    0.8192910,
    1.1872184
    };

  vec eigvals2 = eig_sym(A);

  vec eigvals3;
  bool status = eig_sym(eigvals3, A);

  vec eigvals4;
  mat eigvecs4;
  eig_sym(eigvals4, eigvecs4, A);

  mat B = eigvecs4 * diagmat(eigvals4) * eigvecs4.t();

  REQUIRE( status == true );
  REQUIRE( accu(abs(eigvals2 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(eigvals3 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(eigvals4 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(A        - B       )) == Approx(0.0) );
  }



TEST_CASE("eig_sym_2")
  {
  cx_mat A =
    {
    { cx_double( 0.111205, +0.074101), cx_double(-0.225872, -0.068474), cx_double(-0.192660, +0.236887), cx_double( 0.355204, -0.355735) },
    { cx_double( 0.119869, +0.217667), cx_double(-0.412722, +0.366157), cx_double( 0.069916, -0.222238), cx_double( 0.234987, -0.072355) },
    { cx_double( 0.003791, +0.183253), cx_double(-0.212887, -0.172758), cx_double( 0.168689, -0.393418), cx_double( 0.008795, -0.289654) },
    { cx_double(-0.331639, -0.166660), cx_double( 0.436969, -0.313498), cx_double(-0.431574, +0.017421), cx_double(-0.104165, +0.145246) }
    };

  A = A*A.t();

  vec eigvals1 =
    {
    0.030904,
    0.253778,
    0.432459,
    1.204726
    };

  vec eigvals2 = eig_sym(A);

  vec eigvals3;
  bool status = eig_sym(eigvals3, A);

     vec eigvals4;
  cx_mat eigvecs4;
  eig_sym(eigvals4, eigvecs4, A);

  cx_mat B = eigvecs4 * diagmat(eigvals4) * eigvecs4.t();

  REQUIRE( status == true );
  REQUIRE( accu(abs(eigvals2 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(eigvals3 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(eigvals4 - eigvals1)) == Approx(0.0) );
  REQUIRE( accu(abs(A        - B       )) == Approx(0.0) );
  }



TEST_CASE("eig_sym_3")
  {
  mat A(5,6,fill::randu);

  vec eigvals;
  mat eigvecs;

  REQUIRE_THROWS( eig_sym(eigvals, eigvecs, A) );
  }
