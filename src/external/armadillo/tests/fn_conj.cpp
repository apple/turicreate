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


TEST_CASE("fn_conj_1")
  {
  vec re =   linspace<vec>(1,5,6);
  vec im = 2*linspace<vec>(1,5,6);

  cx_vec a = cx_vec(re,im);
  cx_vec b = conj(a);

  REQUIRE( accu(abs(real(b) - ( re))) == Approx(0.0) );
  REQUIRE( accu(abs(imag(b) - (-im))) == Approx(0.0) );
  }



TEST_CASE("fn_conj2")
  {
  cx_mat A = randu<cx_mat>(5,6);

  cx_mat B = conj(A);

  REQUIRE( all(vectorise(real(B) ==  real(A))) == true );
  REQUIRE( all(vectorise(imag(B) == -imag(A))) == true );
  }
