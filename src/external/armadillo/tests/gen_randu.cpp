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


TEST_CASE("gen_randu_1")
  {
  const uword n_rows = 100;
  const uword n_cols = 101;

  mat A(n_rows,n_cols, fill::randu);

  mat B(n_rows,n_cols); B.randu();

  mat C; C.randu(n_rows,n_cols);

  REQUIRE( (accu(A)/A.n_elem) == Approx(0.5).epsilon(0.01) );
  REQUIRE( (accu(B)/A.n_elem) == Approx(0.5).epsilon(0.01) );
  REQUIRE( (accu(C)/A.n_elem) == Approx(0.5).epsilon(0.01) );

  REQUIRE( (mean(vectorise(A))) == Approx(0.5).epsilon(0.01) );
  }



TEST_CASE("gen_randu_2")
  {
  mat A(50,60,fill::zeros);

  A(span(1,48),span(1,58)).randu();

  REQUIRE( accu(A.head_cols(1)) == Approx(0.0) );
  REQUIRE( accu(A.head_rows(1)) == Approx(0.0) );

  REQUIRE( accu(A.tail_cols(1)) == Approx(0.0) );
  REQUIRE( accu(A.tail_rows(1)) == Approx(0.0) );

  REQUIRE( mean(vectorise(A(span(1,48),span(1,58)))) == Approx(double(0.5)).epsilon(0.01) );
  }
