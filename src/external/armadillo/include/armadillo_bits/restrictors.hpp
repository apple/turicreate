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


//! \addtogroup restrictors
//! @{



// structures for template based restrictions of input/output arguments
// (part of the SFINAE approach)
// http://en.wikipedia.org/wiki/SFINAE


template<typename T> struct arma_scalar_only { };

template<> struct arma_scalar_only<u8>     { typedef u8     result; };
template<> struct arma_scalar_only<s8>     { typedef s8     result; };
template<> struct arma_scalar_only<u16>    { typedef u16    result; };
template<> struct arma_scalar_only<s16>    { typedef s16    result; };
template<> struct arma_scalar_only<u32>    { typedef u32    result; };
template<> struct arma_scalar_only<s32>    { typedef s32    result; };
#if defined(ARMA_USE_U64S64)
template<> struct arma_scalar_only<u64>    { typedef u64    result; };
template<> struct arma_scalar_only<s64>    { typedef s64    result; };
#endif
template<> struct arma_scalar_only<float>  { typedef float  result; };
template<> struct arma_scalar_only<double> { typedef double result; };
#if defined(ARMA_ALLOW_LONG)
template<> struct arma_scalar_only<ulng_t> { typedef ulng_t result; };
template<> struct arma_scalar_only<slng_t> { typedef slng_t result; };
#endif

template<typename T>
struct arma_scalar_only< std::complex<T> > { typedef std::complex<T> result; };



template<typename T> struct arma_integral_only { };

