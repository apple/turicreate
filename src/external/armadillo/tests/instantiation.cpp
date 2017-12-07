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


TEST_CASE("instantiation_mat_1")
  {
  const uword n_rows = 5;
  const uword n_cols = 6;

   mat A(n_rows,n_cols);
  fmat B(n_rows,n_cols);
  umat C(n_rows,n_cols);
  imat D(n_rows,n_cols);

  cx_mat  E(n_rows,n_cols);
  cx_fmat F(n_rows,n_cols);

  mat::fixed<5,6> G;
  }


// TODO: colvec_instantiation
// TODO: rowvec_instantiation


TEST_CASE("instantiation_cube_1")
  {
  const uword n_rows   = 5;
  const uword n_cols   = 6;
  const uword n_slices = 2;

   cube A(n_rows,n_cols,n_slices);
  fcube B(n_rows,n_cols,n_slices);
  ucube C(n_rows,n_cols,n_slices);
  icube D(n_rows,n_cols,n_slices);

  cx_cube  E(n_rows,n_cols,n_slices);
  cx_fcube F(n_rows,n_cols,n_slices);

  cube::fixed<5,6,2> G;
  }


// TODO: field_instantiation 1D 2D 3D
// TODO: spmat_instantiation
