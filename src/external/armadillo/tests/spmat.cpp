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

// Does the matrix correctly report when it is empty?
TEST_CASE("empty_test")
  {
  bool testPassed = true;

  sp_imat test;
  REQUIRE( test.is_empty() );

  test.set_size(3, 4);
  REQUIRE( test.is_empty() == false );
  }

// Can we insert items into the matrix correctly?
TEST_CASE("insertion_test")
  {
  int correctResult[3][4] =
      {{1, 0, 0, 0},
       {2, 3, 1, 0},
       {0, 9, 4, 0}};

  // Now run the same test for the Armadillo sparse matrix.
  SpMat<int> arma_test;
  arma_test.set_size(3, 4);

  // Fill the matrix (hopefully).
  arma_test(0, 0) = 1;
  arma_test(1, 0) = 2;
  arma_test(1, 1) = 3;
  arma_test(2, 1) = 9;
  arma_test(1, 2) = 1;
  arma_test(2, 2) = 4;

  for (size_t i = 0; i < 3; i++)
    {
    for (size_t j = 0; j < 4; j++)
      {
      REQUIRE( (int) arma_test(i, j) == correctResult[i][j] );
      }
    }
  }

// Does sparse-sparse matrix multiplication work?
TEST_CASE("full_sparse_sparse_matrix_multiplication_test")
  {
  // Now perform the test again for SpMat.
  SpMat<int> spa(3, 3);
  SpMat<int> spb(3, 2);
  int correctResult[3][2] =
      {{ 46,  60},
       { 40,  52},
       {121, 160}};

  spa(0, 0) = 1;
  spa(0, 1) = 10;
  spa(0, 2) = 3;
  spa(1, 0) = 3;
  spa(1, 1) = 4;
  spa(1, 2) = 5;
  spa(2, 0) = 12;
  spa(2, 1) = 13;
  spa(2, 2) = 14;

  spb(0, 0) = 1;
  spb(0, 1) = 2;
  spb(1, 0) = 3;
  spb(1, 1) = 4;
  spb(2, 0) = 5;
  spb(2, 1) = 6;

  spa *= spb;

  REQUIRE( spa.n_rows == 3 );
  REQUIRE( spa.n_cols == 2 );

  for (size_t i = 0; i < 3; i++)
    {
    for (size_t j = 0; j < 2; j++)
      {
      REQUIRE( (int) spa(i, j) == correctResult[i][j] );
      }
    }
  }

TEST_CASE("sparse_sparse_matrix_multiplication_test")
  {
  SpMat<double> spaa(10, 10);
  spaa(1, 5) = 0.4;
  spaa(0, 4) = 0.3;
  spaa(0, 8) = 1.2;
  spaa(3, 0) = 1.1;
  spaa(3, 1) = 1.1;
  spaa(3, 2) = 1.1;
  spaa(4, 4) = 0.2;
  spaa(4, 9) = 0.1;
  spaa(6, 2) = 4.1;
  spaa(6, 8) = 4.1;
  spaa(7, 5) = 1.0;
  spaa(8, 9) = 0.4;
  spaa(9, 4) = 0.4;

  double correctResultB[10][10] =
    {{ 0.00, 0.00, 0.00, 0.00, 0.06, 0.00, 0.00, 0.00, 0.00, 0.51 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.33, 0.44, 0.00, 0.00, 1.32, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.08, 0.00, 0.00, 0.00, 0.00, 0.02 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.64 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.16, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.08, 0.00, 0.00, 0.00, 0.00, 0.04 }};

  spaa *= spaa;

  for (size_t i = 0; i < 10; i++)
    {
    for (size_t j = 0; j < 10; j++)
      {
      REQUIRE( (double) spaa(i, j) == Approx(correctResultB[i][j]) );
      }
    }
  }

TEST_CASE("hadamard_product_test")
  {
  SpMat<int> a(4, 4), b(4, 4);

  a(1, 1) = 1;
  a(2, 1) = 1;
  a(3, 3) = 1;
  a(3, 0) = 1;
  a(0, 2) = 1;

  b(1, 1) = 1;
  b(2, 2) = 1;
  b(3, 3) = 1;
  b(3, 0) = 1;
  b(0, 3) = 1;
  b(3, 1) = 1;

  double correctResult[4][4] =
    {{ 0, 0, 0, 0 },
     { 0, 1, 0, 0 },
     { 0, 0, 0, 0 },
     { 1, 0, 0, 1 }};

  a %= b;

  for (size_t i = 0; i < 4; i++)
    {
    for (size_t j = 0; j < 4; j++)
      {
      REQUIRE( a(i, j) == correctResult[i][j] );
      }
    }


  SpMat<double> c, d;
  c.sprandu(30, 25, 0.1);
  d.sprandu(30, 25, 0.1);

  mat e, f;
  e = c;
  f = d;

  c %= d;
  e %= f;

  for (uword i = 0; i < 25; ++i)
    {
    for(uword j = 0; j < 30; ++j)
      {
      REQUIRE( (double) c(j, i) == Approx(e(j, i)) );
      }
    }
  }

TEST_CASE("division_test")
  {
  SpMat<double> a(2, 2), b(2, 2);

  a(0, 1) = 0.5;

  b(0, 1) = 1.0;
  b(1, 0) = 5.0;

  a /= b;

  REQUIRE( std::isnan((double) a(0, 0)) );
  REQUIRE( (double) a(0, 1) == Approx(0.5) );
  REQUIRE( (double) a(1, 0) == Approx(1e-5) );
  REQUIRE( std::isnan((double) a(1, 1)) );
  }

TEST_CASE("insert_delete_test")
  {
  SpMat<double> sp;
  sp.set_size(10, 10);

  // Ensure everything is empty.
  for (size_t i = 0; i < 100; i++)
    {
    REQUIRE( sp(i) == 0.0 );
    }

  // Add an element.
  sp(5, 5) = 43.234;
  REQUIRE( sp.n_nonzero == 1 );
  REQUIRE( (double) sp(5, 5) == Approx(43.234) );

  // Remove the element.
  sp(5, 5) = 0.0;
  REQUIRE( sp.n_nonzero == 0 );
  }

TEST_CASE("value_operator_test")
  {
  // Test operators that work with a single value.
  // =(double), /=(double), *=(double)
  SpMat<double> sp(3, 4);
  double correctResult[3][4] = {{1.5, 0.0, 0.0, 0.0},
                                {2.1, 3.2, 0.9, 0.0},
                                {0.0, 9.3, 4.0, -1.5}};
  sp(0, 0) = 1.5;
  sp(1, 0) = 2.1;
  sp(1, 1) = 3.2;
  sp(1, 2) = 0.9;
  sp(2, 1) = 9.3;
  sp(2, 2) = 4.0;
  sp(2, 3) = -1.5;

  // operator=(double)
  SpMat<double> work = sp;
  work = 5.0;
  REQUIRE( work.n_nonzero == 1 );
  REQUIRE( work.n_elem == 1 );
  REQUIRE( (double) work(0) == Approx(5.0) );

  // operator*=(double)
  work = sp;
  work *= 2;
  REQUIRE( work.n_nonzero == 7 );
  for (size_t i = 0; i < 3; i++)
    {
    for (size_t j = 0; j < 4; j++)
      {
      REQUIRE((double) work(i, j) == Approx(correctResult[i][j] * 2.0) );
      }
    }

  // operator/=(double)
  work = sp;
  work /= 5.5;
  REQUIRE( work.n_nonzero == 7 );
  for (size_t i = 0; i < 3; i++)
    {
    for (size_t j = 0; j < 4; j++)
      {
      REQUIRE((double) work(i, j) == Approx(correctResult[i][j] / 5.5) );
      }
    }
  }

TEST_CASE("iterator_test")
  {
  SpMat<double> x(5, 5);
  x(4, 1) = 3.1;
  x(1, 2) = 4.2;
  x(1, 3) = 3.3;
  x(1, 3) = 5.5; // overwrite
  x(2, 3) = 4.5;
  x(4, 4) = 6.4;

  SpMat<double>::iterator it = x.begin();
  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 1 );
  it++;

  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  it++;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 3 );
  it++;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 3 );
  it++;

  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 4 );
  it++;

  REQUIRE( it == x.end() );

  // Now let's go backwards.
  it--; // Get it off the end.
  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 4 );
  it--;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 3 );
  it--;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 3 );
  it--;

  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  it--;

  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 1 );

  REQUIRE( it == x.begin() );

  // Try removing an element we iterated to.
  it++;
  it++;
  *it = 0;
  REQUIRE( x.n_nonzero == 4 );
}

TEST_CASE("row_iterator_test")
  {
  SpMat<double> x(5, 5);
  x(4, 1) = 3.1;
  x(1, 2) = 4.2;
  x(1, 3) = 3.3;
  x(1, 3) = 5.5; // overwrite
  x(2, 3) = 4.5;
  x(4, 4) = 6.4;

  SpMat<double>::row_iterator it = x.begin_row();
  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );
  it++;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 3 );
  it++;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 3 );
  it++;

  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 1 );
  it++;

  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 4 );
  it++;

//  REQUIRE( it == x.end_row() );

  // Now let's go backwards.
  it--; // Get it off the end.
  REQUIRE( (double) *it == Approx(6.4) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 4 );
  it--;

  REQUIRE( (double) *it == Approx(3.1) );
  REQUIRE( it.row() == 4 );
  REQUIRE( it.col() == 1 );
  it--;

  REQUIRE( (double) *it == Approx(4.5) );
  REQUIRE( it.row() == 2 );
  REQUIRE( it.col() == 3 );
  it--;

  REQUIRE( (double) *it == Approx(5.5) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 3 );
  it--;

  REQUIRE( (double) *it == Approx(4.2) );
  REQUIRE( it.row() == 1 );
  REQUIRE( it.col() == 2 );

  REQUIRE( it == x.begin_row() );

  // Try removing an element we itreated to.
  it++;
  it++;
  *it = 0;
  REQUIRE( x.n_nonzero == 4 );
  }

