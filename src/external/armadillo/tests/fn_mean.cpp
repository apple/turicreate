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

TEST_CASE("fn_mean_spmat_empty_test")
  {
  SpMat<double> m(20, 25);

  SpRow<double> result = mean(m, 0);
  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 25 );

  SpCol<double> result2 = mean(m, 1);
  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 20 );
  REQUIRE( result2.n_cols == 1 );

  double r = mean(mean(m));
  REQUIRE( r == Approx(0.0) );

  // Now the same with subviews.
  result = mean(m.submat(2, 2, 11, 16));
  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 15 );

  result2 = mean(m.submat(2, 2, 11, 16), 1);
  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 10 );
  REQUIRE( result2.n_cols == 1 );

  r = mean(mean(m.submat(2, 2, 11, 16)));
  REQUIRE( r == Approx(0.0) );

  // And with an operation.
  result = mean(trans(m));
  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 20 );

  result2 = mean(trans(m), 1);
  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 25 );
  REQUIRE( result2.n_cols == 1 );

  r = mean(mean(trans(m)));
  REQUIRE( r == Approx(0.0) );
  }



TEST_CASE("fn_mean_spcxmat_empty_test")
  {
  // Now with complex numbers.
  SpMat<std::complex<double> > m(20, 25);
  SpRow<std::complex<double> > result = mean(m, 0);

  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 25 );

  SpCol<std::complex<double> > result2 = mean(m, 1);

  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 20 );
  REQUIRE( result2.n_cols == 1 );

  std::complex<double> r = mean(mean(m));

  REQUIRE( real(r) == Approx(0.0) );
  REQUIRE( imag(r) == Approx(0.0) );

  // Now the same with subviews.
  result = mean(m.submat(2, 2, 11, 16));
  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 15 );

  result2 = mean(m.submat(2, 2, 11, 16), 1);
  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 10 );
  REQUIRE( result2.n_cols == 1 );

  r = mean(mean(m.submat(2, 2, 11, 16)));
  REQUIRE( real(r) == Approx(0.0) );
  REQUIRE( imag(r) == Approx(0.0) );

  // And with an operation.
  result = mean(trans(m));
  REQUIRE( result.n_nonzero == 0 );
  REQUIRE( result.n_rows == 1 );
  REQUIRE( result.n_cols == 20 );

  result2 = mean(trans(m), 1);
  REQUIRE( result2.n_nonzero == 0 );
  REQUIRE( result2.n_rows == 25 );
  REQUIRE( result2.n_cols == 1 );

  r = mean(mean(trans(m)));
  REQUIRE( real(r) == Approx(0.0) );
  REQUIRE( imag(r) == Approx(0.0) );
  }



