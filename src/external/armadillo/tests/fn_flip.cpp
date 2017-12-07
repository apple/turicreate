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


TEST_CASE("fn_flip_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat A_fliplr =
    "\
     0.051408  -0.126745  -0.493936   0.019678   0.201990   0.061198;\
     0.035437   0.296153  -0.045465  -0.149362   0.058956   0.437242;\
    -0.454499   0.068317   0.419733   0.314156  -0.031309  -0.492474;\
     0.373833  -0.135040  -0.393139   0.458476   0.411541   0.336352;\
     0.258704  -0.353768  -0.291020  -0.406953  -0.428913   0.239585;\
    ";

  mat A_flipud =
    "\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
    ";


  mat two_times_A_fliplr =
    "\
     0.102816  -0.253490  -0.987872   0.039356   0.403980   0.122396;\
     0.070874   0.592306  -0.090930  -0.298724   0.117912   0.874484;\
    -0.908998   0.136634   0.839466   0.628312  -0.062618  -0.984948;\
     0.747666  -0.270080  -0.786278   0.916952   0.823082   0.672704;\
     0.517408  -0.707536  -0.582040  -0.813906  -0.857826   0.479170;\
    ";

  mat two_times_A_flipud =
    "\
     0.479170  -0.857826  -0.813906  -0.582040  -0.707536   0.517408;\
     0.672704   0.823082   0.916952  -0.786278  -0.270080   0.747666;\
    -0.984948  -0.062618   0.628312   0.839466   0.136634  -0.908998;\
     0.874484   0.117912  -0.298724  -0.090930   0.592306   0.070874;\
     0.122396   0.403980   0.039356  -0.987872  -0.253490   0.102816;\
    ";

  REQUIRE( accu(abs( fliplr(A) - A_fliplr )) == Approx(0.0) );
  REQUIRE( accu(abs( flipud(A) - A_flipud )) == Approx(0.0) );

  REQUIRE( accu(abs( (-fliplr(A)) + A_fliplr )) == Approx(0.0) );
  REQUIRE( accu(abs( (-flipud(A)) + A_flipud )) == Approx(0.0) );

  REQUIRE( accu(abs( fliplr(-A) + A_fliplr )) == Approx(0.0) );
  REQUIRE( accu(abs( flipud(-A) + A_flipud )) == Approx(0.0) );

  REQUIRE( accu(abs( 2*fliplr(A) - two_times_A_fliplr )) == Approx(0.0) );
  REQUIRE( accu(abs( 2*flipud(A) - two_times_A_flipud )) == Approx(0.0) );

  REQUIRE( accu(abs( fliplr(2*A) - two_times_A_fliplr )) == Approx(0.0) );
  REQUIRE( accu(abs( flipud(2*A) - two_times_A_flipud )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