TEST_CASE("basic_sp_mat_operator_test")
  {
  // +=, -=, *=, /=, %=
  SpMat<double> a(6, 5);
  a(0, 0) = 3.4;
  a(4, 1) = 4.1;
  a(5, 1) = 1.5;
  a(3, 2) = 2.6;
  a(4, 2) = 3.0;
  a(1, 3) = 9.8;
  a(4, 3) = 0.1;
  a(2, 4) = 0.2;
  a(3, 4) = 0.2;
  a(4, 4) = 0.2;
  a(5, 4) = 8.3;

  SpMat<double> b(6, 5);
  b(0, 0) = 3.4;
  b(3, 0) = 0.4;
  b(3, 1) = 0.5;
  b(4, 1) = 1.2;
  b(4, 2) = 3.0;
  b(5, 2) = 1.1;
  b(1, 3) = 0.6;
  b(3, 3) = 1.0;
  b(4, 4) = 7.3;
  b(5, 4) = 7.4;

  double addResult[6][5] = {{6.8 , 0   , 0   , 0   , 0   },
                            {0   , 0   , 0   , 10.4, 0   },
                            {0   , 0   , 0   , 0   , 0.2 },
                            {0.4 , 0.5 , 2.6 , 1.0 , 0.2 },
                            {0   , 5.3 , 6.0 , 0.1 , 7.5 },
                            {0   , 1.5 , 1.1 , 0   , 15.7}};

  double subResult[6][5] = {{0   , 0   , 0   , 0   , 0   },
                            {0   , 0   , 0   , 9.2 , 0   },
                            {0   , 0   , 0   , 0   , 0.2 },
                            {-0.4, -0.5, 2.6 , -1.0, 0.2 },
                            {0   , 2.9 , 0   , 0.1 , -7.1},
                            {0   , 1.5 , -1.1, 0   , 0.9 }};

  SpMat<double> out = a;
  out += b;
  REQUIRE( out.n_nonzero == 15 );
  for (uword r = 0; r < 6; r++)
    {
    for (uword c = 0; c < 5; c++)
      {
      REQUIRE( (double) out(r, c) == Approx(addResult[r][c]) );
      }
    }

  out = a;
  out -= b;
  REQUIRE( out.n_nonzero == 13 );
  for (uword r = 0; r < 6; r++)
    {
    for (uword c = 0; c < 5; c++)
      {
      REQUIRE( (double) out(r, c) == Approx(subResult[r][c]) );
      }
    }
  }

TEST_CASE("min_max_test")
  {
  SpMat<double> a(6, 5);
  a(0, 0) = 3.4;
  a(4, 1) = 4.1;
  a(5, 1) = 1.5;
  a(3, 2) = 2.6;
  a(4, 2) = 3.0;
  a(1, 3) = 9.8;
  a(4, 3) = 0.1;
  a(2, 4) = 0.2;
  a(3, 4) = -0.2;
  a(4, 4) = 0.2;
  a(5, 4) = 8.3;

  uword index, row, col;
  REQUIRE( a.min() == Approx(-0.2) );
  REQUIRE( a.min(index) == Approx(-0.2) );
  REQUIRE( index == 27 );
  REQUIRE( a.min(row, col) == Approx(-0.2) );
  REQUIRE( row == 3 );
  REQUIRE( col == 4 );

  REQUIRE( a.max() == Approx(9.8) );
  REQUIRE( a.max(index) == Approx(9.8) );
  REQUIRE( index == 19 );
  REQUIRE( a.max(row, col) == Approx(9.8) );
  REQUIRE( row == 1 );
  REQUIRE( col == 3 );
  }

TEST_CASE("swap_row_test")
  {
  SpMat<double> a(6, 5);
  a(0, 0) = 3.4;
  a(4, 1) = 4.1;
  a(5, 1) = 1.5;
  a(3, 2) = 2.6;
  a(4, 2) = 3.0;
  a(1, 3) = 9.8;
  a(4, 3) = 0.1;
  a(2, 4) = 0.2;
  a(3, 4) = -0.2;
  a(4, 4) = 0.2;
  a(5, 4) = 8.3;

  /**
   * [[3.4  0.0  0.0  0.0  0.0]
   *  [0.0  0.0  0.0  9.8  0.0]
   *  [0.0  0.0  0.0  0.0  0.2]
   *  [0.0  0.0  2.6  0.0 -0.2]
   *  [0.0  4.1  3.0  0.1  0.2]
   *  [0.0  1.5  0.0  0.0  8.3]]
   */
  double swapOne[6][5] =
    {{ 0.0,  0.0,  2.6,  0.0, -0.2},
     { 0.0,  0.0,  0.0,  9.8,  0.0},
     { 0.0,  0.0,  0.0,  0.0,  0.2},
     { 3.4,  0.0,  0.0,  0.0,  0.0},
     { 0.0,  4.1,  3.0,  0.1,  0.2},
     { 0.0,  1.5,  0.0,  0.0,  8.3}};

  double swapTwo[6][5] =
    {{ 0.0,  0.0,  2.6,  0.0, -0.2},
     { 0.0,  0.0,  0.0,  9.8,  0.0},
     { 0.0,  0.0,  0.0,  0.0,  0.2},
     { 3.4,  0.0,  0.0,  0.0,  0.0},
     { 0.0,  1.5,  0.0,  0.0,  8.3},
     { 0.0,  4.1,  3.0,  0.1,  0.2}};

  a.swap_rows(0, 3);

  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(swapOne[row][col]) );
      }
    }

  a.swap_rows(4, 5);

  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(swapTwo[row][col]) );
      }
    }
  }

TEST_CASE("swap_col_test")
  {
  SpMat<double> a(6, 5);
  a(0, 0) = 3.4;
  a(4, 1) = 4.1;
  a(5, 1) = 1.5;
  a(3, 2) = 2.6;
  a(4, 2) = 3.0;
  a(1, 3) = 9.8;
  a(4, 3) = 0.1;
  a(2, 4) = 0.2;
  a(3, 4) = -0.2;
  a(4, 4) = 0.2;
  a(5, 4) = 8.3;

  mat b(6, 5);
  b.zeros(6, 5);
  b(0, 0) = 3.4;
  b(4, 1) = 4.1;
  b(5, 1) = 1.5;
  b(3, 2) = 2.6;
  b(4, 2) = 3.0;
  b(1, 3) = 9.8;
  b(4, 3) = 0.1;
  b(2, 4) = 0.2;
  b(3, 4) = -0.2;
  b(4, 4) = 0.2;
  b(5, 4) = 8.3;

  /**
   * [[3.4  0.0  0.0  0.0  0.0]
   *  [0.0  0.0  0.0  9.8  0.0]
   *  [0.0  0.0  0.0  0.0  0.2]
   *  [0.0  0.0  2.6  0.0 -0.2]
   *  [0.0  4.1  3.0  0.1  0.2]
   *  [0.0  1.5  0.0  0.0  8.3]]
   */

  a.swap_cols(2, 3);
  b.swap_cols(2, 3);

  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(b(row, col)) );
      }
    }

  a.swap_cols(0, 4);
  b.swap_cols(0, 4);

  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(b(row, col)) );
      }
    }

  a.swap_cols(1, 4);
  b.swap_cols(1, 4);

  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(b(row, col)) );
      }
    }
  }

TEST_CASE("shed_col_test")
  {
  SpMat<int> a(2, 2);
  a(0, 0) = 1;
  a(1, 1) = 1;

  /**
   * [[1 0]
   *  [0 1]]
   *
   * becomes
   *
   * [[0]
   *  [1]]
   */

  a.shed_col(0);
  REQUIRE( a.n_cols == 1 );
  REQUIRE( a.n_rows == 2 );
  REQUIRE( a.n_elem == 2 );
  REQUIRE( a.n_nonzero == 1 );
  REQUIRE( a(0, 0) == 0 );
  REQUIRE( a(1, 0) == 1 );
  }

TEST_CASE("shed_cols_test")
  {
  SpMat<int> a(3, 3);
  a(0, 0) = 1;
  a(1, 1) = 1;
  a(2, 2) = 1;
  SpMat<int> b(3, 3);
  b(0, 0) = 1;
  b(1, 1) = 1;
  b(2, 2) = 1;
  SpMat<int> c(3, 3);
  c(0, 0) = 1;
  c(1, 1) = 1;
  c(2, 2) = 1;

  /**
   * [[1 0 0]
   *  [0 1 0]
   *  [0 0 1]]
   *
   * becomes
   *
   * [[0]
   *  [0]
   *  [1]]
   */

  a.shed_cols(0, 1);
  REQUIRE( a.n_cols == 1 );
  REQUIRE( a.n_rows == 3 );
  REQUIRE( a.n_elem == 3 );
  REQUIRE( a.n_nonzero == 1 );
  REQUIRE( a(0, 0) == 0 );
  REQUIRE( a(1, 0) == 0 );
  REQUIRE( a(2, 0) == 1 );

  b.shed_cols(1, 2);
  REQUIRE( b.n_cols == 1 );
  REQUIRE( b.n_rows == 3 );
  REQUIRE( b.n_elem == 3 );
  REQUIRE( b.n_nonzero == 1 );
  REQUIRE( b(0, 0) == 1 );
  REQUIRE( b(1, 0) == 0 );
  REQUIRE( b(2, 0) == 0 );

  c.shed_cols(0, 0);
  c.shed_cols(1, 1);
  REQUIRE( c.n_cols == 1 );
  REQUIRE( c.n_rows == 3 );
  REQUIRE( c.n_elem == 3 );
  REQUIRE( c.n_nonzero == 1 );
  REQUIRE( c(0, 0) == 0 );
  REQUIRE( c(1, 0) == 1 );
  REQUIRE( c(2, 0) == 0 );
  }

TEST_CASE("shed_row_test")
  {
  SpMat<int> a(3, 3);
  a(0, 0) = 1;
  a(1, 1) = 1;
  a(2, 2) = 1;
  Mat<int> b(3, 3);
  b.zeros(3, 3);
  b(0, 0) = 1;
  b(1, 1) = 1;
  b(2, 2) = 1;

  /**
   * [[1 0 0]
   *  [0 1 0]
   *  [0 0 1]]
   *
   * becomes
   *
   * [[1 0 0]
   *  [0 1 0]]
   */
  a.shed_row(2);
  b.shed_row(2);
  REQUIRE( a.n_cols == 3 );
  REQUIRE( a.n_rows == 2 );
  REQUIRE( a.n_elem == 6 );
  REQUIRE( a.n_nonzero == 2 );
  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(b(row, col)) );
      }
    }
  }

TEST_CASE("shed_rows_test")
  {
  SpMat<int> a(5, 5);
  a(0, 0) = 1;
  a(1, 1) = 1;
  a(2, 2) = 1;
  a(3, 3) = 1;
  a(4, 4) = 1;
  Mat<int> b(5, 5);
  b.zeros(5, 5);
  b(0, 0) = 1;
  b(1, 1) = 1;
  b(2, 2) = 1;
  b(3, 3) = 1;
  b(4, 4) = 1;

  SpMat<int> c = a;
  Mat<int> d = b;

  /**
   * [[1 0 0 0 0]
   *  [0 1 0 0 0]
   *  [0 0 1 0 0]
   *  [0 0 0 1 0]
   *  [0 0 0 0 1]]
   *
   * becomes
   *
   * [[1 0 0 0 0]
   *  [0 1 0 0 0]]
   */
  a.shed_rows(2,4);
  b.shed_rows(2,4);
  REQUIRE( a.n_cols == 5 );
  REQUIRE( a.n_rows == 2 );
  REQUIRE( a.n_elem == 10 );
  REQUIRE( a.n_nonzero == 2 );
  for (uword row = 0; row < a.n_rows; row++)
    {
    for (uword col = 0; col < a.n_cols; col++)
      {
      REQUIRE( (double) a(row, col) == Approx(b(row, col)) );
      }
    }

  c.shed_rows(0, 2);
  d.shed_rows(0, 2);
  REQUIRE( c.n_cols == 5 );
  REQUIRE( c.n_rows == 2 );
  REQUIRE( c.n_elem == 10 );
  REQUIRE( c.n_nonzero == 2 );
  for (uword row = 0; row < c.n_rows; ++row)
    {
    for (uword col = 0; col < c.n_cols; ++col)
      {
      REQUIRE( (int) c(row, col) == d(row, col) );
      }
    }
  }