TEST_CASE("fn_mean_spmat_test")
  {
  // Create a random matrix and do mean testing on it, with varying levels of
  // nonzero (eventually this becomes a fully dense matrix).
  for (int i = 0; i < 10; ++i)
    {
    SpMat<double> x;
    x.sprandu(50, 75, ((double) (i + 1)) / 10);
    mat d(x);

    SpRow<double> rr = mean(x);
    rowvec drr = mean(d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      REQUIRE( drr[j] == Approx((double) rr[j]) );

    SpCol<double> cr = mean(x, 1);
    vec dcr = mean(d, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      REQUIRE( dcr[j] == Approx((double) cr[j]) );

    double dr = mean(mean(x));
    double ddr = mean(mean(d));

    REQUIRE( dr == Approx(ddr) );

    // Now on a subview.
    rr = mean(x.submat(11, 11, 30, 45), 0);
    drr = mean(d.submat(11, 11, 30, 45), 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      REQUIRE( drr[j] == Approx((double) rr[j]) );

    cr = mean(x.submat(11, 11, 30, 45), 1);
    dcr = mean(d.submat(11, 11, 30, 45), 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      REQUIRE( dcr[j] == Approx((double) cr[j]) );

    dr = mean(mean(x.submat(11, 11, 30, 45)));
    ddr = mean(mean(d.submat(11, 11, 30, 45)));

    REQUIRE( dr == Approx(ddr) );

    // Now on an SpOp (spop_scalar_times)
    rr = mean(3.0 * x);
    drr = mean(3.0 * d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      REQUIRE( drr[j] == Approx((double) rr[j]) );

    cr = mean(4.5 * x, 1);
    dcr = mean(4.5 * d, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      REQUIRE( dcr[j] == Approx((double) cr[j]) );

    dr = mean(mean(1.2 * x));
    ddr = mean(mean(1.2 * d));

    REQUIRE( dr == Approx(ddr) );

    // Now on an SpGlue!
    SpMat<double> y;
    y.sprandu(50, 75, 0.3);
    mat e(y);

    rr = mean(x + y);
    drr = mean(d + e);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      REQUIRE( drr[j] == Approx((double) rr[j]) );

    cr = mean(x + y, 1);
    dcr = mean(d + e, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      REQUIRE( dcr[j] == Approx((double) cr[j]) );

    dr = mean(mean(x + y));
    ddr = mean(mean(d + e));

    REQUIRE( dr == Approx(ddr) );
    }
  }



TEST_CASE("fn_mean_spcxmat_test")
  {
  // Create a random matrix and do mean testing on it, with varying levels of
  // nonzero (eventually this becomes a fully dense matrix).
  for (int i = 0; i < 10; ++i)
  {
    SpMat<std::complex<double> > x;
    x.sprandu(50, 75, ((double) (i + 1)) / 10);
    cx_mat d(x);

    SpRow<std::complex<double> > rr = mean(x);
    cx_rowvec drr = mean(d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
      REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
      }

    SpCol<std::complex<double> > cr = mean(x, 1);
    cx_vec dcr = mean(d, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
      REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
      }

    std::complex<double> dr = mean(mean(x));
    std::complex<double> ddr = mean(mean(d));

    REQUIRE( real(dr) == Approx(real(ddr)) );
    REQUIRE( imag(dr) == Approx(imag(ddr)) );

    // Now on a subview.
    rr = mean(x.submat(11, 11, 30, 45), 0);
    drr = mean(d.submat(11, 11, 30, 45), 0);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 35 );
    for (uword j = 0; j < 35; ++j)
      {
      REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
      REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
      }

    cr = mean(x.submat(11, 11, 30, 45), 1);
    dcr = mean(d.submat(11, 11, 30, 45), 1);

    REQUIRE( cr.n_rows == 20 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 20; ++j)
      {
      REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
      REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
      }

    dr = mean(mean(x.submat(11, 11, 30, 45)));
    ddr = mean(mean(d.submat(11, 11, 30, 45)));

    REQUIRE( real(dr) == Approx(real(ddr)) );
    REQUIRE( imag(dr) == Approx(imag(ddr)) );

    // Now on an SpOp (spop_scalar_times)
    rr = mean(3.0 * x);
    drr = mean(3.0 * d);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
      REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
      }

    cr = mean(4.5 * x, 1);
    dcr = mean(4.5 * d, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
      REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
      }

    dr = mean(mean(1.2 * x));
    ddr = mean(mean(1.2 * d));

    REQUIRE( real(dr) == Approx(real(ddr)) );
    REQUIRE( imag(dr) == Approx(imag(ddr)) );

    // Now on an SpGlue!
    SpMat<std::complex<double> > y;
    y.sprandu(50, 75, 0.3);
    cx_mat e(y);

    rr = mean(x + y);
    drr = mean(d + e);

    REQUIRE( rr.n_rows == 1 );
    REQUIRE( rr.n_cols == 75 );
    for (uword j = 0; j < 75; ++j)
      {
      REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
      REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
      }

    cr = mean(x + y, 1);
    dcr = mean(d + e, 1);

    REQUIRE( cr.n_rows == 50 );
    REQUIRE( cr.n_cols == 1 );
    for (uword j = 0; j < 50; ++j)
      {
      REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
      REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
      }

    dr = mean(mean(x + y));
    ddr = mean(mean(d + e));

    REQUIRE( real(dr) == Approx(real(ddr)) );
    REQUIRE( imag(dr) == Approx(imag(ddr)) );
    }
  }


TEST_CASE("fn_mean_sp_vector_test")
  {
  // Test mean() on vectors.
  SpCol<double> c(1000);

  SpCol<double> cr = mean(c, 0);
  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( (double) cr[0] == Approx(0.0) );

  cr = mean(c, 1);
  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( (double) cr[i] == Approx(0.0) );
    }

  double ddcr = mean(c);
  REQUIRE( ddcr == Approx(0.0) );

  c.sprandu(1000, 1, 0.3);
  vec dc(c);

  cr = mean(c, 0);
  vec dcr = mean(dc, 0);

  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( (double) cr[0] == Approx(dcr[0]) );

  cr = mean(c, 1);
  dcr = mean(dc, 1);

  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( (double) cr[i] == Approx(dcr[i]) );
    }

  ddcr = mean(c);
  double dddr = mean(dc);

  REQUIRE( ddcr == Approx(dddr) );

  SpRow<double> r;
  r.sprandu(1, 1000, 0.3);
  rowvec dr(r);

  SpRow<double> rr = mean(r, 0);
  rowvec drr = mean(dr, 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1000 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( (double) rr[i] == Approx(drr[i]) );
    }

  rr = mean(r, 1);
  drr = mean(dr, 1);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1 );
  REQUIRE( (double) rr[0] == Approx(drr[0]) );

  ddcr = mean(r);
  dddr = mean(dr);

  REQUIRE( ddcr == Approx(dddr) );
  }



TEST_CASE("fn_mean_sp_cx_vector_test")
  {
  // Test mean() on vectors.
  SpCol<std::complex<double> > c(1000);

  SpCol<std::complex<double> > cr = mean(c, 0);
  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( real((std::complex<double>) cr[0]) == Approx(0.0) );
  REQUIRE( imag((std::complex<double>) cr[0]) == Approx(0.0) );

  cr = mean(c, 1);
  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
  {
    REQUIRE( real((std::complex<double>) cr[i]) == Approx(0.0) );
    REQUIRE( imag((std::complex<double>) cr[i]) == Approx(0.0) );
  }

  std::complex<double> ddcr = mean(c);
  REQUIRE( real(ddcr) == Approx(0.0) );
  REQUIRE( imag(ddcr) == Approx(0.0) );

  c.sprandu(1000, 1, 0.3);
  cx_vec dc(c);

  cr = mean(c, 0);
  cx_vec dcr = mean(dc, 0);

  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( real((std::complex<double>) cr[0]) == Approx(real(dcr[0])) );
  REQUIRE( imag((std::complex<double>) cr[0]) == Approx(imag(dcr[0])) );

  cr = mean(c, 1);
  dcr = mean(dc, 1);

  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( real((std::complex<double>) cr[i]) == Approx(real(dcr[i])) );
    REQUIRE( imag((std::complex<double>) cr[i]) == Approx(imag(dcr[i])) );
    }

  ddcr = mean(c);
  std::complex<double> dddr = mean(dc);

  REQUIRE( real(ddcr) == Approx(real(dddr)) );
  REQUIRE( imag(ddcr) == Approx(imag(dddr)) );

  SpRow<std::complex<double> > r;
  r.sprandu(1, 1000, 0.3);
  cx_rowvec dr(r);

  SpRow<std::complex<double> > rr = mean(r, 0);
  cx_rowvec drr = mean(dr, 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1000 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( real((std::complex<double>) rr[i]) == Approx(real(drr[i])) );
    REQUIRE( imag((std::complex<double>) rr[i]) == Approx(imag(drr[i])) );
    }

  rr = mean(r, 1);
  drr = mean(dr, 1);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1 );
  REQUIRE( real((std::complex<double>) rr[0]) == Approx(real(drr[0])) );
  REQUIRE( imag((std::complex<double>) rr[0]) == Approx(imag(drr[0])) );

  ddcr = mean(r);
  dddr = mean(dr);

  REQUIRE( real(ddcr) == Approx(real(dddr)) );
  REQUIRE( imag(ddcr) == Approx(imag(dddr)) );
  }



