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


TEST_CASE("fn_conv_1")
  {
  vec a =   linspace<vec>(1,5,6);
  vec b = 2*linspace<vec>(1,6,7);

  vec c = conv(a,b);
  vec d =
    {
      2.00000000000000,
      7.26666666666667,
     17.13333333333333,
     32.93333333333334,
     56.00000000000000,
     87.66666666666667,
    117.66666666666666,
    134.00000000000003,
    137.73333333333335,
    127.53333333333336,
    102.06666666666668,
     60.00000000000000
     };

  REQUIRE( accu(abs(c - d)) == Approx(0.0) );
  }