template<> struct arma_integral_only<u8>  { typedef u8  result; };
template<> struct arma_integral_only<s8>  { typedef s8  result; };
template<> struct arma_integral_only<u16> { typedef u16 result; };
template<> struct arma_integral_only<s16> { typedef s16 result; };
template<> struct arma_integral_only<u32> { typedef u32 result; };
template<> struct arma_integral_only<s32> { typedef s32 result; };
#if defined(ARMA_USE_U64S64)
template<> struct arma_integral_only<u64> { typedef u64 result; };
template<> struct arma_integral_only<s64> { typedef s64 result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct arma_integral_only<ulng_t> { typedef ulng_t result; };
template<> struct arma_integral_only<slng_t> { typedef slng_t result; };
#endif



template<typename T> struct arma_unsigned_integral_only { };

template<> struct arma_unsigned_integral_only<u8>     { typedef u8     result; };
template<> struct arma_unsigned_integral_only<u16>    { typedef u16    result; };
template<> struct arma_unsigned_integral_only<u32>    { typedef u32    result; };
#if defined(ARMA_USE_U64S64)
template<> struct arma_unsigned_integral_only<u64>    { typedef u64    result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct arma_unsigned_integral_only<ulng_t> { typedef ulng_t result; };
#endif



template<typename T> struct arma_signed_integral_only { };

template<> struct arma_signed_integral_only<s8>     { typedef s8     result; };
template<> struct arma_signed_integral_only<s16>    { typedef s16    result; };
template<> struct arma_signed_integral_only<s32>    { typedef s32    result; };
#if defined(ARMA_USE_U64S64)
template<> struct arma_signed_integral_only<s64>    { typedef s64    result; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct arma_signed_integral_only<slng_t> { typedef slng_t result; };
#endif



template<typename T> struct arma_signed_only { };

template<> struct arma_signed_only<s8>     { typedef s8     result; };
template<> struct arma_signed_only<s16>    { typedef s16    result; };
template<> struct arma_signed_only<s32>    { typedef s32    result; };
#if defined(ARMA_USE_U64S64)
template<> struct arma_signed_only<s64>    { typedef s64    result; };
#endif
template<> struct arma_signed_only<float>  { typedef float  result; };
template<> struct arma_signed_only<double> { typedef double result; };
#if defined(ARMA_ALLOW_LONG)
template<> struct arma_signed_only<slng_t> { typedef slng_t result; };
#endif

template<typename T> struct arma_signed_only< std::complex<T> > { typedef std::complex<T> result; };



template<typename T> struct arma_real_only { };

template<> struct arma_real_only<float>  { typedef float  result; };
template<> struct arma_real_only<double> { typedef double result; };


template<typename T> struct arma_real_or_cx_only { };

template<> struct arma_real_or_cx_only< float >                { typedef float                result; };
template<> struct arma_real_or_cx_only< double >               { typedef double               result; };
template<> struct arma_real_or_cx_only< std::complex<float>  > { typedef std::complex<float>  result; };
template<> struct arma_real_or_cx_only< std::complex<double> > { typedef std::complex<double> result; };



template<typename T> struct arma_cx_only { };

template<> struct arma_cx_only< std::complex<float>  > { typedef std::complex<float>  result; };
template<> struct arma_cx_only< std::complex<double> > { typedef std::complex<double> result; };



template<typename T> struct arma_not_cx                    { typedef T result; };
template<typename T> struct arma_not_cx< std::complex<T> > { };



template<typename T> struct arma_blas_type_only { };

template<> struct arma_blas_type_only< float                > { typedef float                result; };
template<> struct arma_blas_type_only< double               > { typedef double               result; };
template<> struct arma_blas_type_only< std::complex<float>  > { typedef std::complex<float>  result; };
template<> struct arma_blas_type_only< std::complex<double> > { typedef std::complex<double> result; };



template<typename T> struct arma_not_blas_type { typedef T result; };

template<> struct arma_not_blas_type< float                > {  };
template<> struct arma_not_blas_type< double               > {  };
template<> struct arma_not_blas_type< std::complex<float>  > {  };
template<> struct arma_not_blas_type< std::complex<double> > {  };



template<typename T> struct arma_op_rel_only { };

template<> struct arma_op_rel_only< op_rel_lt_pre    > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_lt_post   > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_gt_pre    > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_gt_post   > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_lteq_pre  > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_lteq_post > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_gteq_pre  > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_gteq_post > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_eq        > { typedef int result; };
template<> struct arma_op_rel_only< op_rel_noteq     > { typedef int result; };



template<typename T> struct arma_not_op_rel { typedef int result; };

template<> struct arma_not_op_rel< op_rel_lt_pre    > { };
template<> struct arma_not_op_rel< op_rel_lt_post   > { };
template<> struct arma_not_op_rel< op_rel_gt_pre    > { };
template<> struct arma_not_op_rel< op_rel_gt_post   > { };
template<> struct arma_not_op_rel< op_rel_lteq_pre  > { };
template<> struct arma_not_op_rel< op_rel_lteq_post > { };
template<> struct arma_not_op_rel< op_rel_gteq_pre  > { };
template<> struct arma_not_op_rel< op_rel_gteq_post > { };
template<> struct arma_not_op_rel< op_rel_eq        > { };
template<> struct arma_not_op_rel< op_rel_noteq     > { };



template<typename T> struct arma_glue_rel_only { };

template<> struct arma_glue_rel_only< glue_rel_lt    > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_gt    > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_lteq  > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_gteq  > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_eq    > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_noteq > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_and   > { typedef int result; };
template<> struct arma_glue_rel_only< glue_rel_or    > { typedef int result; };



template<typename T> struct arma_Mat_Col_Row_only { };

template<typename eT> struct arma_Mat_Col_Row_only< Mat<eT> > { typedef Mat<eT> result; };
template<typename eT> struct arma_Mat_Col_Row_only< Col<eT> > { typedef Col<eT> result; };
template<typename eT> struct arma_Mat_Col_Row_only< Row<eT> > { typedef Row<eT> result; };



template<typename  T> struct arma_Cube_only             { };
template<typename eT> struct arma_Cube_only< Cube<eT> > { typedef Cube<eT> result; };


template<typename T> struct arma_SpMat_SpCol_SpRow_only { };

template<typename eT> struct arma_SpMat_SpCol_SpRow_only< SpMat<eT> > { typedef SpMat<eT> result; };
template<typename eT> struct arma_SpMat_SpCol_SpRow_only< SpCol<eT> > { typedef SpCol<eT> result; };
template<typename eT> struct arma_SpMat_SpCol_SpRow_only< SpRow<eT> > { typedef SpRow<eT> result; };



template<bool> struct enable_if       {                     };
template<>     struct enable_if<true> { typedef int result; };


template<bool, typename result_type > struct enable_if2                    {                             };
template<      typename result_type > struct enable_if2<true, result_type> { typedef result_type result; };



//! @}
