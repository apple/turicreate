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


TEST_CASE("fn_expmat_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768;\
    ";

  mat B =
    "\
     1.02538869823674172   0.13790240598460396  -0.08367367160987488  -0.41792875395466716  -0.06290741731634941;\
     0.51916795070543009   1.02637747615061659  -0.27092263726001209  -0.22015456965211999   0.22949488377909516;\
    -0.52324535450897358  -0.00364605079279409   1.47390226507168753   0.52746339141825938   0.06773886386884656;\
     0.25814904446999026   0.39201997396696092   0.44019950493687843   0.69739553609450822  -0.04345234429697794;\
     0.16726671762680370  -0.40082334719624046  -0.43832065342010318  -0.30675328534305307   0.64261934864584291;\
    ";

  REQUIRE( accu(abs(expmat(A) - B)) == Approx(0.0) );

  mat X;

  REQUIRE_THROWS( X = expmat(A(span(0,3),span::all)) );
  }