TEST_CASE("fn_mean_robust_sparse_test")
  {
  // Create a sparse matrix with values that will overflow.
  SpMat<double> x;
  x.sprandu(50, 75, 0.1);
  for (SpMat<double>::iterator i = x.begin(); i != x.end(); ++i)
    {
    (*i) *= std::numeric_limits<double>::max();
    }
  mat d(x);

  SpRow<double> rr = mean(x);
  rowvec drr = mean(d);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( drr[j] == Approx((double) rr[j]) );
    }

  SpCol<double> cr = mean(x, 1);
  vec dcr = mean(d, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( dcr[j] == Approx((double) cr[j]) );
    }

  double dr = mean(mean(x));
  double ddr = mean(mean(d));

  REQUIRE( dr == Approx(ddr) );

  // Now on a subview.
  rr = mean(x.submat(11, 11, 30, 45), 0);
  drr = mean(d.submat(11, 11, 30, 45), 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 35 );
  for (uword j = 0; j < 35; ++j)
    {
    REQUIRE( drr[j] == Approx((double) rr[j]) );
    }

  cr = mean(x.submat(11, 11, 30, 45), 1);
  dcr = mean(d.submat(11, 11, 30, 45), 1);

  REQUIRE( cr.n_rows == 20 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 20; ++j)
    {
    REQUIRE( dcr[j] == Approx((double) cr[j]) );
    }

  dr = mean(mean(x.submat(11, 11, 30, 45)));
  ddr = mean(mean(d.submat(11, 11, 30, 45)));

  REQUIRE( dr == Approx(ddr) );

  // Now on an SpOp (spop_scalar_times)
  rr = mean(0.4 * x);
  drr = mean(0.4 * d);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( drr[j] == Approx((double) rr[j]) );
    }

  cr = mean(0.1 * x, 1);
  dcr = mean(0.1 * d, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( dcr[j] == Approx((double) cr[j]) );
    }

  dr = mean(mean(0.7 * x));
  ddr = mean(mean(0.7 * d));

  REQUIRE( dr == Approx(ddr) );

  // Now on an SpGlue!
  SpMat<double> y;
  y.sprandu(50, 75, 0.3);
  for (SpMat<double>::iterator i = y.begin(); i != y.end(); ++i)
    {
    (*i) *= std::numeric_limits<double>::max();
    }
  mat e(y);

  rr = mean(0.5 * x + 0.5 * y);
  drr = mean(0.5 * d + 0.5 * e);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( drr[j] == Approx((double) rr[j]) );
    }

  cr = mean(0.5 * x + 0.5 * y, 1);
  dcr = mean(0.5 * d + 0.5 * e, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( dcr[j] == Approx((double) cr[j]) );
    }

  dr = mean(mean(0.5 * x + 0.5 * y));
  ddr = mean(mean(0.5 * d + 0.5 * e));

  REQUIRE( dr == Approx(ddr) );
  }



