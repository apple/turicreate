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

#if defined(ARMA_USE_HDF5)

TEST_CASE("hdf5_u8_test")
  {
  arma::Mat<u8> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<u8> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<u8> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_u16_test")
  {
  arma::Mat<u16> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<u16> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<u16> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_u32_test")
  {
  arma::Mat<u32> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<u32> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<u32> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



#ifdef ARMA_USE_U64S64
TEST_CASE("hdf5_u64_test")
  {
  arma::Mat<u64> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<u64> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<u64> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }
#endif



TEST_CASE("hdf5_s8_test")
  {
  arma::Mat<s8> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<s8> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<s8> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_s16_test")
  {
  arma::Mat<s16> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<s16> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<s16> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_s32_test")
  {
  arma::Mat<s32> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<s32> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<s32> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



#ifdef ARMA_USE_U64S64
TEST_CASE("hdf5_s64_test")
  {
  arma::Mat<s64> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<s64> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<s64> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }
#endif



TEST_CASE("hdf5_char_test")
  {
  arma::Mat<char> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<char> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<char> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_int_test")
  {
  arma::Mat<signed int> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<signed int> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<signed int> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_uint_test")
  {
  arma::Mat<unsigned int> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<unsigned int> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<unsigned int> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_short_test")
  {
  arma::Mat<signed short> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<signed short> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<signed short> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_ushort_test")
  {
  arma::Mat<unsigned short> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<unsigned short> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<unsigned short> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_long_test")
  {
  arma::Mat<signed long> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<signed long> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<signed long> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_ulong_test")
  {
  arma::Mat<unsigned long> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<unsigned long> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<unsigned long> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



#ifdef ARMA_USE_U64S64
TEST_CASE("hdf5_llong_test")
  {
  arma::Mat<signed long long> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<signed long long> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<signed long long> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_ullong_test")
  {
  arma::Mat<unsigned long long> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<unsigned long long> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<unsigned long long> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }
#endif



TEST_CASE("hdf5_float_test")
  {
  arma::Mat<float> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<float> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<float> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_double_test")
  {
  arma::Mat<double> a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<double> b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<double> c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_complex_float_test")
  {
  arma::Mat<std::complex<float> > a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<std::complex<float> > b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == b[i] );
    }

  // Now autoload.
  arma::Mat<std::complex<float> > c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }



TEST_CASE("hdf5_complex_double_test")
  {
  arma::Mat<std::complex<double> > a;
  a.randu(20, 20);

  // Save first.
  a.save("file.h5", hdf5_binary);

  // Load as different matrix.
  arma::Mat<std::complex<double> > b;
  b.load("file.h5", hdf5_binary);

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    REQUIRE( a[i] == b[i] );

  // Now autoload.
  arma::Mat<std::complex<double> > c;
  c.load("file.h5");

  // Check that they are the same.
  for (uword i = 0; i < a.n_elem; ++i)
    {
    REQUIRE( a[i] == c[i] );
    }

  remove("file.h5");
  }

#endif
