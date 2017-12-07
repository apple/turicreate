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


TEST_CASE("fn_diagmat_1")
  {
  mat A =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  mat Ap1 =
    {
    { -0.0    ,  0.69298,  0.0    ,  0.0     },
    {  0.0    ,  0.0    ,  0.78987,  0.0     },
    {  0.0    ,  0.0    ,  0.0    ,  0.40163 }
    };

  mat Amain =
    {
    { -0.78838,  0.0    ,  0.0    ,  0.0     },
    {  0.0    , -0.12020,  0.0    ,  0.0     },
    {  0.0    ,  0.0    , -0.22263,  0.0     }
    };

  mat Am1 =
    {
    {  0.0    ,  0.0    ,  0.0    ,  0.0     },
    {  0.49345,  0.0    ,  0.0    ,  0.0     },
    {  0.0    ,  0.52104,  0.0    ,  0.0     }
    };


  REQUIRE( accu(abs(diagmat(A   ) - Amain)) == Approx(0.0 ) );
  REQUIRE( accu(abs(diagmat(A, 0) - Amain)) == Approx(0.0 ) );

  REQUIRE( accu(abs(diagmat(A, 1) - Ap1  )) == Approx(0.0 ) );
  REQUIRE( accu(abs(diagmat(A,-1) - Am1  )) == Approx(0.0 ) );
  }



TEST_CASE("fn_diagmat_2")
  {
  mat A =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  vec dp1   = {  0.69298,  0.78987,  0.40163 };
  vec dmain = { -0.78838, -0.12020, -0.22263 };
  vec dm1   = {  0.49345,  0.52104           };

  mat Ap1  (size(A),fill::zeros);    Ap1.diag( 1) = dp1;
  mat Amain(size(A),fill::zeros);  Amain.diag(  ) = dmain;
  mat Am1  (size(A),fill::zeros);    Am1.diag(-1) = dm1;

  REQUIRE( accu(abs(diagmat(A   ) - Amain)) == Approx(0.0) );
  REQUIRE( accu(abs(diagmat(A, 0) - Amain)) == Approx(0.0) );

  REQUIRE( accu(abs(diagmat(A, 1) - Ap1)) == Approx(0.0) );
  REQUIRE( accu(abs(diagmat(A,-1) - Am1)) == Approx(0.0) );
  }



TEST_CASE("fn_diagmat_3")
  {
  mat A =
    {
    { -0.78838,  0.69298,  0.41084,  0.90142 },
    {  0.49345, -0.12020,  0.78987,  0.53124 },
    {  0.73573,  0.52104, -0.22263,  0.40163 }
    };

  mat B =
    "\
     0.171180   0.106848   0.490557  -0.079866;\
     0.073839  -0.428277  -0.049842   0.398193;\
    -0.030523   0.366160   0.260348  -0.412238;\
    ";

  mat Asub = A(span::all,span(0,2));
  mat At   = A.t();

  mat Bsub = B(span::all,span(0,2));
  mat Bt   = B.t();

  mat Asubdiagmat_times_Bsubdiagmat =
    "\
    -0.13495488840   0.00000000000   0.00000000000;\
     0.00000000000   0.05147889540   0.00000000000;\
     0.00000000000   0.00000000000  -0.05796127524;\
    ";

  mat Bsub_times_Adiagmat =
    "\
    -0.13495488840  -0.01284312960  -0.10921270491   0.00000000000;\
    -0.05821319082   0.05147889540   0.01109632446   0.00000000000;\
     0.02406372274  -0.04401243200  -0.05796127524   0.00000000000;\
    ";

  mat Adiagmat_times_Bt =
    "\
    -0.134955  -0.058213   0.024064;\
    -0.012843   0.051479  -0.044012;\
    -0.109213   0.011096  -0.057961;\
    ";

  REQUIRE( accu(abs((diagmat(Asub) * diagmat(Bsub)) - Asubdiagmat_times_Bsubdiagmat)) == Approx(0.0) );

  REQUIRE( accu(abs((Bsub                    * diagmat(A)) - Bsub_times_Adiagmat)) == Approx(0.0) );
  REQUIRE( accu(abs((B(span::all, span(0,2)) * diagmat(A)) - Bsub_times_Adiagmat)) == Approx(0.0) );

  REQUIRE( accu(abs((diagmat(A) * Bt    ) - Adiagmat_times_Bt  )) == Approx(0.0) );
  REQUIRE( accu(abs((diagmat(A) * B.t() ) - Adiagmat_times_Bt  )) == Approx(0.0) );

  // TODO: Asub and At
  }