TEST_CASE("sp_mat_reshape_columnwise_test")
  {
  // Input matrix:
  // [[0 2 0]
  //  [1 3 0]
  //  [0 0 5]
  //  [0 4 6]]
  //
  // Output matrix:
  // [[0 0 0 0]
  //  [1 2 4 5]
  //  [0 3 0 6]]
  SpMat<unsigned int> ref(4, 3);
  ref(1, 0) = 1;
  ref(0, 1) = 2;
  ref(1, 1) = 3;
  ref(3, 1) = 4;
  ref(2, 2) = 5;
  ref(3, 2) = 6;

  // Now reshape.
  ref.reshape(3, 4);

  // Check everything.
  REQUIRE( ref.n_cols == 4 );
  REQUIRE( ref.n_rows == 3 );

  REQUIRE( (unsigned int) ref(0, 0) == 0 );
  REQUIRE( (unsigned int) ref(1, 0) == 1 );
  REQUIRE( (unsigned int) ref(2, 0) == 0 );
  REQUIRE( (unsigned int) ref(0, 1) == 0 );
  REQUIRE( (unsigned int) ref(1, 1) == 2 );
  REQUIRE( (unsigned int) ref(2, 1) == 3 );
  REQUIRE( (unsigned int) ref(0, 2) == 0 );
  REQUIRE( (unsigned int) ref(1, 2) == 4 );
  REQUIRE( (unsigned int) ref(2, 2) == 0 );
  REQUIRE( (unsigned int) ref(0, 3) == 0 );
  REQUIRE( (unsigned int) ref(1, 3) == 5 );
  REQUIRE( (unsigned int) ref(2, 3) == 6 );
  }

TEST_CASE("sp_mat_reshape_rowwise_test")
  {
  // Input matrix:
  // [[0 2 0]
  //  [1 3 0]
  //  [0 0 5]
  //  [0 4 6]]
  //
  // Output matrix:
  // [[0 2 0 1]
  //  [3 0 0 0]
  //  [5 0 4 6]]
  SpMat<unsigned int> ref(4, 3);
  ref(1, 0) = 1;
  ref(0, 1) = 2;
  ref(1, 1) = 3;
  ref(3, 1) = 4;
  ref(2, 2) = 5;
  ref(3, 2) = 6;

  // Now reshape.
  ref.reshape(3, 4, 1 /* row-wise */);

  // Check everything.
  REQUIRE( ref.n_cols == 4 );
  REQUIRE( ref.n_rows == 3 );

  REQUIRE( (unsigned int) ref(0, 0) == 0 );
  REQUIRE( (unsigned int) ref(1, 0) == 3 );
  REQUIRE( (unsigned int) ref(2, 0) == 5 );
  REQUIRE( (unsigned int) ref(0, 1) == 2 );
  REQUIRE( (unsigned int) ref(1, 1) == 0 );
  REQUIRE( (unsigned int) ref(2, 1) == 0 );
  REQUIRE( (unsigned int) ref(0, 2) == 0 );
  REQUIRE( (unsigned int) ref(1, 2) == 0 );
  REQUIRE( (unsigned int) ref(2, 2) == 4 );
  REQUIRE( (unsigned int) ref(0, 3) == 1 );
  REQUIRE( (unsigned int) ref(1, 3) == 0 );
  REQUIRE( (unsigned int) ref(2, 3) == 6 );
  }

TEST_CASE("sp_mat_zeros_tests")
  {
  SpMat<double> m(4, 3);
  m(1, 0) = 1;
  m(0, 1) = 2;
  m(1, 1) = 3;
  m(3, 1) = 4;
  m(2, 2) = 5;
  m(3, 2) = 6;

  // Now zero it out.
  SpMat<double> d = m;

  d.zeros();

  REQUIRE( d.values[0] == 0 );
  REQUIRE( d.row_indices[0] == 0);
  REQUIRE( d.col_ptrs[0] == 0 );
  REQUIRE( d.col_ptrs[1] == 0 );
  REQUIRE( d.col_ptrs[2] == 0 );
  REQUIRE( d.col_ptrs[3] == 0 );
  REQUIRE( d.n_cols == 3 );
  REQUIRE( d.n_rows == 4 );
  REQUIRE( d.n_elem == 12 );
  REQUIRE( d.n_nonzero == 0 );

  // Now zero it out again.
  d = m;
  d.zeros(10);

  REQUIRE( d.values[0] == 0 );
  REQUIRE( d.row_indices[0] == 0);
  REQUIRE( d.col_ptrs[0] == 0 );
  REQUIRE( d.col_ptrs[1] == 0 );
  REQUIRE( d.n_cols == 1 );
  REQUIRE( d.n_rows == 10 );
  REQUIRE( d.n_elem == 10 );
  REQUIRE( d.n_nonzero == 0 );

  // Now zero it out again.
  d = m;
  d.zeros(5, 5);

  REQUIRE( d.values[0] == 0 );
  REQUIRE( d.row_indices[0] == 0);
  REQUIRE( d.col_ptrs[0] == 0 );
  REQUIRE( d.col_ptrs[1] == 0 );
  REQUIRE( d.col_ptrs[2] == 0 );
  REQUIRE( d.col_ptrs[3] == 0 );
  REQUIRE( d.col_ptrs[4] == 0 );
  REQUIRE( d.col_ptrs[5] == 0 );
  REQUIRE( d.n_cols == 5 );
  REQUIRE( d.n_rows == 5 );
  REQUIRE( d.n_elem == 25 );
  REQUIRE( d.n_nonzero == 0 );
  }



/**
 * Check that eye() works.
 */
TEST_CASE("sp_mat_eye_test")
  {
  SpMat<double> e = eye<SpMat<double> >(5, 5);

  REQUIRE( e.n_elem == 25 );
  REQUIRE( e.n_rows == 5 );
  REQUIRE( e.n_cols == 5 );
  REQUIRE( e.n_nonzero == 5 );

  for (uword i = 0; i < 5; i++)
    {
    for (uword j = 0; j < 5; j++)
      {
      if (i == j)
        REQUIRE( (double) e(i, j) == Approx(1.0) );
      else
        REQUIRE( (double) e(i, j) == Approx(1e-5) );
      }
    }

  // Just check that these compile and run.
  e = eye<SpMat<double> >(5, 5);
  e *= eye<SpMat<double> >(5, 5);
  e %= eye<SpMat<double> >(5, 5);
  e /= eye<SpMat<double> >(5, 5);
  }

/**
 * Check that pow works.
 *
TEST_CASE("sp_mat_pow_test")
  {
  SpMat<double> a(3, 3);
  a(0, 2) = 4.3;
  a(1, 1) = -5.5;
  a(2, 2) = -6.3;

  a += pow(a, 2);

  REQUIRE( (double) a(0, 0) == 0 );
  REQUIRE( (double) a(1, 0) == 0 );
  REQUIRE( (double) a(2, 0) == 0 );
  REQUIRE( (double) a(0, 1) == 0 );
  REQUIRE( (double) a(1, 1) == Approx(24.75) );
  REQUIRE( (double) a(2, 1) == 0 );
  REQUIRE( (double) a(0, 2) == Approx(22.79) );
  REQUIRE( (double) a(1, 2) == 0 );
  REQUIRE( (double) a(2, 2) == Approx(33.39) );

  a = pow(a, 2);
  a *= pow(a, 2);
  a %= pow(a, 2);
  a /= pow(a, 2);
  }
*/


