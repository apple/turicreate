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


TEST_CASE("fn_find_unique_1")
  {
  mat A =
    {
    {  1,  3,  5,  6,  7 },
    {  2,  4,  5,  7,  8 },
    {  3,  5,  5,  6,  9 },
    };

  uvec indices = find_unique(A);

  uvec indices2 = { 0, 1, 2, 4, 5, 9, 10, 13, 14 };

  REQUIRE( indices.n_elem == indices2.n_elem );

  bool same = true;

  for(uword i=0; i < indices.n_elem; ++i)
    {
    if(indices(i) != indices2(i))  { same = false; break; }
    }

  REQUIRE( same == true );

  vec unique_elem = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  REQUIRE( accu(abs( A.elem(indices) - unique_elem )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }



TEST_CASE("fn_find_unique_2")
  {
  cx_mat A =
    {
    { cx_double(1,-1), cx_double(3, 2), cx_double(5, 2), cx_double(6, 1), cx_double(7,-1) },
    { cx_double(2, 1), cx_double(4, 4), cx_double(5, 2), cx_double(7,-1), cx_double(8, 1) },
    { cx_double(3, 2), cx_double(5, 1), cx_double(5, 3), cx_double(6, 1), cx_double(9,-9) }
    };

  uvec indices = find_unique(A);

  uvec indices2 = { 0, 1, 2, 4, 5, 6, 8, 9, 10, 13, 14 };

  REQUIRE( indices.n_elem == indices2.n_elem );

  bool same = true;

  for(uword i=0; i < indices.n_elem; ++i)
    {
    if(indices(i) != indices2(i))  { same = false; break; }
    }

  REQUIRE( same == true );

  cx_vec unique_elem =
    {
    cx_double(1,-1),
    cx_double(2, 1),
    cx_double(3, 2),
    cx_double(4, 4),
    cx_double(5, 1),
    cx_double(5, 2),
    cx_double(5, 3),
    cx_double(6, 1),
    cx_double(7,-1),
    cx_double(8, 1),
    cx_double(9,-9)
    };

  REQUIRE( accu(abs( A.elem(indices) - unique_elem )) == Approx(0.0) );

  // REQUIRE_THROWS(  );
  }
