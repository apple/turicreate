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


TEST_CASE("fn_eps_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768;\
    ";

  // NOTE: this may not be portable,
  //       due to minor differences in representations of floating numbers between architectures

  mat B =
    "\
     6.93889390390723e-18   2.77555756156289e-17   3.46944695195361e-18   5.55111512312578e-17   2.77555756156289e-17;\
     5.55111512312578e-17   6.93889390390723e-18   2.77555756156289e-17   6.93889390390723e-18   5.55111512312578e-17;\
     5.55111512312578e-17   6.93889390390723e-18   5.55111512312578e-17   5.55111512312578e-17   1.38777878078145e-17;\
     5.55111512312578e-17   5.55111512312578e-17   5.55111512312578e-17   5.55111512312578e-17   2.77555756156289e-17;\
     2.77555756156289e-17   5.55111512312578e-17   5.55111512312578e-17   5.55111512312578e-17   5.55111512312578e-17;\
    ";

  REQUIRE( accu(abs(eps(A) - B)) == Approx(0.0) );
  }
