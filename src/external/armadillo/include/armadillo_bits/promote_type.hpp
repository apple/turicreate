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


//! \addtogroup promote_type
//! @{


template<typename T1, typename T2>
struct is_promotable
  {
  static const bool value = false;
  typedef T1 result;
  };


struct is_promotable_ok
  {
  static const bool value = true;
  };


template<typename T> struct is_promotable<T,               T> : public is_promotable_ok { typedef T               result; };
template<typename T> struct is_promotable<std::complex<T>, T> : public is_promotable_ok { typedef std::complex<T> result; };

template<> struct is_promotable<std::complex<double>, std::complex<float> > : public is_promotable_ok { typedef std::complex<double> result; };
template<> struct is_promotable<std::complex<double>, float>                : public is_promotable_ok { typedef std::complex<double> result; };
template<> struct is_promotable<std::complex<float>,  double>               : public is_promotable_ok { typedef std::complex<double> result; };


#if defined(ARMA_USE_U64S64)
template<typename t> struct is_promotable<std::complex<t>, u64>    : public is_promotable_ok { typedef std::complex<t> result; };
template<typename t> struct is_promotable<std::complex<t>, s64>    : public is_promotable_ok { typedef std::complex<t> result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<typename t> struct is_promotable<std::complex<t>, ulng_t> : public is_promotable_ok { typedef std::complex<t> result; };
template<typename t> struct is_promotable<std::complex<t>, slng_t> : public is_promotable_ok { typedef std::complex<t> result; };
#endif
template<typename T> struct is_promotable<std::complex<T>, s32>    : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<std::complex<T>, u32>    : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<std::complex<T>, s16>    : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<std::complex<T>, u16>    : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<std::complex<T>, s8>     : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<std::complex<T>, u8>     : public is_promotable_ok { typedef std::complex<T> result; };


template<> struct is_promotable<double, float > : public is_promotable_ok { typedef double result; };
#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<double, s64   > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, u64   > : public is_promotable_ok { typedef double result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct is_promotable<double, slng_t> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, ulng_t> : public is_promotable_ok { typedef double result; };
#endif
template<> struct is_promotable<double, s32   > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, u32   > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, s16   > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, u16   > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, s8    > : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<double, u8    > : public is_promotable_ok { typedef double result; };

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<float, s64   > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, u64   > : public is_promotable_ok { typedef float result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct is_promotable<float, slng_t> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, ulng_t> : public is_promotable_ok { typedef float result; };
#endif
template<> struct is_promotable<float, s32   > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, u32   > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, s16   > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, u16   > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, s8    > : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<float, u8    > : public is_promotable_ok { typedef float result; };

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<u64, u32> : public is_promotable_ok { typedef u64 result; };
template<> struct is_promotable<u64, u16> : public is_promotable_ok { typedef u64 result; };
template<> struct is_promotable<u64, u8 > : public is_promotable_ok { typedef u64 result; };
#endif

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<s64, u64> : public is_promotable_ok { typedef s64 result; };  // float ?
template<> struct is_promotable<s64, u32> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s64, s32> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s64, s16> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s64, u16> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s64, s8 > : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s64, u8 > : public is_promotable_ok { typedef s64 result; };
#endif

template<> struct is_promotable<s32, u32> : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<s32, s16> : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<s32, u16> : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<s32, s8 > : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<s32, u8 > : public is_promotable_ok { typedef s32 result; };

template<> struct is_promotable<u32, s16> : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<u32, u16> : public is_promotable_ok { typedef u32 result; };
template<> struct is_promotable<u32, s8 > : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<u32, u8 > : public is_promotable_ok { typedef u32 result; };

template<> struct is_promotable<s16, u16> : public is_promotable_ok { typedef s16 result; };  // s32 ?
template<> struct is_promotable<s16, s8 > : public is_promotable_ok { typedef s16 result; };
template<> struct is_promotable<s16, u8 > : public is_promotable_ok { typedef s16 result; };

template<> struct is_promotable<u16, s8> : public is_promotable_ok { typedef s16 result; };  // s32 ?
template<> struct is_promotable<u16, u8> : public is_promotable_ok { typedef u16 result; };

template<> struct is_promotable<s8, u8> : public is_promotable_ok { typedef s8 result; };  // s16 ?




//
// mirrored versions

template<typename T> struct is_promotable<T, std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };

template<> struct is_promotable<std::complex<float>, std::complex<double> > : public is_promotable_ok { typedef std::complex<double> result; };
template<> struct is_promotable<float,               std::complex<double> > : public is_promotable_ok { typedef std::complex<double> result; };
template<> struct is_promotable<double,              std::complex<float>  > : public is_promotable_ok { typedef std::complex<double> result; };

#if defined(ARMA_USE_U64S64)
template<typename T> struct is_promotable<s64,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<u64,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<typename T> struct is_promotable<slng_t, std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<ulng_t, std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
#endif
template<typename T> struct is_promotable<s32,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<u32,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<s16,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<u16,    std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<s8,     std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };
template<typename T> struct is_promotable<u8,     std::complex<T> > : public is_promotable_ok { typedef std::complex<T> result; };


