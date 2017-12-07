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

#if defined(ARMA_USE_SUPERLU)

TEST_CASE("fn_spsolve_sparse_test")
  {
  // We want to spsolve a system of equations, AX = B, where we want to recover
  // X and we have A and B, and A is sparse.
  for (size_t t = 0; t < 10; ++t)
    {
    const uword size = 5 * (t + 1);

    mat rX;
    rX.randu(size, size);

    sp_mat A;
    A.sprandu(size, size, 0.25);
    for (uword i = 0; i < size; ++i)
      {
      A(i, i) += rand();
      }

    mat B = A * rX;

    mat X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    mat dA(A);
    mat dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (double) X(j, i) == Approx((double) dX(j, i)) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_nonsymmetric_test")
  {
  for (size_t t = 0; t < 10; ++t)
    {
    const uword r_size = 5 * (t + 1);
    const uword c_size = 3 * (t + 4);

    mat rX;
    rX.randu(r_size, c_size);

    sp_mat A;
    A.sprandu(r_size, r_size, 0.25);
    for (uword i = 0; i < r_size; ++i)
      {
      A(i, i) += rand();
      }

    mat B = A * rX;

    mat X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    mat dA(A);
    mat dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (double) X(j, i) == Approx((double) dX(j, i)) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_float_test")
  {
  // We want to spsolve a system of equations, AX = B, where we want to recover
  // X and we have A and B, and A is sparse.
  for (size_t t = 0; t < 10; ++t)
    {
    const uword size = 5 * (t + 1);

    fmat rX;
    rX.randu(size, size);

    SpMat<float> A;
    A.sprandu(size, size, 0.25);
    for (uword i = 0; i < size; ++i)
      {
      A(i, i) += rand();
      }

    fmat B = A * rX;

    fmat X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    fmat dA(A);
    fmat dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (size_t i = 0; i < dX.n_cols; ++i)
      {
      for (size_t j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (float) X(j, i) == Approx((float) dX(j, i)) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_nonsymmetric_float_test")
  {
  for (size_t t = 0; t < 10; ++t)
    {
    const uword r_size = 5 * (t + 1);
    const uword c_size = 3 * (t + 4);

    fmat rX;
    rX.randu(r_size, c_size);

    SpMat<float> A;
    A.sprandu(r_size, r_size, 0.25);
    for (uword i = 0; i < r_size; ++i)
      {
      A(i, i) += rand();
      }

    fmat B = A * rX;

    fmat X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    fmat dA(A);
    fmat dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (float) X(j, i) == Approx((float) dX(j, i)) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_complex_float_test")
  {
  // We want to spsolve a system of equations, AX = B, where we want to recover
  // X and we have A and B, and A is sparse.
  for (size_t t = 0; t < 10; ++t)
    {
    const uword size = 5 * (t + 1);

    Mat<std::complex<float> > rX;
    rX.randu(size, size);

    SpMat<std::complex<float> > A;
    A.sprandu(size, size, 0.25);
    for(uword i = 0; i < size; ++i)
      {
      A(i, i) += rand();
      }

    Mat<std::complex<float> > B = A * rX;

    Mat<std::complex<float> > X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    Mat<std::complex<float> > dA(A);
    Mat<std::complex<float> > dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (float) std::abs((std::complex<float>) X(j, i)) ==
                 Approx((float) std::abs((std::complex<float>) dX(j, i))) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_nonsymmetric_complex_float_test")
  {
  for (size_t t = 0; t < 10; ++t)
    {
    const uword r_size = 5 * (t + 1);
    const uword c_size = 3 * (t + 4);

    Mat<std::complex<float> > rX;
    rX.randu(r_size, c_size);

    SpMat<std::complex<float> > A;
    A.sprandu(r_size, r_size, 0.25);
    for (uword i = 0; i < r_size; ++i)
      {
      A(i, i) += rand();
      }

    Mat<std::complex<float> > B = A * rX;

    Mat<std::complex<float> > X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    Mat<std::complex<float> > dA(A);
    Mat<std::complex<float> > dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (float) std::abs((std::complex<float>) X(j, i)) ==
                 Approx((float) std::abs((std::complex<float>) dX(j, i))) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_complex_test")
  {
  // We want to spsolve a system of equations, AX = B, where we want to recover
  // X and we have A and B, and A is sparse.
  for (size_t t = 0; t < 10; ++t)
    {
    const uword size = 5 * (t + 1);

    Mat<std::complex<double> > rX;
    rX.randu(size, size);

    SpMat<std::complex<double> > A;
    A.sprandu(size, size, 0.25);
    for (uword i = 0; i < size; ++i)
      {
      A(i, i) += rand();
      }

    Mat<std::complex<double> > B = A * rX;

    Mat<std::complex<double> > X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    Mat<std::complex<double> > dA(A);
    Mat<std::complex<double> > dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (double) std::abs((std::complex<double>) X(j, i)) ==
                 Approx((double) std::abs((std::complex<double>) dX(j, i))) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_sparse_nonsymmetric_complex_test")
  {
  for (size_t t = 0; t < 10; ++t)
    {
    const uword r_size = 5 * (t + 1);
    const uword c_size = 3 * (t + 4);

    Mat<std::complex<double> > rX;
    rX.randu(r_size, c_size);

    SpMat<std::complex<double> > A;
    A.sprandu(r_size, r_size, 0.25);
    for (uword i = 0; i < r_size; ++i)
      {
      A(i, i) += rand();
      }

    Mat<std::complex<double> > B = A * rX;

    Mat<std::complex<double> > X;
    bool result = spsolve(X, A, B);
    REQUIRE( result );

    // Dense solver.
    Mat<std::complex<double> > dA(A);
    Mat<std::complex<double> > dX = solve(dA, B);

    REQUIRE( X.n_cols == dX.n_cols );
    REQUIRE( X.n_rows == dX.n_rows );

    for (uword i = 0; i < dX.n_cols; ++i)
      {
      for (uword j = 0; j < dX.n_rows; ++j)
        {
        REQUIRE( (double) std::abs((std::complex<double>) X(j, i)) ==
                 Approx((double) std::abs((std::complex<double>) dX(j, i))) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_delayed_sparse_test")
  {
  const uword size = 10;

  mat rX;
  rX.randu(size, size);

  sp_mat A;
  A.sprandu(size, size, 0.25);
  for (uword i = 0; i < size; ++i)
    {
    A(i, i) += rand();
    }

  mat B = A * rX;

  mat X;
  bool result = spsolve(X, A, B);
  REQUIRE( result );

  mat dX = spsolve(A, B);

  REQUIRE( X.n_cols == dX.n_cols );
  REQUIRE( X.n_rows == dX.n_rows );

  for (uword i = 0; i < dX.n_cols; ++i)
    {
    for (uword j = 0; j < dX.n_rows; ++j)
      {
      REQUIRE( (double) X(j, i) == Approx((double) dX(j, i)) );
      }
    }
  }



TEST_CASE("fn_spsolve_superlu_solve_test")
  {
  // Solve this matrix, as in the examples:
  // [[19  0  21 21  0]
  //  [12 21   0  0  0]
  //  [ 0 12  16  0  0]
  //  [ 0  0   0  5 21]
  //  [12 12   0  0 18]]
  sp_mat b(5, 5);
  b(0, 0) = 19;
  b(0, 2) = 21;
  b(0, 3) = 21;
  b(1, 0) = 12;
  b(1, 1) = 21;
  b(2, 1) = 12;
  b(2, 2) = 16;
  b(3, 3) = 5;
  b(3, 4) = 21;
  b(4, 0) = 12;
  b(4, 1) = 12;
  b(4, 4) = 18;

  mat db(b);

  sp_mat a;
  a.eye(5, 5);
  mat da(a);

  mat x;
  spsolve(x, a, db);

  mat dx = solve(da, db);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      REQUIRE( (double) x(j, i) == Approx(dx(j, i)) );
      }
    }
  }



TEST_CASE("fn_spsolve_random_superlu_solve_test")
  {
  // Try to solve some random systems.
  const size_t iterations = 10;
  for (size_t it = 0; it < iterations; ++it)
    {
    sp_mat a;
    a.sprandu(50, 50, 0.3);
    sp_mat trueX;
    trueX.sprandu(50, 50, 0.3);

    sp_mat b = a * trueX;

    // Get things into the right format.
    mat db(b);

    mat x;

    spsolve(x, a, db);

    for (uword i = 0; i < x.n_cols; ++i)
      {
      for (uword j = 0; j < x.n_rows; ++j)
        {
        REQUIRE( x(j, i) == Approx((double) trueX(j, i)) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_float_superlu_solve_test")
  {
  // Solve this matrix, as in the examples:
  // [[19  0  21 21  0]
  //  [12 21   0  0  0]
  //  [ 0 12  16  0  0]
  //  [ 0  0   0  5 21]
  //  [12 12   0  0 18]]
  sp_fmat b(5, 5);
  b(0, 0) = 19;
  b(0, 2) = 21;
  b(0, 3) = 21;
  b(1, 0) = 12;
  b(1, 1) = 21;
  b(2, 1) = 12;
  b(2, 2) = 16;
  b(3, 3) = 5;
  b(3, 4) = 21;
  b(4, 0) = 12;
  b(4, 1) = 12;
  b(4, 4) = 18;

  fmat db(b);

  sp_fmat a;
  a.eye(5, 5);
  fmat da(a);

  fmat x;
  spsolve(x, a, db);

  fmat dx = solve(da, db);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      REQUIRE( (float) x(j, i) == Approx(dx(j, i)) );
      }
    }
  }



TEST_CASE("fn_spsolve_float_random_superlu_solve_test")
  {
  // Try to solve some random systems.
  const size_t iterations = 10;
  for (size_t it = 0; it < iterations; ++it)
    {
    sp_fmat a;
    a.sprandu(50, 50, 0.3);
    sp_fmat trueX;
    trueX.sprandu(50, 50, 0.3);

    sp_fmat b = a * trueX;

    // Get things into the right format.
    fmat db(b);

    fmat x;

    spsolve(x, a, db);

    for (uword i = 0; i < x.n_cols; ++i)
      {
      for (uword j = 0; j < x.n_rows; ++j)
        {
        if (std::abs(trueX(j, i)) < 0.001)
          REQUIRE( std::abs(x(j, i)) < 0.005 );
        else
          REQUIRE( trueX(j, i) == Approx((float) x(j, i)).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_float_superlu_solve_test")
  {
  // Solve this matrix, as in the examples:
  // [[19  0  21 21  0]
  //  [12 21   0  0  0]
  //  [ 0 12  16  0  0]
  //  [ 0  0   0  5 21]
  //  [12 12   0  0 18]] (imaginary part is the same)
  SpMat<std::complex<float> > b(5, 5);
  b(0, 0) = std::complex<float>(19, 19);
  b(0, 2) = std::complex<float>(21, 21);
  b(0, 3) = std::complex<float>(21, 21);
  b(1, 0) = std::complex<float>(12, 12);
  b(1, 1) = std::complex<float>(21, 21);
  b(2, 1) = std::complex<float>(12, 12);
  b(2, 2) = std::complex<float>(16, 16);
  b(3, 3) = std::complex<float>(5, 5);
  b(3, 4) = std::complex<float>(21, 21);
  b(4, 0) = std::complex<float>(12, 12);
  b(4, 1) = std::complex<float>(12, 12);
  b(4, 4) = std::complex<float>(18, 18);

  Mat<std::complex<float> > db(b);

  SpMat<std::complex<float> > a;
  a.eye(5, 5);
  Mat<std::complex<float> > da(a);

  Mat<std::complex<float> > x;
  spsolve(x, a, db);

  Mat<std::complex<float> > dx = solve(da, db);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      if (std::abs(x(j, i)) < 0.001 )
        {
        REQUIRE( std::abs(dx(j, i)) < 0.005 );
        }
      else
        {
        REQUIRE( ((std::complex<float>) x(j, i)).real() ==
                 Approx(dx(j, i).real()).epsilon(0.01) );
        REQUIRE( ((std::complex<float>) x(j, i)).imag() ==
                 Approx(dx(j, i).imag()).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_float_random_superlu_solve_test")
  {
  // Try to solve some random systems.
  const size_t iterations = 10;
  for (size_t it = 0; it < iterations; ++it)
    {
    SpMat<std::complex<float> > a;
    a.sprandu(50, 50, 0.3);
    SpMat<std::complex<float> > trueX;
    trueX.sprandu(50, 50, 0.3);

    SpMat<std::complex<float> > b = a * trueX;

    // Get things into the right format.
    Mat<std::complex<float> > db(b);

    Mat<std::complex<float> > x;

    spsolve(x, a, db);

    for (uword i = 0; i < x.n_cols; ++i)
      {
      for (uword j = 0; j < x.n_rows; ++j)
        {
        if (std::abs((std::complex<float>) trueX(j, i)) < 0.001 )
          {
          REQUIRE( std::abs(x(j, i)) < 0.001 );
          }
        else
          {
          REQUIRE( ((std::complex<float>) trueX(j, i)).real() ==
                   Approx(x(j, i).real()).epsilon(0.01) );
          REQUIRE( ((std::complex<float>) trueX(j, i)).imag() ==
                   Approx(x(j, i).imag()).epsilon(0.01) );
          }
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_superlu_solve_test")
  {
  // Solve this matrix, as in the examples:
  // [[19  0  21 21  0]
  //  [12 21   0  0  0]
  //  [ 0 12  16  0  0]
  //  [ 0  0   0  5 21]
  //  [12 12   0  0 18]] (imaginary part is the same)
  SpMat<std::complex<double> > b(5, 5);
  b(0, 0) = std::complex<double>(19, 19);
  b(0, 2) = std::complex<double>(21, 21);
  b(0, 3) = std::complex<double>(21, 21);
  b(1, 0) = std::complex<double>(12, 12);
  b(1, 1) = std::complex<double>(21, 21);
  b(2, 1) = std::complex<double>(12, 12);
  b(2, 2) = std::complex<double>(16, 16);
  b(3, 3) = std::complex<double>(5, 5);
  b(3, 4) = std::complex<double>(21, 21);
  b(4, 0) = std::complex<double>(12, 12);
  b(4, 1) = std::complex<double>(12, 12);
  b(4, 4) = std::complex<double>(18, 18);

  cx_mat db(b);

  sp_cx_mat a;
  a.eye(5, 5);
  cx_mat da(a);

  cx_mat x;
  spsolve(x, a, db);

  cx_mat dx = solve(da, db);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      if (std::abs(x(j, i)) < 0.001)
        {
        REQUIRE( std::abs(dx(j, i)) < 0.005 );
        }
      else
        {
        REQUIRE( ((std::complex<double>) x(j, i)).real() ==
                 Approx(dx(j, i).real()).epsilon(0.01) );
        REQUIRE( ((std::complex<double>) x(j, i)).imag() ==
                 Approx(dx(j, i).imag()).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_random_superlu_solve_test")
  {
  // Try to solve some random systems.
  const size_t iterations = 10;
  for (size_t it = 0; it < iterations; ++it)
    {
    sp_cx_mat a;
    a.sprandu(50, 50, 0.3);
    sp_cx_mat trueX;
    trueX.sprandu(50, 50, 0.3);

    sp_cx_mat b = a * trueX;

    // Get things into the right format.
    cx_mat db(b);

    cx_mat x;

    spsolve(x, a, db);

    for (uword i = 0; i < x.n_cols; ++i)
      {
      for (uword j = 0; j < x.n_rows; ++j)
        {
        if (std::abs((std::complex<double>) trueX(j, i)) < 0.001)
          {
          REQUIRE( std::abs(x(j, i)) < 0.005 );
          }
        else
          {
          REQUIRE( ((std::complex<double>) trueX(j, i)).real() ==
                   Approx(x(j, i).real()).epsilon(0.01) );
          REQUIRE( ((std::complex<double>) trueX(j, i)).imag() ==
                   Approx(x(j, i).imag()).epsilon(0.01) );
          }
        }
      }
    }
  }



TEST_CASE("fn_spsolve_function_test")
  {
  sp_mat a;
  a.sprandu(50, 50, 0.3);
  sp_mat trueX;
  trueX.sprandu(50, 50, 0.3);

  sp_mat b = a * trueX;

  // Get things into the right format.
  mat db(b);

  mat x;

  // Mostly these are compilation tests.
  spsolve(x, a, db);
  x = spsolve(a, db); // Test another overload.
  x = spsolve(a, db + 0.0);
  spsolve(x, a, db + 0.0);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      REQUIRE( (double) trueX(j, i) == Approx(x(j, i)) );
      }
    }
  }



TEST_CASE("fn_spsolve_float_function_test")
  {
  sp_fmat a;
  a.sprandu(50, 50, 0.3);
  sp_fmat trueX;
  trueX.sprandu(50, 50, 0.3);

  sp_fmat b = a * trueX;

  // Get things into the right format.
  fmat db(b);

  fmat x;

  // Mostly these are compilation tests.
  spsolve(x, a, db);
  x = spsolve(a, db); // Test another overload.
  x = spsolve(a, db + 0.0);
  spsolve(x, a, db + 0.0);

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      if (std::abs(trueX(j, i)) < 0.001)
        {
        REQUIRE( std::abs(x(j, i)) < 0.001 );
        }
      else
        {
        REQUIRE( (float) trueX(j, i) == Approx(x(j, i)).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_function_test")
  {
  sp_cx_mat a;
  a.sprandu(50, 50, 0.3);
  sp_cx_mat trueX;
  trueX.sprandu(50, 50, 0.3);

  sp_cx_mat b = a * trueX;

  // Get things into the right format.
  cx_mat db(b);

  cx_mat x;

  // Mostly these are compilation tests.
  spsolve(x, a, db);
  x = spsolve(a, db); // Test another overload.
  x = spsolve(a, db + std::complex<double>(0.0));
  spsolve(x, a, db + std::complex<double>(0.0));

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      if (std::abs((std::complex<double>) trueX(j, i)) < 0.001)
        {
        REQUIRE( std::abs(x(j, i)) < 0.005 );
        }
      else
        {
        REQUIRE( ((std::complex<double>) trueX(j, i)).real() ==
                 Approx(x(j, i).real()).epsilon(0.01) );
        REQUIRE( ((std::complex<double>) trueX(j, i)).imag() ==
                 Approx(x(j, i).imag()).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_spsolve_cx_float_function_test")
  {
  sp_cx_fmat a;
  a.sprandu(50, 50, 0.3);
  sp_cx_fmat trueX;
  trueX.sprandu(50, 50, 0.3);

  sp_cx_fmat b = a * trueX;

  // Get things into the right format.
  cx_fmat db(b);

  cx_fmat x;

  // Mostly these are compilation tests.
  spsolve(x, a, db);
  x = spsolve(a, db); // Test another overload.
  x = spsolve(a, db + std::complex<float>(0.0));
  spsolve(x, a, db + std::complex<float>(0.0));

  for (uword i = 0; i < x.n_cols; ++i)
    {
    for (uword j = 0; j < x.n_rows; ++j)
      {
      if (std::abs((std::complex<float>) trueX(j, i)) < 0.001 )
        {
        REQUIRE( std::abs(x(j, i)) < 0.005 );
        }
      else
        {
        REQUIRE( ((std::complex<float>) trueX(j, i)).real() ==
                 Approx(x(j, i).real()).epsilon(0.01) );
        REQUIRE( ((std::complex<float>) trueX(j, i)).imag() ==
                 Approx(x(j, i).imag()).epsilon(0.01) );
        }
      }
    }
  }

#endif