// I hate myself.
#undef TEST_OPERATOR
#define TEST_OPERATOR(EOP_TEST, EOP) \
TEST_CASE(EOP_TEST) \
  {\
  SpMat<double> a(3, 3);\
  a(0, 2) = 4.3;\
  a(1, 1) = -5.5;\
  a(2, 2) = -6.3;\
  a(1, 0) = 0.001;\
  Mat<double> b(3, 3);\
  b.zeros();\
  b(0, 2) = 4.3;\
  b(1, 1) = -5.5;\
  b(2, 2) = -6.3;\
  b(1, 0) = 0.001;\
  \
  SpMat<double> c = EOP(a);\
  Mat<double> d = EOP(b);\
  \
  if (c(0, 0) == c(0, 0) && d(0, 0) == d(0, 0))\
    REQUIRE( c(0, 0) == d(0, 0) );\
  if (c(1, 0) == c(1, 0) && d(1, 0) == d(1, 0))\
    REQUIRE( c(1, 0) == d(1, 0) );\
  if (c(2, 0) == c(2, 0) && d(2, 0) == d(2, 0))\
    REQUIRE( c(2, 0) == d(2, 0) );\
  if (c(0, 1) == c(0, 1) && d(0, 1) == d(0, 1))\
    REQUIRE( c(0, 1) == d(0, 1) );\
  if (c(1, 1) == c(1, 1) && d(1, 1) == d(1, 1))\
    REQUIRE( c(1, 1) == d(1, 1) );\
  if (c(2, 1) == c(2, 1) && d(2, 1) == d(2, 1))\
    REQUIRE( c(2, 1) == d(2, 1) );\
  if (c(0, 2) == c(0, 2) && d(0, 2) == d(0, 2))\
    REQUIRE( c(0, 2) == d(0, 2) );\
  if (c(1, 2) == c(1, 2) && d(1, 2) == d(1, 2))\
    REQUIRE( c(1, 2) == d(1, 2) );\
  if (c(2, 2) == c(2, 2) && d(2, 2) == d(2, 2))\
    REQUIRE( c(2, 2) == d(2, 2) );\
  \
  c -= EOP(a);\
  d -= EOP(b);\
  \
  if (c(0, 0) == c(0, 0) && d(0, 0) == d(0, 0))\
    REQUIRE( c(0, 0) == d(0, 0) );\
  if (c(1, 0) == c(1, 0) && d(1, 0) == d(1, 0))\
    REQUIRE( c(1, 0) == d(1, 0) );\
  if (c(2, 0) == c(2, 0) && d(2, 0) == d(2, 0))\
    REQUIRE( c(2, 0) == d(2, 0) );\
  if (c(0, 1) == c(0, 1) && d(0, 1) == d(0, 1))\
    REQUIRE( c(0, 1) == d(0, 1) );\
  if (c(1, 1) == c(1, 1) && d(1, 1) == d(1, 1))\
    REQUIRE( c(1, 1) == d(1, 1) );\
  if (c(2, 1) == c(2, 1) && d(2, 1) == d(2, 1))\
    REQUIRE( c(2, 1) == d(2, 1) );\
  if (c(0, 2) == c(0, 2) && d(0, 2) == d(0, 2))\
    REQUIRE( c(0, 2) == d(0, 2) );\
  if (c(1, 2) == c(1, 2) && d(1, 2) == d(1, 2))\
    REQUIRE( c(1, 2) == d(1, 2) );\
  if (c(2, 2) == c(2, 2) && d(2, 2) == d(2, 2))\
    REQUIRE( c(2, 2) == d(2, 2) );\
  \
  c %= EOP(a);\
  d %= EOP(b);\
  \
  if (c(0, 0) == c(0, 0) && d(0, 0) == d(0, 0))\
    REQUIRE( c(0, 0) == d(0, 0) );\
  if (c(1, 0) == c(1, 0) && d(1, 0) == d(1, 0))\
    REQUIRE( c(1, 0) == d(1, 0) );\
  if (c(2, 0) == c(2, 0) && d(2, 0) == d(2, 0))\
    REQUIRE( c(2, 0) == d(2, 0) );\
  if (c(0, 1) == c(0, 1) && d(0, 1) == d(0, 1))\
    REQUIRE( c(0, 1) == d(0, 1) );\
  if (c(1, 1) == c(1, 1) && d(1, 1) == d(1, 1))\
    REQUIRE( c(1, 1) == d(1, 1) );\
  if (c(2, 1) == c(2, 1) && d(2, 1) == d(2, 1))\
    REQUIRE( c(2, 1) == d(2, 1) );\
  if (c(0, 2) == c(0, 2) && d(0, 2) == d(0, 2))\
    REQUIRE( c(0, 2) == d(0, 2) );\
  if (c(1, 2) == c(1, 2) && d(1, 2) == d(1, 2))\
    REQUIRE( c(1, 2) == d(1, 2) );\
  if (c(2, 2) == c(2, 2) && d(2, 2) == d(2, 2))\
    REQUIRE( c(2, 2) == d(2, 2) );\
  \
  c *= EOP(a);\
  d *= EOP(b);\
  \
  if (c(0, 0) == c(0, 0) && d(0, 0) == d(0, 0))\
    REQUIRE( c(0, 0) == d(0, 0) );\
  if (c(1, 0) == c(1, 0) && d(1, 0) == d(1, 0))\
    REQUIRE( c(1, 0) == d(1, 0) );\
  if (c(2, 0) == c(2, 0) && d(2, 0) == d(2, 0))\
    REQUIRE( c(2, 0) == d(2, 0) );\
  if (c(0, 1) == c(0, 1) && d(0, 1) == d(0, 1))\
    REQUIRE( c(0, 1) == d(0, 1) );\
  if (c(1, 1) == c(1, 1) && d(1, 1) == d(1, 1))\
    REQUIRE( c(1, 1) == d(1, 1) );\
  if (c(2, 1) == c(2, 1) && d(2, 1) == d(2, 1))\
    REQUIRE( c(2, 1) == d(2, 1) );\
  if (c(0, 2) == c(0, 2) && d(0, 2) == d(0, 2))\
    REQUIRE( c(0, 2) == d(0, 2) );\
  if (c(1, 2) == c(1, 2) && d(1, 2) == d(1, 2))\
    REQUIRE( c(1, 2) == d(1, 2) );\
  if (c(2, 2) == c(2, 2) && d(2, 2) == d(2, 2))\
    REQUIRE( c(2, 2) == d(2, 2) );\
  \
  c /= EOP(a);\
  d /= EOP(b);\
  \
  if (c(0, 0) == c(0, 0) && d(0, 0) == d(0, 0))\
    REQUIRE( c(0, 0) == d(0, 0) );\
  if (c(1, 0) == c(1, 0) && d(1, 0) == d(1, 0))\
    REQUIRE( c(1, 0) == d(1, 0) );\
  if (c(2, 0) == c(2, 0) && d(2, 0) == d(2, 0))\
    REQUIRE( c(2, 0) == d(2, 0) );\
  if (c(0, 1) == c(0, 1) && d(0, 1) == d(0, 1))\
    REQUIRE( c(0, 1) == d(0, 1) );\
  if (c(1, 1) == c(1, 1) && d(1, 1) == d(1, 1))\
    REQUIRE( c(1, 1) == d(1, 1) );\
  if (c(2, 1) == c(2, 1) && d(2, 1) == d(2, 1))\
    REQUIRE( c(2, 1) == d(2, 1) );\
  if (c(0, 2) == c(0, 2) && d(0, 2) == d(0, 2))\
    REQUIRE( c(0, 2) == d(0, 2) );\
  if (c(1, 2) == c(1, 2) && d(1, 2) == d(1, 2))\
    REQUIRE( c(1, 2) == d(1, 2) );\
  if (c(2, 2) == c(2, 2) && d(2, 2) == d(2, 2))\
    REQUIRE( c(2, 2) == d(2, 2) );\
  }

// Now run all the operators...
TEST_OPERATOR("sp_mat_abs_test", abs)
//TEST_OPERATOR("sp_mat_eps_test", eps);
//TEST_OPERATOR(expTest, exp);
//TEST_OPERATOR(exp2Test, exp2);
//TEST_OPERATOR(exp10Test, exp10);
//TEST_OPERATOR(trunc_expTest, trunc_exp);
//TEST_OPERATOR(logTest, log);
//TEST_OPERATOR(log2Test, log2);
//TEST_OPERATOR(log10Test, log10);
//TEST_OPERATOR(trunc_logTest, trunc_log);
TEST_OPERATOR("sp_mat_sqrt_test", sqrt)
TEST_OPERATOR("sp_mat_square_test", square)
TEST_OPERATOR("sp_mat_floor_test", floor)
TEST_OPERATOR("sp_mat_ceil_test", ceil)
//TEST_OPERATOR(cosTest, cos);
//TEST_OPERATOR(acosTest, acos);
//TEST_OPERATOR(coshTest, cosh);
//TEST_OPERATOR(acoshTest, acosh);
//TEST_OPERATOR(sinTest, sin);
//TEST_OPERATOR(asinTest, asin);
//TEST_OPERATOR(sinhTest, sinh);
//TEST_OPERATOR(asinhTest, asinh);
//TEST_OPERATOR(tanTest, tan);
//TEST_OPERATOR(tanhTest, tanh);
//TEST_OPERATOR(atanTest, atan);
//TEST_OPERATOR(atanhTest, atanh);

/*
TEST_CASE("spmat_diskio_tests")
  {
  std::string file_names[] = {"raw_ascii.txt",
                              "raw_binary.bin",
                              "arma_ascii.csv",
                              "csv_ascii.csv",
                              "arma_binary.bin",
                              "pgm_binary.bin",
                              "coord_ascii.txt"};
  diskio dio;
  SpMat<int> m(4, 3);
  m(0, 0) = 1;
  m(3, 0) = 2;
  m(0, 2) = 3;
  m(3, 2) = 4;
  m(2, 1) = 5;
  m(1, 2) = 6;

  // Save the matrix.
  REQUIRE( dio.save_raw_ascii(m, file_names[0]) );
//  REQUIRE( dio.save_raw_binary(m, file_names[1]) );
//  REQUIRE( dio.save_arma_ascii(m, file_names[2]) );
//  REQUIRE( dio.save_csv_ascii(m, file_names[3]) );
  REQUIRE( dio.save_arma_binary(m, file_names[4]) );
//  REQUIRE( dio.save_pgm_binary(m, file_names[5]) );
  REQUIRE( dio.save_coord_ascii(m, file_names[6]) );

  // Load the files.
  SpMat<int> lm[7];
  std::string err;
  REQUIRE( dio.load_raw_ascii(lm[0], file_names[0], err) );
//  REQUIRE( dio.load_raw_binary(lm[1], file_names[1], err) );
//  REQUIRE( dio.load_arma_ascii(lm[2], file_names[2], err) );
//  REQUIRE( dio.load_csv_ascii(lm[3], file_names[3], err) );
  REQUIRE( dio.load_arma_binary(lm[4], file_names[4], err) );
//  REQUIRE( dio.load_pgm_binary(lm[5], file_names[5], err) );
  REQUIRE( dio.load_coord_ascii(lm[6], file_names[6], err) );

  // Now make sure all the matrices are identical.
  for (uword i = 0; i < 7; i++)
    {
    for (uword r = 0; r < 4; r++)
      {
      for (uword c = 0; c < 3; c++)
        {
        REQUIRE( m(r, c) == lm[i](r, c) );
        }
      }
    }

  for (size_t i = 0; i < 7; ++i)
    {
    remove(file_names[i].c_str());
    }
  }
*/


TEST_CASE("min_test")
  {
  SpCol<double> a(5);

  a(0) = 3.0;
  a(2) = 1.0;

  double res = min(a);
  REQUIRE( res == Approx(1e-5) );

  a(0) = -3.0;
  a(2) = -1.0;

  res = min(a);
  REQUIRE( res == Approx(-3.0) );

  a(0) = 1.3;
  a(1) = 2.4;
  a(2) = 3.1;
  a(3) = 4.4;
  a(4) = 1.4;

  res = min(a);
  REQUIRE( res == Approx(1.3) );

  SpRow<double> b(5);

  b(0) = 3.0;
  b(2) = 1.0;

  res = min(b);
  REQUIRE( res == Approx(1e-5) );

  b(0) = -3.0;
  b(2) = -1.0;

  res = min(b);
  REQUIRE( res == Approx(-3.0) );

  b(0) = 1.3;
  b(1) = 2.4;
  b(2) = 3.1;
  b(3) = 4.4;
  b(4) = 1.4;

  res = min(b);
  REQUIRE( res == Approx(1.3) );

  SpMat<double> c(6, 5);

  c(0, 0) = 1.0;
  c(1, 0) = 3.0;
  c(2, 0) = 4.0;
  c(3, 0) = 0.6;
  c(4, 0) = 1.4;
  c(5, 0) = 1.2;
  c(3, 2) = 1.3;
  c(2, 3) = -4.0;
  c(4, 3) = -1.4;
  c(5, 2) = -3.4;
  c(5, 3) = -4.1;

  SpMat<double> r = min(c, 0);
  REQUIRE( r.n_rows == 1 );
  REQUIRE( r.n_cols == 5 );
  REQUIRE( (double) r(0, 0) == Approx(0.6) );
  REQUIRE( (double) r(0, 1) == Approx(1e-5) );
  REQUIRE( (double) r(0, 2) == Approx(-3.4) );
  REQUIRE( (double) r(0, 3) == Approx(-4.1) );
  REQUIRE( (double) r(0, 4) == Approx(1e-5) );

  r = min(c, 1);
  REQUIRE( r.n_rows == 6 );
  REQUIRE( r.n_cols == 1 );
  REQUIRE( (double) r(0, 0) == Approx(1e-5) );
  REQUIRE( (double) r(1, 0) == Approx(1e-5) );
  REQUIRE( (double) r(2, 0) == Approx(-4.0) );
  REQUIRE( (double) r(3, 0) == Approx(1e-5) );
  REQUIRE( (double) r(4, 0) == Approx(-1.4) );
  REQUIRE( (double) r(5, 0) == Approx(-4.1) );
  }


