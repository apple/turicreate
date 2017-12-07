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


TEST_CASE("gen_linspace_1")
  {
  vec a = linspace(1,5,5);

  REQUIRE(a(0) == Approx(1.0));
  REQUIRE(a(1) == Approx(2.0));
  REQUIRE(a(2) == Approx(3.0));
  REQUIRE(a(3) == Approx(4.0));
  REQUIRE(a(4) == Approx(5.0));

  vec b = linspace<vec>(1,5,6);

  REQUIRE(b(0) == Approx(1.0));
  REQUIRE(b(1) == Approx(1.8));
  REQUIRE(b(2) == Approx(2.6));
  REQUIRE(b(3) == Approx(3.4));
  REQUIRE(b(4) == Approx(4.2));
  REQUIRE(b(5) == Approx(5.0));

  rowvec c = linspace<rowvec>(1,5,6);

  REQUIRE(c(0) == Approx(1.0));
  REQUIRE(c(1) == Approx(1.8));
  REQUIRE(c(2) == Approx(2.6));
  REQUIRE(c(3) == Approx(3.4));
  REQUIRE(c(4) == Approx(4.2));
  REQUIRE(c(5) == Approx(5.0));

  mat X = linspace<mat>(1,5,6);

  REQUIRE(X.n_rows == 6);
  REQUIRE(X.n_cols == 1);
  }