TEST_CASE("fn_mean_robust_cx_sparse_test")
  {
  SpMat<std::complex<double> > x;
  x.sprandu(50, 75, 0.3);
  for (SpMat<std::complex<double> >::iterator i = x.begin(); i != x.end(); ++i)
    {
    (*i) *= std::numeric_limits<double>::max();
    }
  cx_mat d(x);

  SpRow<std::complex<double> > rr = mean(x);
  cx_rowvec drr = mean(d);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
    REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
    }

  SpCol<std::complex<double> > cr = mean(x, 1);
  cx_vec dcr = mean(d, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
    REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
    }

  std::complex<double> dr = mean(mean(x));
  std::complex<double> ddr = mean(mean(d));

  REQUIRE( real(dr) == Approx(real(ddr)) );
  REQUIRE( imag(dr) == Approx(imag(ddr)) );

  // Now on a subview.
  rr = mean(x.submat(11, 11, 30, 45), 0);
  drr = mean(d.submat(11, 11, 30, 45), 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 35 );
  for (uword j = 0; j < 35; ++j)
    {
    REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
    REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
    }

  cr = mean(x.submat(11, 11, 30, 45), 1);
  dcr = mean(d.submat(11, 11, 30, 45), 1);

  REQUIRE( cr.n_rows == 20 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 20; ++j)
    {
    REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
    REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
    }

  dr = mean(mean(x.submat(11, 11, 30, 45)));
  ddr = mean(mean(d.submat(11, 11, 30, 45)));

  REQUIRE( real(dr) == Approx(real(ddr)) );
  REQUIRE( imag(dr) == Approx(imag(ddr)) );

  // Now on an SpOp (spop_scalar_times)
  rr = mean(0.5 * x);
  drr = mean(0.5 * d);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
    REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
    }

  cr = mean(0.7 * x, 1);
  dcr = mean(0.7 * d, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
    REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
    }

  dr = mean(mean(0.6 * x));
  ddr = mean(mean(0.6 * d));

  REQUIRE( real(dr) == Approx(real(ddr)) );
  REQUIRE( imag(dr) == Approx(imag(ddr)) );

  // Now on an SpGlue!
  SpMat<std::complex<double> > y;
  y.sprandu(50, 75, 0.3);
  for (SpMat<std::complex<double> >::iterator i = y.begin(); i != y.end(); ++i)
    {
    (*i) *= std::numeric_limits<double>::max();
    }
  cx_mat e(y);

  rr = mean(0.5 * x + 0.5 * y);
  drr = mean(0.5 * d + 0.5 * e);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 75 );
  for (uword j = 0; j < 75; ++j)
    {
    REQUIRE( real(drr[j]) == Approx(real((std::complex<double>) rr[j])) );
    REQUIRE( imag(drr[j]) == Approx(imag((std::complex<double>) rr[j])) );
    }

  cr = mean(0.5 * x + 0.5 * y, 1);
  dcr = mean(0.5 * d + 0.5 * e, 1);

  REQUIRE( cr.n_rows == 50 );
  REQUIRE( cr.n_cols == 1 );
  for (uword j = 0; j < 50; ++j)
    {
    REQUIRE( real(dcr[j]) == Approx(real((std::complex<double>) cr[j])) );
    REQUIRE( imag(dcr[j]) == Approx(imag((std::complex<double>) cr[j])) );
    }

  dr = mean(mean(0.5 * x + 0.5 * y));
  ddr = mean(mean(0.5 * d + 0.5 * e));

  REQUIRE( real(dr) == Approx(real(ddr)) );
  REQUIRE( imag(dr) == Approx(imag(ddr)) );
  }