TEST_CASE("max_test")
  {
  SpCol<double> a(5);

  a(0) = -3.0;
  a(2) = -1.0;

  double resa = max(a);
  REQUIRE( resa == Approx(1e-5) );

  a(0) = 3.0;
  a(2) = 1.0;

  resa = max(a);
  REQUIRE( resa == Approx(3.0) );

  a(0) = -1.3;
  a(1) = -2.4;
  a(2) = -3.1;
  a(3) = -4.4;
  a(4) = -1.4;

  resa = max(a);
  REQUIRE( resa == Approx(-1.3) );

  SpRow<double> b(5);

  b(0) = -3.0;
  b(2) = -1.0;

  resa = max(b);
  REQUIRE( resa == Approx(1e-5) );

  b(0) = 3.0;
  b(2) = 1.0;

  resa = max(b);
  REQUIRE( resa == Approx(3.0) );

  b(0) = -1.3;
  b(1) = -2.4;
  b(2) = -3.1;
  b(3) = -4.4;
  b(4) = -1.4;

  resa = max(b);
  REQUIRE( resa == Approx(-1.3) );

  SpMat<double> c(6, 5);

  c(0, 0) = 1.0;
  c(1, 0) = 3.0;
  c(2, 0) = 4.0;
  c(3, 0) = 0.6;
  c(4, 0) = -1.4;
  c(5, 0) = 1.2;
  c(3, 2) = 1.3;
  c(2, 3) = -4.0;
  c(4, 3) = -1.4;
  c(5, 2) = -3.4;
  c(5, 3) = -4.1;

  SpMat<double> res = max(c, 0);
  REQUIRE( res.n_rows == 1 );
  REQUIRE( res.n_cols == 5 );
  REQUIRE( (double) res(0, 0) == Approx(4.0) );
  REQUIRE( (double) res(0, 1) == Approx(1e-5) );
  REQUIRE( (double) res(0, 2) == Approx(1.3) );
  REQUIRE( (double) res(0, 3) == Approx(1e-5) );
  REQUIRE( (double) res(0, 4) == Approx(1e-5) );

  res = max(c, 1);
  REQUIRE( res.n_rows == 6 );
  REQUIRE( res.n_cols == 1 );
  REQUIRE( (double) res(0, 0) == Approx(1.0) );
  REQUIRE( (double) res(1, 0) == Approx(3.0) );
  REQUIRE( (double) res(2, 0) == Approx(4.0) );
  REQUIRE( (double) res(3, 0) == Approx(1.3) );
  REQUIRE( (double) res(4, 0) == Approx(1e-5) );
  REQUIRE( (double) res(5, 0) == Approx(1.2) );
  }


TEST_CASE("spmat_min_cx_test")
{
  SpCol<std::complex<double> > a(5);

  a(0) = std::complex<double>(3.0, -2.0);
  a(2) = std::complex<double>(1.0, 1.0);

  std::complex<double> res = min(a);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  a(0) = std::complex<double>(-3.0, -2.0);
  a(2) = std::complex<double>(-1.0, -1.0);

  res = min(a);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  a(0) = std::complex<double>(1.0, 0.5);
  a(1) = std::complex<double>(2.4, 1.4);
  a(2) = std::complex<double>(0.5, 0.5);
  a(3) = std::complex<double>(2.0, 2.0);
  a(4) = std::complex<double>(1.4, -1.4);

  res = min(a);
  REQUIRE( res.real() == Approx(0.5) );
  REQUIRE( res.imag() == Approx(0.5) );

  SpRow<std::complex<double> > b(5);

  b(0) = std::complex<double>(3.0, -2.0);
  b(2) = std::complex<double>(1.0, 1.0);

  res = min(b);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  b(0) = std::complex<double>(-3.0, -2.0);
  b(2) = std::complex<double>(-1.0, -1.0);

  res = min(b);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  b(0) = std::complex<double>(1.0, 0.5);
  b(1) = std::complex<double>(2.4, 1.4);
  b(2) = std::complex<double>(0.5, 0.5);
  b(3) = std::complex<double>(2.0, 2.0);
  b(4) = std::complex<double>(1.4, -1.4);

  res = min(b);
  REQUIRE( res.real() == Approx(0.5) );
  REQUIRE( res.imag() == Approx(0.5) );

  SpMat<std::complex<double> > c(4, 3);

  c(0, 0) = std::complex<double>(1.0, 2.0);
  c(0, 1) = std::complex<double>(0.5, 0.5);
  c(0, 2) = std::complex<double>(2.0, 4.0);
  c(1, 1) = std::complex<double>(-1.0, -2.0);
  c(2, 1) = std::complex<double>(-3.0, -3.0);
  c(3, 1) = std::complex<double>(0.25, 0.25);

  SpMat<std::complex<double> > r = min(c, 0);
  REQUIRE( r.n_rows == 1 );
  REQUIRE( r.n_cols == 3 );
  REQUIRE( ((std::complex<double>) r(0, 0)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(0, 0)).imag() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(0, 1)).real() == Approx(0.25) );
  REQUIRE( ((std::complex<double>) r(0, 1)).imag() == Approx(0.25) );
  REQUIRE( ((std::complex<double>) r(0, 2)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(0, 2)).imag() == Approx(1e-5) );

  r = min(c, 1);
  REQUIRE( r.n_rows == 4 );
  REQUIRE( r.n_cols == 1 );
  REQUIRE( ((std::complex<double>) r(0, 0)).real() == Approx(0.5) );
  REQUIRE( ((std::complex<double>) r(0, 0)).imag() == Approx(0.5) );
  REQUIRE( ((std::complex<double>) r(1, 0)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(1, 0)).imag() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(2, 0)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(2, 0)).imag() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(3, 0)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(3, 0)).imag() == Approx(1e-5) );
}



TEST_CASE("spmat_max_cx_test")
{
  SpCol<std::complex<double> > a(5);

  a(0) = std::complex<double>(3.0, -2.0);
  a(2) = std::complex<double>(1.0, 1.0);

  std::complex<double> res = max(a);
  REQUIRE( res.real() == Approx(3.0) );
  REQUIRE( res.imag() == Approx(-2.0) );

  a(0) = std::complex<double>(0);
  a(2) = std::complex<double>(0);

  res = max(a);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  a(0) = std::complex<double>(1.0, 0.5);
  a(1) = std::complex<double>(2.4, 1.4);
  a(2) = std::complex<double>(0.5, 0.5);
  a(3) = std::complex<double>(2.0, 2.0);
  a(4) = std::complex<double>(1.4, -1.4);

  res = max(a);
  REQUIRE( res.real() == Approx(2.0) );
  REQUIRE( res.imag() == Approx(2.0) );

  SpRow<std::complex<double> > b(5);

  b(0) = std::complex<double>(3.0, -2.0);
  b(2) = std::complex<double>(1.0, 1.0);

  res = max(b);
  REQUIRE( res.real() == Approx(3.0) );
  REQUIRE( res.imag() == Approx(-2.0) );

  b(0) = std::complex<double>(0);
  b(2) = std::complex<double>(0);

  res = max(b);
  REQUIRE( res.real() == Approx(1e-5) );
  REQUIRE( res.imag() == Approx(1e-5) );

  b(0) = std::complex<double>(1.0, 0.5);
  b(1) = std::complex<double>(2.4, 1.4);
  b(2) = std::complex<double>(0.5, 0.5);
  b(3) = std::complex<double>(2.0, 2.0);
  b(4) = std::complex<double>(1.4, -1.4);

  res = max(b);
  REQUIRE( res.real() == Approx(2.0) );
  REQUIRE( res.imag() == Approx(2.0) );

  SpMat<std::complex<double> > c(4, 3);

  c(0, 0) = std::complex<double>(1.0, 2.0);
  c(0, 1) = std::complex<double>(0.5, 0.5);
  c(1, 1) = std::complex<double>(-1.0, -2.0);
  c(2, 1) = std::complex<double>(-3.0, -3.0);
  c(3, 1) = std::complex<double>(0.25, 0.25);

  SpMat<std::complex<double> > r = max(c, 0);
  REQUIRE( r.n_rows == 1 );
  REQUIRE( r.n_cols == 3 );
  REQUIRE( ((std::complex<double>) r(0, 0)).real() == Approx(1.0) );
  REQUIRE( ((std::complex<double>) r(0, 0)).imag() == Approx(2.0) );
  REQUIRE( ((std::complex<double>) r(0, 1)).real() == Approx(-3.0) );
  REQUIRE( ((std::complex<double>) r(0, 1)).imag() == Approx(-3.0) );
  REQUIRE( ((std::complex<double>) r(0, 2)).real() == Approx(1e-5) );
  REQUIRE( ((std::complex<double>) r(0, 2)).imag() == Approx(1e-5) );

  r = max(c, 1);
  REQUIRE( r.n_rows == 4 );
  REQUIRE( r.n_cols == 1 );
  REQUIRE( ((std::complex<double>) r(0, 0)).real() == Approx(1.0) );
  REQUIRE( ((std::complex<double>) r(0, 0)).imag() == Approx(2.0) );
  REQUIRE( ((std::complex<double>) r(1, 0)).real() == Approx(-1.0) );
  REQUIRE( ((std::complex<double>) r(1, 0)).imag() == Approx(-2.0) );
  REQUIRE( ((std::complex<double>) r(2, 0)).real() == Approx(-3.0) );
  REQUIRE( ((std::complex<double>) r(2, 0)).imag() == Approx(-3.0) );
  REQUIRE( ((std::complex<double>) r(3, 0)).real() == Approx(0.25) );
  REQUIRE( ((std::complex<double>) r(3, 0)).imag() == Approx(0.25) );
  }



