// Copyright 2011-2017 Ryan Curtin (http://www.ratml.org/)
// Copyright 2017 National ICT Australia (NICTA)
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

TEST_CASE("sprow_shed_col_test")
  {

  SpRow<int> d(10);
  d[3] = 2;
  d[4] = 6;
  d[6] = 2;
  d[7] = 9;
  d[8] = 1;
  d[9] = -2;

  d.shed_cols(4, 7);
  REQUIRE( d.n_cols == 6 );
  REQUIRE( d.n_rows == 1 );
  REQUIRE( d.n_elem == 6 );
  REQUIRE( d.n_nonzero == 3 );
  REQUIRE( d[0] == 0 );
  REQUIRE( d[1] == 0 );
  REQUIRE( d[2] == 0 );
  REQUIRE( d[3] == 2 );
  REQUIRE( d[4] == 1 );
  REQUIRE( d[5] == -2 );
  }



TEST_CASE("sprow_row_constructor_test")
  {
  SpMat<double> m(100, 100);
  m.sprandu(100, 100, 0.3);

  SpRow<double> r = m.row(0);

  rowvec v(r);

  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( v(i) == (double) r(i) );
    }
  }