TEST_CASE("fn_mean_robust_sparse_vector_test")
  {
  // Test mean() on vectors.
  SpCol<double> c(1000);

  SpCol<double> cr;
  double ddcr;

  c.sprandu(1000, 1, 0.3);
  for (SpCol<double>::iterator i = c.begin(); i != c.end(); ++i)
    {
    (*i) *= (std::numeric_limits<double>::max());
    }
  vec dc(c);

  cr = mean(c, 0);
  vec dcr = mean(dc, 0);

  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( (double) cr[0] == Approx(dcr[0]) );

  cr = mean(c, 1);
  dcr = mean(dc, 1);

  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( (double) cr[i] == Approx(dcr[i]) );
    }

  ddcr = mean(c);
  double dddr = mean(dc);

  REQUIRE( ddcr == Approx(dddr) );

  SpRow<double> r;
  r.sprandu(1, 1000, 0.3);
  for (SpRow<double>::iterator i = r.begin(); i != r.end(); ++i)
    {
    (*i) *= (std::numeric_limits<double>::max());
    }
  rowvec dr(r);

  SpRow<double> rr = mean(r, 0);
  rowvec drr = mean(dr, 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1000 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( (double) rr[i] == Approx(drr[i]) );
    }

  rr = mean(r, 1);
  drr = mean(dr, 1);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1 );
  REQUIRE( (double) rr[0] == Approx(drr[0]) );

  ddcr = mean(r);
  dddr = mean(dr);

  REQUIRE( ddcr == Approx(dddr) );
  }