TEST_CASE("spmat_complex_constructor_test")
  {
  // First make two sparse matrices.
  SpMat<double> a(8, 10);
  SpMat<double> b(8, 10);

  a(0, 0) = 4;
  a(4, 2) = 5;
  a(5, 3) = 6;
  a(6, 3) = 7;
  a(1, 4) = 1;
  a(5, 4) = 6;
  a(7, 6) = 3;
  a(0, 7) = 2;
  a(3, 7) = 3;

  b(0, 0) = 4;
  b(4, 2) = 5;
  b(7, 3) = 4;
  b(1, 4) = 1;
  b(3, 4) = 6;
  b(5, 4) = -1;
  b(6, 4) = 2;
  b(7, 4) = 3;
  b(6, 5) = 2;
  b(6, 6) = 3;
  b(3, 7) = 4;
  b(6, 7) = 5;

  SpMat<std::complex<double> > c(a, b);

  REQUIRE( c.n_nonzero == 16 );
  REQUIRE( (std::complex<double>) c(0, 0) == std::complex<double>(4, 4) );
  REQUIRE( (std::complex<double>) c(4, 2) == std::complex<double>(5, 5) );
  REQUIRE( (std::complex<double>) c(5, 3) == std::complex<double>(6, 0) );
  REQUIRE( (std::complex<double>) c(6, 3) == std::complex<double>(7, 0) );
  REQUIRE( (std::complex<double>) c(7, 3) == std::complex<double>(0, 4) );
  REQUIRE( (std::complex<double>) c(1, 4) == std::complex<double>(1, 1) );
  REQUIRE( (std::complex<double>) c(3, 4) == std::complex<double>(0, 6) );
  REQUIRE( (std::complex<double>) c(5, 4) == std::complex<double>(6, -1) );
  REQUIRE( (std::complex<double>) c(6, 4) == std::complex<double>(0, 2) );
  REQUIRE( (std::complex<double>) c(7, 4) == std::complex<double>(0, 3) );
  REQUIRE( (std::complex<double>) c(6, 5) == std::complex<double>(0, 2) );
  REQUIRE( (std::complex<double>) c(6, 6) == std::complex<double>(0, 3) );
  REQUIRE( (std::complex<double>) c(7, 6) == std::complex<double>(3, 0) );
  REQUIRE( (std::complex<double>) c(0, 7) == std::complex<double>(2, 0) );
  REQUIRE( (std::complex<double>) c(3, 7) == std::complex<double>(3, 4) );
  REQUIRE( (std::complex<double>) c(6, 7) == std::complex<double>(0, 5) );
  }



TEST_CASE("spmat_unary_operators_test")
  {
  SpMat<int> a(3, 3);
  SpMat<int> b(3, 3);

  a(0, 0) = 1;
  a(1, 2) = 4;
  a(2, 2) = 5;

  b(0, 1) = 1;
  b(1, 0) = 2;
  b(1, 2) = -4;
  b(2, 2) = 5;

  SpMat<int> c = a + b;

  REQUIRE( c.n_nonzero == 4 );

  REQUIRE( (double) c(0, 0) == 1 );
  REQUIRE( (double) c(1, 0) == 2 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 1 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == 0 );
  REQUIRE( (double) c(2, 2) == 10 );

  c = a - b;

  REQUIRE( c.n_nonzero == 4 );

  REQUIRE( (double) c(0, 0) == 1 );
  REQUIRE( (double) c(1, 0) == -2 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == -1 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == 8 );
  REQUIRE( (double) c(2, 2) == 0 );

  c = a % b;

  REQUIRE( c.n_nonzero == 2 );

  REQUIRE( (double) c(0, 0) == 0 );
  REQUIRE( (double) c(1, 0) == 0 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 0 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == -16 );
  REQUIRE( (double) c(2, 2) == 25 );

  a(0, 0) = 4;
  b(0, 0) = 2;
/*
  c = a / b;

  REQUIRE( c.n_nonzero == 3 );

  REQUIRE( (double) c(0, 0) == 2 );
  REQUIRE( (double) c(1, 0) == 0 );
  REQUIRE( (double) c(2, 0) == 0 );
  REQUIRE( (double) c(0, 1) == 0 );
  REQUIRE( (double) c(1, 1) == 0 );
  REQUIRE( (double) c(2, 1) == 0 );
  REQUIRE( (double) c(0, 2) == 0 );
  REQUIRE( (double) c(1, 2) == -1 );
  REQUIRE( (double) c(2, 2) == 1 );
*/
  }



TEST_CASE("spmat_unary_val_operators_test")
  {
  SpMat<double> a(2, 2);

  a(0, 0) = 2.0;
  a(1, 1) = -3.0;

  SpMat<double> b = a * 3.0;

  REQUIRE( b.n_nonzero == 2 );
  REQUIRE( (double) b(0, 0) == Approx(6.0) );
  REQUIRE( (double) b(0, 1) == Approx(1e-5) );
  REQUIRE( (double) b(1, 0) == Approx(1e-5) );
  REQUIRE( (double) b(1, 1) == Approx(-9.0) );

  b = a / 3.0;

  REQUIRE( b.n_nonzero == 2 );
  REQUIRE( (double) b(0, 0) == Approx(2.0 / 3.0) );
  REQUIRE( (double) b(0, 1) == Approx(1e-5) );
  REQUIRE( (double) b(1, 0) == Approx(1e-5) );
  REQUIRE( (double) b(1, 1) == Approx(-1.0) );
  }


TEST_CASE("spmat_sparse_unary_multiplication_test")
  {
  SpMat<double> spaa(10, 10);
  spaa(1, 5) = 0.4;
  spaa(0, 4) = 0.3;
  spaa(0, 8) = 1.2;
  spaa(3, 0) = 1.1;
  spaa(3, 1) = 1.1;
  spaa(3, 2) = 1.1;
  spaa(4, 4) = 0.2;
  spaa(4, 9) = 0.1;
  spaa(6, 2) = 4.1;
  spaa(6, 8) = 4.1;
  spaa(7, 5) = 1.0;
  spaa(8, 9) = 0.4;
  spaa(9, 4) = 0.4;

  double correctResultB[10][10] =
    {{ 0.00, 0.00, 0.00, 0.00, 0.06, 0.00, 0.00, 0.00, 0.00, 0.51 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.33, 0.44, 0.00, 0.00, 1.32, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.08, 0.00, 0.00, 0.00, 0.00, 0.02 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.64 },
     { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.16, 0.00, 0.00, 0.00, 0.00, 0.00 },
     { 0.00, 0.00, 0.00, 0.00, 0.08, 0.00, 0.00, 0.00, 0.00, 0.04 }};

  SpMat<double> spab = spaa * spaa;

  for (uword i = 0; i < 10; i++)
    {
    for (uword j = 0; j < 10; j++)
      {
      REQUIRE( (double) spab(i, j) == Approx(correctResultB[i][j]) );
      }
    }

  SpMat<double> spac(15, 15);
  spac(6, 10) = 0.4;
  spac(5, 9) = 0.3;
  spac(5, 13) = 1.2;
  spac(8, 5) = 1.1;
  spac(8, 6) = 1.1;
  spac(8, 7) = 1.1;
  spac(9, 9) = 0.2;
  spac(9, 14) = 0.1;
  spac(11, 7) = 4.1;
  spac(11, 13) = 4.1;
  spac(12, 10) = 1.0;
  spac(13, 14) = 0.4;
  spac(14, 9) = 0.4;

  spab = spaa * spac.submat(5, 5, 14, 14);

  for (uword i = 0; i < 10; i++)
    {
    for (uword j = 0; j < 10; j++)
      {
      REQUIRE( (double) spab(i, j) == Approx(correctResultB[i][j]) );
      }
    }
  }



TEST_CASE("spmat_unary_operator_test_2")
  {
  SpMat<double> a(3, 3);
  a(0, 0) = 1;
  a(0, 2) = 3.5;
  a(1, 2) = 4.0;
  a(2, 2) = -3.0;

  mat b(3, 3);
  b.fill(3.0);

  mat c = a + b;

  REQUIRE( c(0, 0) == Approx(4.0) );
  REQUIRE( c(1, 0) == Approx(3.0) );
  REQUIRE( c(2, 0) == Approx(3.0) );
  REQUIRE( c(0, 1) == Approx(3.0) );
  REQUIRE( c(1, 1) == Approx(3.0) );
  REQUIRE( c(2, 1) == Approx(3.0) );
  REQUIRE( c(0, 2) == Approx(6.5) );
  REQUIRE( c(1, 2) == Approx(7.0) );
  REQUIRE( c(2, 2) == Approx(1e-5) );

  c = a - b;

  REQUIRE( c(0, 0) == Approx(-2.0) );
  REQUIRE( c(1, 0) == Approx(-3.0) );
  REQUIRE( c(2, 0) == Approx(-3.0) );
  REQUIRE( c(0, 1) == Approx(-3.0) );
  REQUIRE( c(1, 1) == Approx(-3.0) );
  REQUIRE( c(2, 1) == Approx(-3.0) );
  REQUIRE( c(0, 2) == Approx(0.5) );
  REQUIRE( c(1, 2) == Approx(1.0) );
  REQUIRE( c(2, 2) == Approx(-6.0) );

  SpMat<double> d = a % b;

  REQUIRE( d.n_nonzero == 4 );
  REQUIRE( (double) d(0, 0) == Approx(3.0) );
  REQUIRE( (double) d(1, 0) == Approx(1e-5) );
  REQUIRE( (double) d(2, 0) == Approx(1e-5) );
  REQUIRE( (double) d(0, 1) == Approx(1e-5) );
  REQUIRE( (double) d(1, 1) == Approx(1e-5) );
  REQUIRE( (double) d(2, 1) == Approx(1e-5) );
  REQUIRE( (double) d(0, 2) == Approx(10.5) );
  REQUIRE( (double) d(1, 2) == Approx(12.0) );
  REQUIRE( (double) d(2, 2) == Approx(-9.0) );

  d = a / b;

  REQUIRE( d.n_nonzero == 4 );
  REQUIRE( (double) d(0, 0) == Approx((1.0 / 3.0)) );
  REQUIRE( (double) d(1, 0) == Approx(1e-5) );
  REQUIRE( (double) d(2, 0) == Approx(1e-5) );
  REQUIRE( (double) d(0, 1) == Approx(1e-5) );
  REQUIRE( (double) d(1, 1) == Approx(1e-5) );
  REQUIRE( (double) d(2, 1) == Approx(1e-5) );
  REQUIRE( (double) d(0, 2) == Approx((3.5 / 3.0)) );
  REQUIRE( (double) d(1, 2) == Approx((4.0 / 3.0)) );
  REQUIRE( (double) d(2, 2) == Approx(-1.0) );

  c = a * b;

  REQUIRE( (double) c(0, 0) == Approx(13.5) );
  REQUIRE( (double) c(1, 0) == Approx(12.0) );
  REQUIRE( (double) c(2, 0) == Approx(-9.0) );
  REQUIRE( (double) c(0, 1) == Approx(13.5) );
  REQUIRE( (double) c(1, 1) == Approx(12.0) );
  REQUIRE( (double) c(2, 1) == Approx(-9.0) );
  REQUIRE( (double) c(0, 2) == Approx(13.5) );
  REQUIRE( (double) c(1, 2) == Approx(12.0) );
  REQUIRE( (double) c(2, 2) == Approx(-9.0) );

  c = b * a;

  REQUIRE( (double) c(0, 0) == Approx(3.0) );
  REQUIRE( (double) c(1, 0) == Approx(3.0) );
  REQUIRE( (double) c(2, 0) == Approx(3.0) );
  REQUIRE( (double) c(0, 1) == Approx(1e-5) );
  REQUIRE( (double) c(1, 1) == Approx(1e-5) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(13.5) );
  REQUIRE( (double) c(1, 2) == Approx(13.5) );
  REQUIRE( (double) c(2, 2) == Approx(13.5) );
  }