template<> struct is_promotable<float,  double> : public is_promotable_ok { typedef double result; };
#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<s64,    double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<u64,    double> : public is_promotable_ok { typedef double result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct is_promotable<slng_t, double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<ulng_t, double> : public is_promotable_ok { typedef double result; };
#endif
template<> struct is_promotable<s32,    double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<u32,    double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<s16,    double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<u16,    double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<s8,     double> : public is_promotable_ok { typedef double result; };
template<> struct is_promotable<u8,     double> : public is_promotable_ok { typedef double result; };

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<s64,    float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<u64,    float> : public is_promotable_ok { typedef float result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct is_promotable<slng_t, float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<ulng_t, float> : public is_promotable_ok { typedef float result; };
#endif
template<> struct is_promotable<s32,    float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<u32,    float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<s16,    float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<u16,    float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<s8,     float> : public is_promotable_ok { typedef float result; };
template<> struct is_promotable<u8,     float> : public is_promotable_ok { typedef float result; };

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<u32, u64> : public is_promotable_ok { typedef u64 result; };
template<> struct is_promotable<u16, u64> : public is_promotable_ok { typedef u64 result; };
template<> struct is_promotable<u8,  u64> : public is_promotable_ok { typedef u64 result; };
#endif

#if defined(ARMA_USE_U64S64)
template<> struct is_promotable<u64, s64> : public is_promotable_ok { typedef s64 result; };  // float ?
template<> struct is_promotable<s32, s64> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<u32, s64> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s16, s64> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<u16, s64> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<s8 , s64> : public is_promotable_ok { typedef s64 result; };
template<> struct is_promotable<u8 , s64> : public is_promotable_ok { typedef s64 result; };
#endif

template<> struct is_promotable<u32, s32> : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<s16, s32> : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<u16, s32> : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<s8 , s32> : public is_promotable_ok { typedef s32 result; };
template<> struct is_promotable<u8 , s32> : public is_promotable_ok { typedef s32 result; };

template<> struct is_promotable<s16, u32> : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<u16, u32> : public is_promotable_ok { typedef u32 result; };
template<> struct is_promotable<s8 , u32> : public is_promotable_ok { typedef s32 result; };  // float ?
template<> struct is_promotable<u8 , u32> : public is_promotable_ok { typedef u32 result; };

template<> struct is_promotable<u16, s16> : public is_promotable_ok { typedef s16 result; };  // s32 ?
template<> struct is_promotable<s8 , s16> : public is_promotable_ok { typedef s16 result; };
template<> struct is_promotable<u8 , s16> : public is_promotable_ok { typedef s16 result; };

template<> struct is_promotable<s8, u16> : public is_promotable_ok { typedef s16 result; };  // s32 ?
template<> struct is_promotable<u8, u16> : public is_promotable_ok { typedef u16 result; };

template<> struct is_promotable<u8, s8> : public is_promotable_ok { typedef s8 result; };  // s16 ?





template<typename T1, typename T2>
struct promote_type
  {
  inline static void check()
    {
    arma_type_check(( is_promotable<T1,T2>::value == false ));
    }

  typedef typename is_promotable<T1,T2>::result result;
  };



template<typename T1, typename T2>
struct eT_promoter
  {
  typedef typename promote_type<typename T1::elem_type, typename T2::elem_type>::result eT;
  };



//! @}
