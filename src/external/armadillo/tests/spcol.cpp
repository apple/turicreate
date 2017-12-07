// Copyright 2011-2017 Ryan Curtin (http://www.ratml.org/)
// Copyright 2011-2012 Matthew Amidon
// Copyright 2011-2012 James Cline
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

TEST_CASE("spcol_insert_test")
  {
  SpCol<double> sp;
  sp.set_size(10, 1);
  // Ensure everything is empty.
  for (size_t i = 0; i < 10; i++)
    REQUIRE( sp(i) == 0.0 );

  // Add an element.
  sp(5, 0) = 43.234;
  REQUIRE( sp.n_nonzero == 1 );
  REQUIRE( (double) sp(5, 0) == Approx(43.234) );

  // Remove the element.
  sp(5, 0) = 0.0;
  REQUIRE( sp.n_nonzero == 0 );
  }

TEST_CASE("col_iterator_test")
  {
  SpCol<double> x(5, 1);
  x(3) = 3.1;
  x(0) = 4.2;
  x(1) = 3.3;
  x(1) = 5.5; // overwrite
  x(2) = 4.5;
  x(4) = 6.4;

  SpCol<double>::iterator it = x.begin();
  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );
  it++;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1);
  REQUIRE( it.col() == 0);
  it++;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 0 );
  it++;

  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 3 );
  REQUIRE( it.col() == 0 );
  it++;

  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 0 );
  it++;

  REQUIRE( it == x.end() );

  // Now let's go backwards.
  it--; // Get it off the end.
  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 0 );
  it--;

  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 3);
  REQUIRE( it.col() == 0);
  it--;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2);
  REQUIRE( it.col() == 0);
  it--;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 0 );
  it--;

  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 0 );
  REQUIRE( it.col() == 0 );

  REQUIRE( it == x.begin() );

  // Try removing an element we itreated to.
  it++;
  it++;
  *it = 0;
  REQUIRE( x.n_nonzero == 4 );
  }

TEST_CASE("basic_sp_col_operator_test")
  {
  // +=, -=, *=, /=, %=
  SpCol<double> a(6, 1);
  a(0) = 3.4;
  a(1) = 2.0;

  SpCol<double> b(6, 1);
  b(0) = 3.4;
  b(3) = 0.4;

  double addResult[6] = {6.8, 2.0, 0.0, 0.4, 0.0, 0.0};
  double subResult[6] = {0.0, 2.0, 0.0, -0.4, 0.0, 0.0};

  SpCol<double> out = a;
  out += b;
  REQUIRE( out.n_nonzero == 3 );
  for (u32 r = 0; r < 6; r++)
    {
    REQUIRE( (double) out(r) == Approx(addResult[r]) );
    }

  out = a;
  out -= b;
  REQUIRE( out.n_nonzero == 2 );
  for (u32 r = 0; r < 6; r++)
    {
    REQUIRE( (double) out(r) == Approx(subResult[r]) );
    }
  }

/*
BOOST_AUTO_TEST_CASE(SparseSparseColMultiplicationTest) {
  SpCol<double> spaa(4, 1);
  SpMat<double> spbb(1, 4);

  spaa(0, 0) = 321.2;
  spaa(1, 0) = .123;
  spaa(2, 0) = 231.4;
  spaa(3, 0) = .03214;

  spbb(0, 0) = 32.23;
  spbb(0, 1) = 5.1;
  spbb(0, 2) = 4.4;
  spbb(0, 3) = .88;

  SpMat<double> precision = spaa;

  precision *= spbb; //Wolfram alpha insisted on rounding..

  spaa *= spbb;

  for (size_t i = 0; i < 4; i++)
    for (size_t j = 0; j < 4; j++)
      BOOST_REQUIRE_CLOSE((double) spaa(i, j), (double) precision(i, j), 1e-5);
}
*/

TEST_CASE("spcol_shed_row_test")
  {
  // On an SpCol
  SpCol<int> e(10);
  e(1) = 5;
  e(4) = 56;
  e(5) = 6;
  e(7) = 4;
  e(8) = 2;
  e(9) = -1;
  e.shed_rows(4, 7);

  REQUIRE( e.n_cols == 1 );
  REQUIRE( e.n_rows == 6 );
  REQUIRE( e.n_nonzero == 3 );
  REQUIRE( (int) e[0] == 0 );
  REQUIRE( (int) e[1] == 5 );
  REQUIRE( (int) e[2] == 0 );
  REQUIRE( (int) e[3] == 0 );
  REQUIRE( (int) e[4] == 2 );
  REQUIRE( (int) e[5] == -1 );
  }



TEST_CASE("spcol_col_constructor")
  {
  SpMat<double> m(100, 100);
  m.sprandu(100, 100, 0.3);

  SpCol<double> c = m.col(0);

  vec v(c);

  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( v(i) == (double) c(i) );
    }
  }