TEST_CASE("spmat_mat_operator_tests")
  {
  SpMat<double> a(3, 3);
  a(0, 0) = 2.0;
  a(1, 2) = 3.5;
  a(2, 1) = -2.0;
  a(2, 2) = 4.5;

  mat b(3, 3);
  b.fill(2.0);

  mat c(b);

  c += a;

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.5) );
  REQUIRE( (double) c(2, 2) == Approx(6.5) );

  c = b + a;

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(1e-5) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(5.5) );
  REQUIRE( (double) c(2, 2) == Approx(6.5) );

  c = b;
  c -= a;

  REQUIRE( (double) c(0, 0) == Approx(1e-5) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(4.0) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(-1.5) );
  REQUIRE( (double) c(2, 2) == Approx(-2.5) );

  c = b - a;

  REQUIRE( (double) c(0, 0) == Approx(1e-5) );
  REQUIRE( (double) c(1, 0) == Approx(2.0) );
  REQUIRE( (double) c(2, 0) == Approx(2.0) );
  REQUIRE( (double) c(0, 1) == Approx(2.0) );
  REQUIRE( (double) c(1, 1) == Approx(2.0) );
  REQUIRE( (double) c(2, 1) == Approx(4.0) );
  REQUIRE( (double) c(0, 2) == Approx(2.0) );
  REQUIRE( (double) c(1, 2) == Approx(-1.5) );
  REQUIRE( (double) c(2, 2) == Approx(-2.5) );

  c = b;
  c *= a;

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 0) == Approx(4.0) );
  REQUIRE( (double) c(0, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 1) == Approx(-4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(0, 2) == Approx(16.0) );
  REQUIRE( (double) c(1, 2) == Approx(16.0) );
  REQUIRE( (double) c(2, 2) == Approx(16.0) );

  mat e = b * a;

  REQUIRE( (double) e(0, 0) == Approx(4.0) );
  REQUIRE( (double) e(1, 0) == Approx(4.0) );
  REQUIRE( (double) e(2, 0) == Approx(4.0) );
  REQUIRE( (double) e(0, 1) == Approx(-4.0) );
  REQUIRE( (double) e(1, 1) == Approx(-4.0) );
  REQUIRE( (double) e(2, 1) == Approx(-4.0) );
  REQUIRE( (double) e(0, 2) == Approx(16.0) );
  REQUIRE( (double) e(1, 2) == Approx(16.0) );
  REQUIRE( (double) e(2, 2) == Approx(16.0) );

  c = b;
  c %= a;

  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(1, 0) == Approx(1e-5) );
  REQUIRE( (double) c(2, 0) == Approx(1e-5) );
  REQUIRE( (double) c(0, 1) == Approx(1e-5) );
  REQUIRE( (double) c(1, 1) == Approx(1e-5) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(0, 2) == Approx(1e-5) );
  REQUIRE( (double) c(1, 2) == Approx(7.0) );
  REQUIRE( (double) c(2, 2) == Approx(9.0) );

  SpMat<double> d = b % a;

  REQUIRE( d.n_nonzero == 4 );
  REQUIRE( (double) c(0, 0) == Approx(4.0) );
  REQUIRE( (double) c(2, 1) == Approx(-4.0) );
  REQUIRE( (double) c(1, 2) == Approx(7.0) );
  REQUIRE( (double) c(2, 2) == Approx(9.0) );

  c = b;
  c /= a;

  REQUIRE( c(0, 0) == Approx(1.0) );
  REQUIRE( std::isinf(c(1, 0)) );
  REQUIRE( std::isinf(c(2, 0)) );
  REQUIRE( std::isinf(c(0, 1)) );
  REQUIRE( std::isinf(c(1, 1)) );
  REQUIRE( c(2, 1) == Approx(-1.0) );
  REQUIRE( std::isinf(c(0, 2)) );
  REQUIRE( c(1, 2) == Approx(2.0 / 3.5) );
  REQUIRE( c(2, 2) == Approx(2.0 / 4.5) );
  }


TEST_CASE("spmat_empty_hadamard")
  {
  SpMat<double> x(5, 5), y(5, 5), z;

  z = x % y;

  REQUIRE( z.n_nonzero == 0 );
  REQUIRE( z.n_rows == 5 );
  REQUIRE( z.n_cols == 5 );
  }



TEST_CASE("spmat_sparse_dense_in_place")
  {
  SpMat<double> a;
  a.sprandu(50, 50, 0.1);
  mat b;
  b.randu(50, 50);
  mat d( a);

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) a(r, c) != 0)
        REQUIRE( (double) a(r, c) == Approx(d(r, c)) );
      else
        REQUIRE( d(r, c) == Approx(1e-5) );
      }
    }

  SpMat<double> x;
  mat y;

  x = a;
  y = d;

  x *= b;
  y *= b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) a(r, c) != 0)
        REQUIRE( (double) a(r, c) == Approx(d(r, c)) );
      else
        REQUIRE( d(r, c) == Approx(1e-5) );
      }
    }

  x = a;
  y = d;

  x /= b;
  y /= b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) a(r, c) != 0)
        REQUIRE( (double) a(r, c) == Approx(d(r, c)) );
      else
        REQUIRE( d(r, c) == Approx(1e-5) );
      }
    }

  x = a;
  y = d;

  x %= b;
  y %= b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) a(r, c) != 0)
        REQUIRE( (double) a(r, c) == Approx(d(r, c)) );
      else
        REQUIRE(d(r, c) == Approx(1e-5) );
      }
    }
  }



TEST_CASE("spmat_sparse_dense_not_in_place")
  {
  SpMat<double> a;
  a.sprandu(50, 50, 0.1);
  mat b;
  b.randu(50, 50);
  mat d(a);

  SpMat<double> x;
  mat y;
  mat z;

  y = a + b;
  z = d + b;

  for (uword c = 0; c < 50; ++c)
    {
    for(uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = a - b;
  z = d - b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = a * b;
  z = d * b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = a % b;
  z = d % b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = a / b;
  z = d / b;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = b + a;
  z = b + d;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
    }
  }

  y = b - a;
  z = b - d;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = b * a;
  z = b * d;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }

  y = b % a;
  z = b % d;

  for (uword c = 0; c < 50; ++c)
    {
    for (uword r = 0; r < 50; ++r)
      {
      if ((double) y(r, c) != 0)
        REQUIRE( (double) y(r, c) == Approx(z(r, c)) );
      else
        REQUIRE( z(r, c) == Approx(1e-5) );
      }
    }
  }



TEST_CASE("spmat_batch_insert_test")
  {
  Mat<uword> locations(2, 5);
  locations(1, 0) = 1;
  locations(0, 0) = 2;
  locations(1, 1) = 1;
  locations(0, 1) = 7;
  locations(1, 2) = 4;
  locations(0, 2) = 0;
  locations(1, 3) = 4;
  locations(0, 3) = 9;
  locations(1, 4) = 5;
  locations(0, 4) = 0;

  Col<double> values(5);
  values[0] = 1.5;
  values[1] = -15.15;
  values[2] = 2.2;
  values[3] = 3.0;
  values[4] = 5.0;

  SpMat<double> m(locations, values, 10, 10, true);

  REQUIRE( m.n_nonzero == 5 );
  REQUIRE( m.n_rows == 10 );
  REQUIRE( m.n_cols == 10 );
  REQUIRE( (double) m(2, 1) == Approx(1.5) );
  REQUIRE( (double) m(7, 1) == Approx(-15.15) );
  REQUIRE( (double) m(0, 4) == Approx(2.2) );
  REQUIRE( (double) m(9, 4) == Approx(3.0) );
  REQUIRE( (double) m(0, 5) == Approx(5.0) );
  REQUIRE( m.col_ptrs[11] == std::numeric_limits<uword>::max() );

  // Auto size detection.
  SpMat<double> n(locations, values, true);

  REQUIRE( n.n_nonzero == 5 );
  REQUIRE( n.n_rows == 10 );
  REQUIRE( n.n_cols == 6 );
  REQUIRE( (double) n(2, 1) == Approx(1.5) );
  REQUIRE( (double) n(7, 1) == Approx(-15.15) );
  REQUIRE( (double) n(0, 4) == Approx(2.2) );
  REQUIRE( (double) n(9, 4) == Approx(3.0) );
  REQUIRE( (double) n(0, 5) == Approx(5.0) );
  REQUIRE( n.col_ptrs[7] == std::numeric_limits<uword>::max() );
  }



TEST_CASE("spmat_batch_insert_unsorted_test")
  {
  Mat<uword> locations(2, 5);
  locations(1, 0) = 4;
  locations(0, 0) = 0;
  locations(1, 1) = 1;
  locations(0, 1) = 2;
  locations(1, 2) = 4;
  locations(0, 2) = 9;
  locations(1, 3) = 5;
  locations(0, 3) = 0;
  locations(1, 4) = 1;
  locations(0, 4) = 7;

  Col<double> values(5);
  values[1] = 1.5;
  values[4] = -15.15;
  values[0] = 2.2;
  values[2] = 3.0;
  values[3] = 5.0;

  SpMat<double> m(locations, values, 10, 10, true);

  REQUIRE( m.n_nonzero == 5 );
  REQUIRE( m.n_rows == 10 );
  REQUIRE( m.n_cols == 10 );
  REQUIRE( (double) m(2, 1) == Approx(1.5) );
  REQUIRE( (double) m(7, 1) == Approx(-15.15) );
  REQUIRE( (double) m(0, 4) == Approx(2.2) );
  REQUIRE( (double) m(9, 4) == Approx(3.0) );
  REQUIRE( (double) m(0, 5) == Approx(5.0) );

  // Auto size detection.
  SpMat<double> n(locations, values, true);

  REQUIRE( n.n_nonzero == 5 );
  REQUIRE( n.n_rows == 10 );
  REQUIRE( n.n_cols == 6 );
  REQUIRE( (double) n(2, 1) == Approx(1.5) );
  REQUIRE( (double) n(7, 1) == Approx(-15.15) );
  REQUIRE( (double) n(0, 4) == Approx(2.2) );
  REQUIRE( (double) n(9, 4) == Approx(3.0) );
  REQUIRE( (double) n(0, 5) == Approx(5.0) );
  }



