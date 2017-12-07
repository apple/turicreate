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


TEST_CASE("bounds_1")
  {
  const uword n_rows = 5;
  const uword n_cols = 6;

  mat A(n_rows, n_cols, fill::zeros);

  REQUIRE_NOTHROW( A(n_rows-1,n_cols-1) = 0 );

  // out of bounds access will throw unless ARMA_NO_DEBUG is defined
  REQUIRE_THROWS( A(n_rows,n_cols) = 0 );
  }