TEST_CASE("fn_mean_robust_cx_sparse_vector_test")
  {
  // Test mean() on vectors.
  SpCol<std::complex<double> > c(1000);

  SpCol<std::complex<double> > cr;
  std::complex<double> ddcr;

  c.sprandu(1000, 1, 0.3);
  for (SpCol<std::complex<double> >::iterator i = c.begin(); i != c.end(); ++i)
    {
    (*i) *= (std::numeric_limits<double>::max());
    }
  cx_vec dc(c);

  cr = mean(c, 0);
  cx_vec dcr = mean(dc, 0);

  REQUIRE( cr.n_rows == 1 );
  REQUIRE( cr.n_cols == 1 );
  REQUIRE( real((std::complex<double>) cr[0]) == Approx(real(dcr[0])) );
  REQUIRE( imag((std::complex<double>) cr[0]) == Approx(imag(dcr[0])) );

  cr = mean(c, 1);
  dcr = mean(dc, 1);

  REQUIRE( cr.n_rows == 1000 );
  REQUIRE( cr.n_cols == 1 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( real((std::complex<double>) cr[i]) == Approx(real(dcr[i])) );
    REQUIRE( imag((std::complex<double>) cr[i]) == Approx(imag(dcr[i])) );
    }

  ddcr = mean(c);
  std::complex<double> dddr = mean(dc);

  REQUIRE( real(ddcr) == Approx(real(dddr)) );
  REQUIRE( imag(ddcr) == Approx(imag(dddr)) );

  SpRow<std::complex<double> > r;
  r.sprandu(1, 1000, 0.3);
  cx_rowvec dr(r);

  SpRow<std::complex<double> > rr = mean(r, 0);
  cx_rowvec drr = mean(dr, 0);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1000 );
  for (uword i = 0; i < 1000; ++i)
    {
    REQUIRE( real((std::complex<double>) rr[i]) == Approx(real(drr[i])) );
    REQUIRE( imag((std::complex<double>) rr[i]) == Approx(imag(drr[i])) );
    }

  rr = mean(r, 1);
  drr = mean(dr, 1);

  REQUIRE( rr.n_rows == 1 );
  REQUIRE( rr.n_cols == 1 );
  REQUIRE( real((std::complex<double>) rr[0]) == Approx(real(drr[0])) );
  REQUIRE( imag((std::complex<double>) rr[0]) == Approx(imag(drr[0])) );

  ddcr = mean(r);
  dddr = mean(dr);

  REQUIRE( real(ddcr) == Approx(real(dddr)) );
  REQUIRE( imag(ddcr) == Approx(imag(dddr)) );
  }



TEST_CASE("fn_mean_sparse_alias_test")
  {
  sp_mat s;
  s.sprandu(70, 70, 0.3);
  mat d(s);

  s = mean(s);
  d = mean(d);

  REQUIRE( d.n_rows == s.n_rows );
  REQUIRE( d.n_cols == s.n_cols );
  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx((double) s[i]) );
    }

  s.sprandu(70, 70, 0.3);
  d = s;

  s = mean(s, 1);
  d = mean(d, 1);
  for (uword i = 0; i < d.n_elem; ++i)
    {
    REQUIRE( d[i] == Approx((double) s[i]) );
    }
  }