TEST_CASE("spmat_batch_insert_empty_test")
  {
  Mat<uword> locations(2, 0);
  Col<double> values;

  SpMat<double> m(locations, values, 10, 10, false);

  REQUIRE( m.n_nonzero == 0 );
  REQUIRE( m.n_rows == 10 );
  REQUIRE( m.n_cols == 10 );
  REQUIRE( m.col_ptrs[11] == std::numeric_limits<uword>::max() );

  SpMat<double> n(locations, values, false);

  REQUIRE( n.n_nonzero == 0 );
  REQUIRE( n.n_rows == 0 );
  REQUIRE( n.n_cols == 0 );
  REQUIRE( n.col_ptrs[1] == std::numeric_limits<uword>::max() );

  SpMat<double> o(locations, values, 10, 10, true);

  REQUIRE( o.n_nonzero == 0 );
  REQUIRE( o.n_rows == 10 );
  REQUIRE( o.n_cols == 10 );
  REQUIRE( o.col_ptrs[11] == std::numeric_limits<uword>::max() );

  SpMat<double> p(locations, values, true);

  REQUIRE( p.n_nonzero == 0 );
  REQUIRE( p.n_rows == 0 );
  REQUIRE( p.n_cols == 0 );
  REQUIRE( p.col_ptrs[1] == std::numeric_limits<uword>::max() );
  }


// Make sure a matrix is the same as the other one.
template<typename T1, typename T2>
void CheckMatrices(const T1& a, const T2& b)
{
  REQUIRE( a.n_rows == b.n_rows );
  REQUIRE( a.n_cols == b.n_cols );
  for (uword i = 0; i < a.n_elem; ++i)
    REQUIRE( (double) a[i] == Approx((double) b[i]) );
}

// Test the constructor written by Dirk.
TEST_CASE("spmat_dirk_constructor_test")
  {
  // Come up with some values and stuff.
  vec values = "4.0 2.0 1.0 3.2 1.2 3.5";
  Col<uword> row_indices = "1 3 1 2 4 5";
  Col<uword> col_ptrs = "0 2 2 3 4 6";

  // Ok, now make a matrix.
  sp_mat M(row_indices, col_ptrs, values, 6, 5);

  // Make the equivalent dense matrix.
  mat D(6, 5);
  D.fill(0);
  D(1, 0) = 4.0;
  D(3, 0) = 2.0;
  D(1, 2) = 1.0;
  D(2, 3) = 3.2;
  D(4, 4) = 1.2;
  D(5, 4) = 3.5;

  // So now let's just do a bunch of operations and make sure everything is the
  // same.
  sp_mat dm = M * M.t();
  mat dd = D * D.t();

  CheckMatrices(dm, dd);

  dm = M.t() * M;
  dd = D.t() * D;

  CheckMatrices(dm, dd);

  sp_mat am = M + M;
  mat ad = D + D;

  CheckMatrices(am, ad);

  dm = M + D;
  ad = D + M;

  CheckMatrices(dm, ad);
  }



TEST_CASE("spmat_clear_test")
  {
  sp_mat x;
  x.sprandu(10, 10, 0.6);

  x.clear();

  REQUIRE( x.n_cols == 0 );
  REQUIRE( x.n_rows == 0 );
  REQUIRE( x.n_nonzero == 0 );
  }



TEST_CASE("spmat_batch_insert_zeroes_test")
  {
  Mat<uword> locations(2, 5);
  locations(1, 0) = 1;
  locations(0, 0) = 2;
  locations(1, 1) = 1;
  locations(0, 1) = 7;
  locations(1, 2) = 4;
  locations(0, 2) = 0;
  locations(1, 3) = 4;
  locations(0, 3) = 9;
  locations(1, 4) = 5;
  locations(0, 4) = 0;

  Col<double> values(5);
  values[0] = 1.5;
  values[1] = -15.15;
  values[2] = 2.2;
  values[3] = 0.0;
  values[4] = 5.0;

  SpMat<double> m(locations, values, 10, 10, false, true);

  REQUIRE( m.n_nonzero == 4 );
  REQUIRE( m.n_rows == 10 );
  REQUIRE( m.n_cols == 10 );
  REQUIRE( (double) m(2, 1) == Approx(1.5) );
  REQUIRE( (double) m(7, 1) == Approx(-15.15) );
  REQUIRE( (double) m(0, 4) == Approx(2.2) );
  REQUIRE( (double) m(9, 4) == Approx(1e-5) );
  REQUIRE( (double) m(0, 5) == Approx(5.0) );

  // Auto size detection.
  SpMat<double> n(locations, values, false);

  REQUIRE( n.n_nonzero == 4 );
  REQUIRE( n.n_rows == 10 );
  REQUIRE( n.n_cols == 6 );
  REQUIRE( (double) n(2, 1) == Approx(1.5) );
  REQUIRE( (double) n(7, 1) == Approx(-15.15) );
  REQUIRE( (double) n(0, 4) == Approx(2.2) );
  REQUIRE( (double) n(9, 4) == Approx(1e-5) );
  REQUIRE( (double) n(0, 5) == Approx(5.0) );
  }



TEST_CASE("spmat_batch_insert_unsorted_case_zeroes")
  {
  Mat<uword> locations(2, 5);
  locations(1, 0) = 4;
  locations(0, 0) = 0;
  locations(1, 1) = 1;
  locations(0, 1) = 2;
  locations(1, 2) = 4;
  locations(0, 2) = 9;
  locations(1, 3) = 5;
  locations(0, 3) = 0;
  locations(1, 4) = 1;
  locations(0, 4) = 7;

  Col<double> values(5);
  values[1] = 1.5;
  values[4] = -15.15;
  values[0] = 2.2;
  values[2] = 0.0;
  values[3] = 5.0;

  SpMat<double> m(locations, values, 10, 10, true);

  REQUIRE( m.n_nonzero == 4 );
  REQUIRE( m.n_rows == 10 );
  REQUIRE( m.n_cols == 10 );
  REQUIRE( (double) m(2, 1) == Approx(1.5) );
  REQUIRE( (double) m(7, 1) == Approx(-15.15) );
  REQUIRE( (double) m(0, 4) == Approx(2.2) );
  REQUIRE( (double) m(9, 4) == Approx(1e-5) );
  REQUIRE( (double) m(0, 5) == Approx(5.0) );
  REQUIRE( m.col_ptrs[11] == std::numeric_limits<uword>::max() );

  // Auto size detection.
  SpMat<double> n(locations, values, true);

  REQUIRE( n.n_nonzero == 4 );
  REQUIRE( n.n_rows == 10 );
  REQUIRE( n.n_cols == 6 );
  REQUIRE( (double) n(2, 1) == Approx(1.5) );
  REQUIRE( (double) n(7, 1) == Approx(-15.15) );
  REQUIRE( (double) n(0, 4) == Approx(2.2) );
  REQUIRE( (double) n(9, 4) == Approx(1e-5) );
  REQUIRE( (double) n(0, 5) == Approx(5.0) );
  REQUIRE( n.col_ptrs[7] == std::numeric_limits<uword>::max() );
  }



TEST_CASE("spmat_const_row_col_iterator_test")
  {
  mat X;
  X.zeros(5, 5);
  for (uword i = 0; i < 5; ++i)
    {
    X.col(i) += i;
    }

  for (uword i = 0; i < 5; ++i)
    {
    X.row(i) += 3 * i;
    }

  // Make sure default constructor works okay.
  mat::const_row_col_iterator it;
  // Make sure ++ operator, operator* and comparison operators work fine.
  size_t count = 0;
  for (it = X.begin_row_col(); it != X.end_row_col(); it++)
    {
    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );

    count++;
    }
  REQUIRE( count == 25 );
  it = X.end_row_col();
  do
    {
    it--;
    count--;

    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );
    } while (it != X.begin_row_col());

  REQUIRE( count == 0 );
  }



TEST_CASE("spmat_row_col_iterator_test")
  {
  mat X;
  X.zeros(5, 5);
  for (size_t i = 0; i < 5; ++i)
    {
    X.col(i) += i;
    }

  for (size_t i = 0; i < 5; ++i)
    {
    X.row(i) += 3 * i;
    }

  // Make sure default constructor works okay.
  mat::row_col_iterator it;
  // Make sure ++ operator, operator* and comparison operators work fine.
  size_t count = 0;
  for (it = X.begin_row_col(); it != X.end_row_col(); it++)
    {
    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );

    count++;
    }
  REQUIRE( count == 25 );
  it = X.end_row_col();
  do
    {
    it--;
    count--;

    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );
    } while (it != X.begin_row_col());

  REQUIRE( count == 0 );
  }



TEST_CASE("spmat_const_sprow_col_iterator_test")
  {
  sp_mat X(5, 5);
  for (size_t i = 0; i < 5; ++i)
    {
    X.col(i) += i;
    }

  for (size_t i = 0; i < 5; ++i)
    {
    X.row(i) += 3 * i;
    }

  // Make sure default constructor works okay.
  sp_mat::const_row_col_iterator it;
  // Make sure ++ operator, operator* and comparison operators work fine.
  size_t count = 1;
  for (it = X.begin_row_col(); it != X.end_row_col(); it++)
    {
    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );

    count++;
    }
  REQUIRE( count == 25 );
  it = X.end_row_col();
  do
    {
    it--;
    count--;

    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );
    } while (it != X.begin_row_col());

  REQUIRE( count == 1 );
  }



TEST_CASE("spmat_sprow_col_iterator_test")
  {
  sp_mat X(5, 5);
  for (size_t i = 0; i < 5; ++i)
    {
    X.col(i) += i;
    }

  for (size_t i = 0; i < 5; ++i)
    {
    X.row(i) += 3 * i;
    }

  // Make sure default constructor works okay.
  sp_mat::row_col_iterator it;
  // Make sure ++ operator, operator* and comparison operators work fine.
  size_t count = 1;
  for (it = X.begin_row_col(); it != X.end_row_col(); it++)
    {
    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );

    count++;
    }
  REQUIRE( count == 25 );
  it = X.end_row_col();
  do
    {
    it--;
    count--;

    // Check iterator value.
    REQUIRE( *it == (count % 5) * 3 + (count / 5) );

    // Check iterator position.
    REQUIRE( it.row() == count % 5 );
    REQUIRE( it.col() == count / 5 );
    } while (it != X.begin_row_col());

  REQUIRE( count == 1 );
  }
