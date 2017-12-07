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

TEST_CASE("fn_var_empty_sparse_test")
  {
  SpMat<double> m(100, 100);

  SpRow<double> result = var(m);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 0, 0);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 1, 0);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 1);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  SpCol<double> colres = var(m, 1, 1);

  REQUIRE( colres.n_cols == 1 );
  REQUIRE( colres.n_rows == 100 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) colres[i] == Approx(0.0) );
    }

  colres = var(m, 0, 1);

  REQUIRE( colres.n_cols == 1 );
  REQUIRE( colres.n_rows == 100 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) colres[i] == Approx(0.0) );
    }
  }



TEST_CASE("fn_var_empty_cx_sparse_test")
  {
  SpMat<std::complex<double> > m(100, 100);

  SpRow<double> result = var(m);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 0, 0);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 1, 0);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  result = var(m, 1);

  REQUIRE( result.n_cols == 100 );
  REQUIRE( result.n_rows == 1 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) result[i] == Approx(0.0) );
    }

  SpCol<double> colres = var(m, 1, 1);

  REQUIRE( colres.n_cols == 1 );
  REQUIRE( colres.n_rows == 100 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) colres[i] == Approx(0.0) );
    }

  colres = var(m, 0, 1);

  REQUIRE( colres.n_cols == 1 );
  REQUIRE( colres.n_rows == 100 );
  for (uword i = 0; i < 100; ++i)
    {
    REQUIRE( (double) colres[i] == Approx(0.0) );
    }
  }



TEST_CASE("fn_var_sparse_test")
  {
  // Create a random matrix and do variance testing on it, with varying levels
  // of nonzero (eventually this becomes a fully dense matrix).
  for (int i = 0; i < 10; ++i)
    {
    SpMat<double> x;
    x.sprandu(50, 75, ((double) (i + 1)) / 10);
    mat d(x);

    SpRow<double> rr = var(x);
    rowvec drr = var(d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x, 1, 0);
    drr = var(d, 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    SpCol<double> cr = var(x, 0, 1);
    vec dcr = var(d, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x, 1, 1);
    dcr = var(d, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on a subview.
    rr = var(x.submat(11, 11, 30, 45), 0, 0);
    drr = var(d.submat(11, 11, 30, 45), 0, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x.submat(11, 11, 30, 45), 1, 0);
    drr = var(d.submat(11, 11, 30, 45), 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(x.submat(11, 11, 30, 45), 0, 1);
    dcr = var(d.submat(11, 11, 30, 45), 0, 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x.submat(11, 11, 30, 45), 1, 1);
    dcr = var(d.submat(11, 11, 30, 45), 1, 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on an SpOp (spop_scalar_times)
    rr = var(3.0 * x, 0, 0);
    drr = var(3.0 * d, 0, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(3.0 * x, 1, 0);
    drr = var(3.0 * d, 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(4.5 * x, 0, 1);
    dcr = var(4.5 * d, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(4.5 * x, 1, 1);
    dcr = var(4.5 * d, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on an SpGlue!
    SpMat<double> y;
    y.sprandu(50, 75, 0.3);
    mat e(y);

    rr = var(x + y);
    drr = var(d + e);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x + y, 1);
    drr = var(d + e, 1);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(x + y, 0, 1);
    dcr = var(d + e, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x + y, 1, 1);
    dcr = var(d + e, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }
    }
  }



TEST_CASE("fn_var_sparse_cx_test")
  {
  // Create a random matrix and do variance testing on it, with varying levels
  // of nonzero (eventually this becomes a fully dense matrix).
  for (int i = 0; i < 10; ++i)
    {
    SpMat<std::complex<double> > x;
    x.sprandu(50, 75, ((double) (i + 1)) / 10);
    cx_mat d(x);

    SpRow<double> rr = var(x);
    rowvec drr = var(d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x, 1, 0);
    drr = var(d, 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    SpCol<double> cr = var(x, 0, 1);
    vec dcr = var(d, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x, 1, 1);
    dcr = var(d, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on a subview.
    rr = var(x.submat(11, 11, 30, 45), 0, 0);
    drr = var(d.submat(11, 11, 30, 45), 0, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x.submat(11, 11, 30, 45), 1, 0);
    drr = var(d.submat(11, 11, 30, 45), 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(x.submat(11, 11, 30, 45), 0, 1);
    dcr = var(d.submat(11, 11, 30, 45), 0, 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x.submat(11, 11, 30, 45), 1, 1);
    dcr = var(d.submat(11, 11, 30, 45), 1, 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on an SpOp (spop_scalar_times)
    rr = var(3.0 * x, 0, 0);
    drr = var(3.0 * d, 0, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(3.0 * x, 1, 0);
    drr = var(3.0 * d, 1, 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(4.5 * x, 0, 1);
    dcr = var(4.5 * d, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(4.5 * x, 1, 1);
    dcr = var(4.5 * d, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    // Now on an SpGlue!
    SpMat<std::complex<double> > y;
    y.sprandu(50, 75, 0.3);
    cx_mat e(y);

    rr = var(x + y);
    drr = var(d + e);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    rr = var(x + y, 1);
    drr = var(d + e, 1);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( drr[j] == Approx((double) rr[j]) );
      }

    cr = var(x + y, 0, 1);
    dcr = var(d + e, 0, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }

    cr = var(x + y, 1, 1);
    dcr = var(d + e, 1, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( dcr[j] == Approx((double) cr[j]) );
      }
    }
  }



TEST_CASE("fn_var_sparse_alias_test")
  {
  sp_mat s;
  s.sprandu(70, 70, 0.3);
  mat d(s);

  s = var(s);
  d = var(d);

  REQUIRE( d.n_rows == s.n_rows );
  REQUIRE( d.n_cols == s.n_cols );
  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE(d[i] == Approx((double) s[i]) );
    }

  s.sprandu(70, 70, 0.3);
  d = s;

  s = var(s, 1);
  d = var(d, 1);
  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx((double) s[i]) );
    }
  }
