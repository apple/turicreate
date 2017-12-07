// Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2008-2016 National ICT Australia (NICTA)
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


//! \addtogroup arma_rng_cxx98
//! @{



class arma_rng_cxx98
  {
  public:

  typedef unsigned int seed_type;

  inline static void set_seed(const seed_type val);

  arma_inline static int    randi_val();
  arma_inline static double randu_val();
       inline static double randn_val();

  template<typename eT>
  inline static void randn_dual_val(eT& out1, eT& out2);

  template<typename eT>
  inline static void randi_fill(eT* mem, const uword N, const int a, const int b);

  inline static int randi_max_val();
  };



inline
void
arma_rng_cxx98::set_seed(const arma_rng_cxx98::seed_type val)
  {
  std::srand(val);
  }



arma_inline
int
arma_rng_cxx98::randi_val()
  {
  #if (RAND_MAX == 32767)
    {
    u32 val1 = u32(std::rand());
    u32 val2 = u32(std::rand());

    val1 <<= 15;

    return (val1 | val2);
    }
  #else
    {
    return std::rand();
    }
  #endif
  }



arma_inline
double
arma_rng_cxx98::randu_val()
  {
  return double( double(randi_val()) * ( double(1) / double(randi_max_val()) ) );
  }



inline
double
arma_rng_cxx98::randn_val()
  {
  // polar form of the Box-Muller transformation:
  // http://en.wikipedia.org/wiki/Box-Muller_transformation
  // http://en.wikipedia.org/wiki/Marsaglia_polar_method

  double tmp1;
  double tmp2;
  double w;

  do
    {
    tmp1 = double(2) * double(randi_val()) * (double(1) / double(randi_max_val())) - double(1);
    tmp2 = double(2) * double(randi_val()) * (double(1) / double(randi_max_val())) - double(1);

    w = tmp1*tmp1 + tmp2*tmp2;
    }
  while ( w >= double(1) );

  return double( tmp1 * std::sqrt( (double(-2) * std::log(w)) / w) );
  }



template<typename eT>
inline
void
arma_rng_cxx98::randn_dual_val(eT& out1, eT& out2)
  {
  // make sure we are internally using at least floats
  typedef typename promote_type<eT,float>::result eTp;

  eTp tmp1;
  eTp tmp2;
  eTp w;

  do
    {
    tmp1 = eTp(2) * eTp(randi_val()) * (eTp(1) / eTp(randi_max_val())) - eTp(1);
    tmp2 = eTp(2) * eTp(randi_val()) * (eTp(1) / eTp(randi_max_val())) - eTp(1);

    w = tmp1*tmp1 + tmp2*tmp2;
    }
  while ( w >= eTp(1) );

  const eTp k = std::sqrt( (eTp(-2) * std::log(w)) / w);

  out1 = eT(tmp1*k);
  out2 = eT(tmp2*k);
  }



template<typename eT>
inline
void
arma_rng_cxx98::randi_fill(eT* mem, const uword N, const int a, const int b)
  {
  if( (a == 0) && (b == RAND_MAX) )
    {
    for(uword i=0; i<N; ++i)
      {
      mem[i] = eT(std::rand());
      }
    }
  else
    {
    const uword length = uword(b - a + 1);

    const double scale = double(length) / double(randi_max_val());

    for(uword i=0; i<N; ++i)
      {
      mem[i] = eT((std::min)( b, (int( double(randi_val()) * scale ) + a) ));
      }
    }
  }



inline
int
arma_rng_cxx98::randi_max_val()
  {
  #if (RAND_MAX == 32767)
    return ( (32767 << 15) + 32767);
  #else
    return RAND_MAX;
  #endif
  }



//! @}
