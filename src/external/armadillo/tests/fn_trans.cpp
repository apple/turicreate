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


TEST_CASE("fn_trans_1")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat At =
    "\
     0.061198   0.437242  -0.492474   0.336352   0.239585;\
     0.201990   0.058956  -0.031309   0.411541  -0.428913;\
     0.019678  -0.149362   0.314156   0.458476  -0.406953;\
    -0.493936  -0.045465   0.419733  -0.393139  -0.291020;\
    -0.126745   0.296153   0.068317  -0.135040  -0.353768;\
     0.051408   0.035437  -0.454499   0.373833   0.258704;\
    ";

  rowvec At_sum0 = { -0.28641, 0.63296, -0.17608, 1.05202, -0.98237 };

  colvec At_sum1 =
    {
     0.58190,
     0.21227,
     0.23599,
    -0.80383,
    -0.25108,
     0.26488
    };


  rowvec A_col1_t = { 0.201990, 0.058956, -0.031309, 0.411541, -0.428913 };

  colvec A_row1_t =
    {
     0.437242,
     0.058956,
    -0.149362,
    -0.045465,
     0.296153,
     0.035437
    };


  double accu_A_col1_t = 0.21227;
  double accu_A_row1_t = 0.63296;

  REQUIRE( accu(abs(mat(A.t().t()) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(    A.t().t()  - A)) == Approx(0.0) );

  REQUIRE( accu(abs(mat(A.t()    ) - At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(A.st()   ) - At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(A.ht()   ) - At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat( trans(A)) - At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(strans(A)) - At)) == Approx(0.0) );

  REQUIRE( accu(abs(A.t()     - At)) == Approx(0.0) );
  REQUIRE( accu(abs(A.st()    - At)) == Approx(0.0) );
  REQUIRE( accu(abs(A.ht()    - At)) == Approx(0.0) );
  REQUIRE( accu(abs( trans(A) - At)) == Approx(0.0) );
  REQUIRE( accu(abs(strans(A) - At)) == Approx(0.0) );

  REQUIRE( accu(abs(mat(At.t()    ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(At.st()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(At.ht()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat( trans(At)) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(strans(At)) - A)) == Approx(0.0) );

  REQUIRE( accu(abs(At.t()     - A)) == Approx(0.0) );
  REQUIRE( accu(abs(At.st()    - A)) == Approx(0.0) );
  REQUIRE( accu(abs(At.ht()    - A)) == Approx(0.0) );
  REQUIRE( accu(abs( trans(At) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(strans(At) - A)) == Approx(0.0) );

  REQUIRE( accu(abs((0 + At.t()    ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs((0 + At.st()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs((0 + At.ht()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs((0 +  trans(At)) - A)) == Approx(0.0) );
  REQUIRE( accu(abs((0 + strans(At)) - A)) == Approx(0.0) );

  REQUIRE( accu(abs(mat(0 + At.t()    ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(0 + At.st()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(0 + At.ht()   ) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(0 +  trans(At)) - A)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(0 + strans(At)) - A)) == Approx(0.0) );


  REQUIRE( accu(abs(2*A.t()    - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(2*trans(A) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs((2*A).t()  - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(2*A) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs((A+A).t()  - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(A+A) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs((A.t()    + At) - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs((trans(A) + At) - 2*At)) == Approx(0.0) );


  REQUIRE( accu(abs(mat(2*A.t())    - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(2*trans(A)) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs(mat((2*A).t())  - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(trans(2*A)) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs(mat((A+A).t())  - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(trans(A+A)) - 2*At)) == Approx(0.0) );

  REQUIRE( accu(abs(mat(A.t()    + At) - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(mat(trans(A) + At) - 2*At)) == Approx(0.0) );


  REQUIRE( accu(abs(rowvec(A.col(1).t()) - A_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(colvec(A.row(1).t()) - A_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs(A.col(1).t() - A_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(A.row(1).t() - A_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs(2*A.col(1).t() - 2*A_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(2*A.row(1).t() - 2*A_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs( (A.col(1).t() + A_col1_t) - 2*A_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs( (A.row(1).t() + A_row1_t) - 2*A_row1_t)) == Approx(0.0) );


  REQUIRE( abs( accu(A.col(1).t()) - accu_A_col1_t ) == Approx(0.0) );
  REQUIRE( abs( accu(A.row(1).t()) - accu_A_row1_t ) == Approx(0.0) );

  REQUIRE( abs( accu(A.col(1).t()) - accu(A.col(1)) ) == Approx(0.0) );
  REQUIRE( abs( accu(A.row(1).t()) - accu(A.row(1)) ) == Approx(0.0) );

  REQUIRE( abs( sum(A.col(1).t()) - accu_A_col1_t ) == Approx(0.0) );
  REQUIRE( abs( sum(A.row(1).t()) - accu_A_row1_t ) == Approx(0.0) );

  REQUIRE_THROWS( A + A.t() );
  }



TEST_CASE("fn_trans_2")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  cx_mat C = cx_mat(A,fliplr(A));

  cx_mat Ct =
    {
    { cx_double( 0.061198, -0.051408), cx_double( 0.437242, -0.035437), cx_double(-0.492474, +0.454499), cx_double( 0.336352, -0.373833), cx_double( 0.239585, -0.258704) },
    { cx_double( 0.201990, +0.126745), cx_double( 0.058956, -0.296153), cx_double(-0.031309, -0.068317), cx_double( 0.411541, +0.135040), cx_double(-0.428913, +0.353768) },
    { cx_double( 0.019678, +0.493936), cx_double(-0.149362, +0.045465), cx_double( 0.314156, -0.419733), cx_double( 0.458476, +0.393139), cx_double(-0.406953, +0.291020) },
    { cx_double(-0.493936, -0.019678), cx_double(-0.045465, +0.149362), cx_double( 0.419733, -0.314156), cx_double(-0.393139, -0.458476), cx_double(-0.291020, +0.406953) },
    { cx_double(-0.126745, -0.201990), cx_double( 0.296153, -0.058956), cx_double( 0.068317, +0.031309), cx_double(-0.135040, -0.411541), cx_double(-0.353768, +0.428913) },
    { cx_double( 0.051408, -0.061198), cx_double( 0.035437, -0.437242), cx_double(-0.454499, +0.492474), cx_double( 0.373833, -0.336352), cx_double( 0.258704, -0.239585) }
    };

  cx_rowvec C_col1_t = { cx_double(0.201990, +0.126745), cx_double(0.058956, -0.296153), cx_double(-0.031309, -0.068317), cx_double(0.411541, +0.135040), cx_double(-0.428913, +0.353768) };

  cx_colvec C_row1_t =
    {
    cx_double( 0.437242, -0.035437),
    cx_double( 0.058956, -0.296153),
    cx_double(-0.149362, +0.045465),
    cx_double(-0.045465, +0.149362),
    cx_double( 0.296153, -0.058956),
    cx_double( 0.035437, -0.437242)
    };


  REQUIRE( accu(abs(C.t().t() - C)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat(C.t()   ) - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(C.ht()  ) - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(trans(C)) - Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(C.t()    - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(C.ht()   - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(C) - Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat(Ct.t()   ) - C)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(Ct.ht()  ) - C)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(trans(Ct)) - C)) == Approx(0.0) );

  REQUIRE( accu(abs(Ct.t()    - C)) == Approx(0.0) );
  REQUIRE( accu(abs(Ct.ht()   - C)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(Ct) - C)) == Approx(0.0) );

  REQUIRE( accu(abs(2*C.t()    - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(2*trans(C) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs((2*C).t()  - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(2*C) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs((C+C).t()  - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(trans(C+C) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_double(2,3)*C.t()    - cx_double(2,3)*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_double(2,3)*trans(C) - cx_double(2,3)*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat(2*C.t())    - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(2*trans(C)) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat((2*C).t())  - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(trans(2*C)) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat((C+C).t())  - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(trans(C+C)) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_mat(cx_double(2,3)*C.t())    - cx_double(2,3)*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(cx_double(2,3)*trans(C)) - cx_double(2,3)*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs((C.t()    + Ct) - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs((trans(C) + Ct) - 2*Ct)) == Approx(0.0) );

  REQUIRE( accu(abs(cx_rowvec(C.col(1).t()) - C_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(cx_colvec(C.row(1).t()) - C_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs(C.col(1).t() - C_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(C.row(1).t() - C_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs(2*C.col(1).t() - 2*C_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs(2*C.row(1).t() - 2*C_row1_t)) == Approx(0.0) );

  REQUIRE( accu(abs( (C.col(1).t() + C_col1_t) - 2*C_col1_t)) == Approx(0.0) );
  REQUIRE( accu(abs( (C.row(1).t() + C_row1_t) - 2*C_row1_t)) == Approx(0.0) );

  //

  REQUIRE( accu(abs(cx_mat(C.st())    - conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(cx_mat(strans(C)) - conj(Ct))) == Approx(0.0) );

  REQUIRE( accu(abs(C.st()    - conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(strans(C) - conj(Ct))) == Approx(0.0) );

  REQUIRE( accu(abs(2*C.st()    - conj(2*Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(2*strans(C) - conj(2*Ct))) == Approx(0.0) );

  REQUIRE( accu(abs(cx_double(2,3)*C.st()    - cx_double(2,3)*conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(cx_double(2,3)*strans(C) - cx_double(2,3)*conj(Ct))) == Approx(0.0) );

  REQUIRE( accu(abs((C.st()    + C.st()) - conj(2*Ct))) == Approx(0.0) );
  REQUIRE( accu(abs((strans(C) + C.st()) - conj(2*Ct))) == Approx(0.0) );

  REQUIRE( accu(abs(cx_rowvec(C.col(1).st()) - conj(C_col1_t))) == Approx(0.0) );
  REQUIRE( accu(abs(cx_colvec(C.row(1).st()) - conj(C_row1_t))) == Approx(0.0) );

  REQUIRE( accu(abs(C.col(1).st() - conj(C_col1_t))) == Approx(0.0) );
  REQUIRE( accu(abs(C.row(1).st() - conj(C_row1_t))) == Approx(0.0) );

  REQUIRE( accu(abs(2*C.col(1).st() - conj(2*C_col1_t))) == Approx(0.0) );
  REQUIRE( accu(abs(2*C.row(1).st() - conj(2*C_row1_t))) == Approx(0.0) );

  REQUIRE( accu(abs( (C.col(1).st() + conj(C_col1_t)) - conj(2*C_col1_t))) == Approx(0.0) );
  REQUIRE( accu(abs( (C.row(1).st() + conj(C_row1_t)) - conj(2*C_row1_t))) == Approx(0.0) );

  //

  REQUIRE_THROWS( C + C.t() );
  }



TEST_CASE("fn_trans_3")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  mat At =
    "\
     0.061198   0.437242  -0.492474   0.336352   0.239585;\
     0.201990   0.058956  -0.031309   0.411541  -0.428913;\
     0.019678  -0.149362   0.314156   0.458476  -0.406953;\
    -0.493936  -0.045465   0.419733  -0.393139  -0.291020;\
    -0.126745   0.296153   0.068317  -0.135040  -0.353768;\
     0.051408   0.035437  -0.454499   0.373833   0.258704;\
    ";

  mat B = A.head_cols(5);

  mat Bt = At.head_rows(5);

  mat X;
  mat Y;

  X = A;  X = X.t();
  Y = B;  Y = Y.t();

  REQUIRE( accu(abs(X - At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - Bt)) == Approx(0.0) );

  X = A;  X = 0 + X.t();
  Y = B;  Y = 0 + Y.t();

  REQUIRE( accu(abs(X - At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - Bt)) == Approx(0.0) );

  X = A;  X = 2*X.t();
  Y = B;  Y = 2*Y.t();

  REQUIRE( accu(abs(X - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Bt)) == Approx(0.0) );

  X = A;  X = 0 + 2*X.t();
  Y = B;  Y = 0 + 2*Y.t();

  REQUIRE( accu(abs(X - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Bt)) == Approx(0.0) );

  X = A;  X = (2*X).t();
  Y = B;  Y = (2*Y).t();

  REQUIRE( accu(abs(X - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Bt)) == Approx(0.0) );

  X = A;  X = (X+X).t();
  Y = B;  Y = (Y+Y).t();

  REQUIRE( accu(abs(X - 2*At)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Bt)) == Approx(0.0) );

  colvec q = A.col(1);
  rowvec r = A.row(1);

  REQUIRE_THROWS( q = q.t() );
  REQUIRE_THROWS( r = r.t() );
  }



TEST_CASE("fn_trans_4")
  {
  mat A =
    "\
     0.061198   0.201990   0.019678  -0.493936  -0.126745   0.051408;\
     0.437242   0.058956  -0.149362  -0.045465   0.296153   0.035437;\
    -0.492474  -0.031309   0.314156   0.419733   0.068317  -0.454499;\
     0.336352   0.411541   0.458476  -0.393139  -0.135040   0.373833;\
     0.239585  -0.428913  -0.406953  -0.291020  -0.353768   0.258704;\
    ";

  cx_mat C = cx_mat(A,fliplr(A));

  cx_mat Ct =
    {
    { cx_double( 0.061198, -0.051408), cx_double( 0.437242, -0.035437), cx_double(-0.492474, +0.454499), cx_double( 0.336352, -0.373833), cx_double( 0.239585, -0.258704) },
    { cx_double( 0.201990, +0.126745), cx_double( 0.058956, -0.296153), cx_double(-0.031309, -0.068317), cx_double( 0.411541, +0.135040), cx_double(-0.428913, +0.353768) },
    { cx_double( 0.019678, +0.493936), cx_double(-0.149362, +0.045465), cx_double( 0.314156, -0.419733), cx_double( 0.458476, +0.393139), cx_double(-0.406953, +0.291020) },
    { cx_double(-0.493936, -0.019678), cx_double(-0.045465, +0.149362), cx_double( 0.419733, -0.314156), cx_double(-0.393139, -0.458476), cx_double(-0.291020, +0.406953) },
    { cx_double(-0.126745, -0.201990), cx_double( 0.296153, -0.058956), cx_double( 0.068317, +0.031309), cx_double(-0.135040, -0.411541), cx_double(-0.353768, +0.428913) },
    { cx_double( 0.051408, -0.061198), cx_double( 0.035437, -0.437242), cx_double(-0.454499, +0.492474), cx_double( 0.373833, -0.336352), cx_double( 0.258704, -0.239585) }
    };

  cx_mat D = C.head_cols(5);

  cx_mat Dt = Ct.head_rows(5);

  cx_mat X;
  cx_mat Y;

  //

  X = C;  X = X.t();
  Y = D;  Y = Y.t();

  REQUIRE( accu(abs(X - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - Dt)) == Approx(0.0) );

  X = C;  X = 0 + X.t();
  Y = D;  Y = 0 + Y.t();

  REQUIRE( accu(abs(X - Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - Dt)) == Approx(0.0) );

  X = C;  X = 2*X.t();
  Y = D;  Y = 2*Y.t();

  REQUIRE( accu(abs(X - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Dt)) == Approx(0.0) );

  X = C;  X = 0 + 2*X.t();
  Y = D;  Y = 0 + 2*Y.t();

  REQUIRE( accu(abs(X - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Dt)) == Approx(0.0) );

  X = C;  X = (2*X).t();
  Y = D;  Y = (2*Y).t();

  REQUIRE( accu(abs(X - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Dt)) == Approx(0.0) );

  X = C;  X = (X+X).t();
  Y = D;  Y = (Y+Y).t();

  REQUIRE( accu(abs(X - 2*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*Dt)) == Approx(0.0) );

  X = C;  X = cx_double(2,3)*X.t();
  Y = D;  Y = cx_double(2,3)*Y.t();

  REQUIRE( accu(abs(X - cx_double(2,3)*Ct)) == Approx(0.0) );
  REQUIRE( accu(abs(Y - cx_double(2,3)*Dt)) == Approx(0.0) );

  //

  X = C;  X = X.st();
  Y = D;  Y = Y.st();

  REQUIRE( accu(abs(X - conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - conj(Dt))) == Approx(0.0) );

  X = C;  X = 0 + X.st();
  Y = D;  Y = 0 + Y.st();

  REQUIRE( accu(abs(X - conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - conj(Dt))) == Approx(0.0) );

  X = C;  X = 2*X.st();
  Y = D;  Y = 2*Y.st();

  REQUIRE( accu(abs(X - 2*conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*conj(Dt))) == Approx(0.0) );

  X = C;  X = 0 + 2*X.st();
  Y = D;  Y = 0 + 2*Y.st();

  REQUIRE( accu(abs(X - 2*conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - 2*conj(Dt))) == Approx(0.0) );

  X = C;  X = (2*X).st();
  Y = D;  Y = (2*Y).st();

  REQUIRE( accu(abs(X - conj(2*Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - conj(2*Dt))) == Approx(0.0) );

  X = C;  X = (X+X).st();
  Y = D;  Y = (Y+Y).st();

  REQUIRE( accu(abs(X - conj(2*Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - conj(2*Dt))) == Approx(0.0) );

  X = C;  X = cx_double(2,3)*X.st();
  Y = D;  Y = cx_double(2,3)*Y.st();

  REQUIRE( accu(abs(X - cx_double(2,3)*conj(Ct))) == Approx(0.0) );
  REQUIRE( accu(abs(Y - cx_double(2,3)*conj(Dt))) == Approx(0.0) );
  }



TEST_CASE("op_trans_sp_mat")
  {
  SpMat<unsigned int> a(4, 4);
  a(1, 0) = 5;
  a(2, 2) = 3;
  a(3, 3) = 4;
  a(1, 3) = 6;
  a(3, 1) = 8;

  SpMat<unsigned int> b = trans(a);

  REQUIRE( (unsigned int) b(0, 0) == 0 );
  REQUIRE( (unsigned int) b(1, 0) == 0 );
  REQUIRE( (unsigned int) b(2, 0) == 0 );
  REQUIRE( (unsigned int) b(3, 0) == 0 );
  REQUIRE( (unsigned int) b(0, 1) == 5 );
  REQUIRE( (unsigned int) b(1, 1) == 0 );
  REQUIRE( (unsigned int) b(2, 1) == 0 );
  REQUIRE( (unsigned int) b(3, 1) == 6 );
  REQUIRE( (unsigned int) b(0, 2) == 0 );
  REQUIRE( (unsigned int) b(1, 2) == 0 );
  REQUIRE( (unsigned int) b(2, 2) == 3 );
  REQUIRE( (unsigned int) b(3, 2) == 0 );
  REQUIRE( (unsigned int) b(0, 3) == 0 );
  REQUIRE( (unsigned int) b(1, 3) == 8 );
  REQUIRE( (unsigned int) b(2, 3) == 0 );
  REQUIRE( (unsigned int) b(3, 3) == 4 );

  b = trans(b); // inplace

  REQUIRE( (unsigned int) b(0, 0) == 0 );
  REQUIRE( (unsigned int) b(1, 0) == 5 );
  REQUIRE( (unsigned int) b(2, 0) == 0 );
  REQUIRE( (unsigned int) b(3, 0) == 0 );
  REQUIRE( (unsigned int) b(0, 1) == 0 );
  REQUIRE( (unsigned int) b(1, 1) == 0 );
  REQUIRE( (unsigned int) b(2, 1) == 0 );
  REQUIRE( (unsigned int) b(3, 1) == 8 );
  REQUIRE( (unsigned int) b(0, 2) == 0 );
  REQUIRE( (unsigned int) b(1, 2) == 0 );
  REQUIRE( (unsigned int) b(2, 2) == 3 );
  REQUIRE( (unsigned int) b(3, 2) == 0 );
  REQUIRE( (unsigned int) b(0, 3) == 0 );
  REQUIRE( (unsigned int) b(1, 3) == 6 );
  REQUIRE( (unsigned int) b(2, 3) == 0 );
  REQUIRE( (unsigned int) b(3, 3) == 4 );

  b = trans(a + a); // on an op

  REQUIRE( (unsigned int) b(0, 0) == 0 );
  REQUIRE( (unsigned int) b(1, 0) == 0 );
  REQUIRE( (unsigned int) b(2, 0) == 0 );
  REQUIRE( (unsigned int) b(3, 0) == 0 );
  REQUIRE( (unsigned int) b(0, 1) == 10 );
  REQUIRE( (unsigned int) b(1, 1) == 0 );
  REQUIRE( (unsigned int) b(2, 1) == 0 );
  REQUIRE( (unsigned int) b(3, 1) == 12 );
  REQUIRE( (unsigned int) b(0, 2) == 0 );
  REQUIRE( (unsigned int) b(1, 2) == 0 );
  REQUIRE( (unsigned int) b(2, 2) == 6 );
  REQUIRE( (unsigned int) b(3, 2) == 0 );
  REQUIRE( (unsigned int) b(0, 3) == 0 );
  REQUIRE( (unsigned int) b(1, 3) == 16 );
  REQUIRE( (unsigned int) b(2, 3) == 0 );
  REQUIRE( (unsigned int) b(3, 3) == 8 );

  b = trans(trans(a)); // on another trans

  REQUIRE( (unsigned int) b(0, 0) == 0 );
  REQUIRE( (unsigned int) b(1, 0) == 5 );
  REQUIRE( (unsigned int) b(2, 0) == 0 );
  REQUIRE( (unsigned int) b(3, 0) == 0 );
  REQUIRE( (unsigned int) b(0, 1) == 0 );
  REQUIRE( (unsigned int) b(1, 1) == 0 );
  REQUIRE( (unsigned int) b(2, 1) == 0 );
  REQUIRE( (unsigned int) b(3, 1) == 8 );
  REQUIRE( (unsigned int) b(0, 2) == 0 );
  REQUIRE( (unsigned int) b(1, 2) == 0 );
  REQUIRE( (unsigned int) b(2, 2) == 3 );
  REQUIRE( (unsigned int) b(3, 2) == 0 );
  REQUIRE( (unsigned int) b(0, 3) == 0 );
  REQUIRE( (unsigned int) b(1, 3) == 6 );
  REQUIRE( (unsigned int) b(2, 3) == 0 );
  REQUIRE( (unsigned int) b(3, 3) == 4 );
  }


TEST_CASE("op_trans_sp_cxmat")
  {
  SpMat<std::complex<double> > a(10, 10);
  for (uword c = 0; c < 7; ++c)
  {
    a(c, c) = std::complex<double>(1.3, 2.4);
    a(c + 1, c) = std::complex<double>(0.0, -1.3);
    a(c + 2, c) = std::complex<double>(2.1, 0.0);
  }

  SpMat<std::complex<double> > b = trans(a);

  REQUIRE( b.n_nonzero == a.n_nonzero );

  for (uword r = 0; r < 10; ++r)
    {
    for (uword c = 0; c < 10; ++c)
      {
      double lr = real(std::complex<double>(a(r, c)));
      double rr = real(std::complex<double>(b(c, r)));
      double li = imag(std::complex<double>(a(r, c)));
      double ri = imag(std::complex<double>(b(c, r)));

      REQUIRE( lr == Approx(rr) );
      REQUIRE( li == Approx(-ri) );
      }
    }

  b = trans(a.submat(3, 3, 7, 7));

  REQUIRE( b.n_nonzero == a.submat(3, 3, 7, 7).n_nonzero );

  for (uword r = 0; r < 5; ++r)
    {
    for (uword c = 0; c < 5; ++c)
      {
      double lr = real(std::complex<double>(a(r + 3, c + 3)));
      double rr = real(std::complex<double>(b(c, r)));
      double li = imag(std::complex<double>(a(r + 3, c + 3)));
      double ri = imag(std::complex<double>(b(c, r)));

      REQUIRE( lr == Approx(rr) );
      REQUIRE( li == Approx(-ri) );
      }
    }
  }
