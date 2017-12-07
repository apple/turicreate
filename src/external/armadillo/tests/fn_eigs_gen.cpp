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

TEST_CASE("fn_eigs_gen_odd_test")
  {
  const uword n_rows = 10;
  const uword n_eigval = 5;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    sp_mat m;
    m.sprandu(n_rows, n_rows, 0.3);
    mat d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<double> > sp_eigval;
    Mat< std::complex<double> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<double> > eigval;
    Mat< std::complex<double> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<double>(sp_eigval[i]).real() - eigval[k].real()) < 1e-4) &&
            (std::abs(std::complex<double>(sp_eigval[i]).imag() - eigval[k].imag()) < 1e-4) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.1) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.1) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_even_test")
  {
  const uword n_rows = 10;
  const uword n_eigval = 4;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    sp_mat m;
    m.sprandu(n_rows, n_rows, 0.3);
    sp_mat z(5, 5);
    z.sprandu(5, 5, 0.5);
    m.submat(2, 2, 6, 6) += 5 * z;
    mat d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<double> > sp_eigval;
    Mat< std::complex<double> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<double> > eigval;
    Mat< std::complex<double> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<double>(sp_eigval[i]).real() - eigval[k].real()) < 1e-4) &&
            (std::abs(std::complex<double>(sp_eigval[i]).imag() - eigval[k].imag()) < 1e-4) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_odd_float_test")
  {
  const uword n_rows = 10;
  const uword n_eigval = 5;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat<float> m;
    m.sprandu(n_rows, n_rows, 0.3);
    for (uword i = 0; i < n_rows; ++i)
      {
      m(i, i) += 5 * double(i) / double(n_rows);
      }
    Mat<float> d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<float> > sp_eigval;
    Mat< std::complex<float> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<float> > eigval;
    Mat< std::complex<float> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<float>(sp_eigval[i]).real() - eigval[k].real()) < 0.001) &&
            (std::abs(std::complex<float>(sp_eigval[i]).imag() - eigval[k].imag()) < 0.001) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.001) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_even_float_test")
  {
  const uword n_rows = 12;
  const uword n_eigval = 8;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat<float> m;
    m.sprandu(n_rows, n_rows, 0.3);
    for (uword i = 0; i < n_rows; ++i)
      {
      m(i, i) += 5 * double(i) / double(n_rows);
      }
    Mat<float> d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<float> > sp_eigval;
    Mat< std::complex<float> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<float> > eigval;
    Mat< std::complex<float> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<float>(sp_eigval[i]).real() - eigval[k].real()) < 0.001) &&
            (std::abs(std::complex<float>(sp_eigval[i]).imag() - eigval[k].imag()) < 0.001) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_odd_complex_float_test")
  {
  const uword n_rows = 10;
  const uword n_eigval = 5;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat< std::complex<float> > m;
    m.sprandu(n_rows, n_rows, 0.3);
    Mat< std::complex<float> > d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<float> > sp_eigval;
    Mat< std::complex<float> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<float> > eigval;
    Mat< std::complex<float> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<float>(sp_eigval[i]).real() - eigval[k].real()) < 0.001) &&
            (std::abs(std::complex<float>(sp_eigval[i]).imag() - eigval[k].imag()) < 0.001) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_even_complex_float_test")
  {
  const uword n_rows = 12;
  const uword n_eigval = 8;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat< std::complex<float> > m;
    m.sprandu(n_rows, n_rows, 0.3);
    Mat< std::complex<float> > d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<float> > sp_eigval;
    Mat< std::complex<float> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<float> > eigval;
    Mat< std::complex<float> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<float>(sp_eigval[i]).real() - eigval[k].real()) < 0.001) &&
            (std::abs(std::complex<float>(sp_eigval[i]).imag() - eigval[k].imag()) < 0.001) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("eigs_gen_odd_complex_test")
  {
  const uword n_rows = 10;
  const uword n_eigval = 5;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat< std::complex<double> > m;
    m.sprandu(n_rows, n_rows, 0.3);
    Mat< std::complex<double> > d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<double> > sp_eigval;
    Mat< std::complex<double> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<double> > eigval;
    Mat< std::complex<double> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<double>(sp_eigval[i]).real() - eigval[k].real()) < 1e-10) &&
            (std::abs(std::complex<double>(sp_eigval[i]).imag() - eigval[k].imag()) < 1e-10) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (size_t j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }



TEST_CASE("fn_eigs_gen_even_complex_test")
  {
  const uword n_rows = 15;
  const uword n_eigval = 6;
  for (size_t trial = 0; trial < 10; ++trial)
    {
    SpMat< std::complex<double> > m;
    m.sprandu(n_rows, n_rows, 0.3);
    Mat< std::complex<double> > d(m);

    // Eigendecompose, getting first 5 eigenvectors.
    Col< std::complex<double> > sp_eigval;
    Mat< std::complex<double> > sp_eigvec;
    eigs_gen(sp_eigval, sp_eigvec, m, n_eigval);

    // Do the same for the dense case.
    Col< std::complex<double> > eigval;
    Mat< std::complex<double> > eigvec;
    eig_gen(eigval, eigvec, d);

    uvec used(n_rows);
    used.fill(0);

    for (size_t i = 0; i < n_eigval; ++i)
      {
      // Sorting these is a super bitch.
      // Find which one is the likely dense eigenvalue.
      uword dense_eval = n_rows + 1;
      for (uword k = 0; k < n_rows; ++k)
        {
        if ((std::abs(std::complex<double>(sp_eigval[i]).real() - eigval[k].real()) < 1e-10) &&
            (std::abs(std::complex<double>(sp_eigval[i]).imag() - eigval[k].imag()) < 1e-10) &&
            (used[k] == 0))
          {
          dense_eval = k;
          used[k] = 1;
          break;
          }
        }

      REQUIRE( dense_eval != n_rows + 1 );

      REQUIRE( std::abs(sp_eigval[i]) == Approx(std::abs(eigval[dense_eval])).epsilon(0.01) );
      for (uword j = 0; j < n_rows; ++j)
        {
        REQUIRE( std::abs(sp_eigvec(j, i)) == Approx(std::abs(eigvec(j, dense_eval))).epsilon(0.01) );
        }
      }
    }
  }
