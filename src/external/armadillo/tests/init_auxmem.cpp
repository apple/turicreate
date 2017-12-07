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


TEST_CASE("init_auxmem_1")
  {
  double data[] = { 1, 2, 3, 4, 5, 6 };

  mat A(data, 2, 3);
  mat B(data, 2, 3, false);
  mat C(data, 2, 3, false, true);

  REQUIRE( A(0,0) == double(1) );
  REQUIRE( A(1,0) == double(2) );

  REQUIRE( A(0,1) == double(3) );
  REQUIRE( A(1,1) == double(4) );

  REQUIRE( A(0,2) == double(5) );
  REQUIRE( A(1,2) == double(6) );

  A(0,0) = 123.0;  REQUIRE( data[0] == 1 );

  B(0,0) = 123.0;  REQUIRE( data[0] == 123.0 );

  REQUIRE_THROWS( C.set_size(5,6) );
  }
