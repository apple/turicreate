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


//! \addtogroup traits
//! @{


template<typename T1>
struct get_pod_type
  { typedef T1 result; };

template<typename T2>
struct get_pod_type< std::complex<T2> >
  { typedef T2 result; };



template<typename T>
struct is_Mat_fixed_only
  {
  typedef char yes[1];
  typedef char no[2];

  template<typename X> static yes& check(typename X::Mat_fixed_type*);
  template<typename>   static no&  check(...);

  static const bool value = ( sizeof(check<T>(0)) == sizeof(yes) );
  };



template<typename T>
struct is_Row_fixed_only
  {
  typedef char yes[1];
  typedef char no[2];

  template<typename X> static yes& check(typename X::Row_fixed_type*);
  template<typename>   static no&  check(...);

  static const bool value = ( sizeof(check<T>(0)) == sizeof(yes) );
  };



template<typename T>
struct is_Col_fixed_only
  {
  typedef char yes[1];
  typedef char no[2];

  template<typename X> static yes& check(typename X::Col_fixed_type*);
  template<typename>   static no&  check(...);

  static const bool value = ( sizeof(check<T>(0)) == sizeof(yes) );
  };



template<typename T>
struct is_Mat_fixed
  { static const bool value = ( is_Mat_fixed_only<T>::value || is_Row_fixed_only<T>::value || is_Col_fixed_only<T>::value ); };



template<typename T>
struct is_Mat_only
  { static const bool value = is_Mat_fixed_only<T>::value; };

template<typename eT>
struct is_Mat_only< Mat<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat_only< const Mat<eT> >
  { static const bool value = true; };



template<typename T>
struct is_Mat
  { static const bool value = ( is_Mat_fixed_only<T>::value || is_Row_fixed_only<T>::value || is_Col_fixed_only<T>::value ); };

template<typename eT>
struct is_Mat< Mat<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat< const Mat<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat< Row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat< const Row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat< Col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Mat< const Col<eT> >
  { static const bool value = true; };



template<typename T>
struct is_Row
  { static const bool value = is_Row_fixed_only<T>::value; };

template<typename eT>
struct is_Row< Row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Row< const Row<eT> >
  { static const bool value = true; };



template<typename T>
struct is_Col
  { static const bool value = is_Col_fixed_only<T>::value; };

template<typename eT>
struct is_Col< Col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Col< const Col<eT> >
  { static const bool value = true; };



template<typename T>
struct is_diagview
  { static const bool value = false; };

template<typename eT>
struct is_diagview< diagview<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_diagview< const diagview<eT> >
  { static const bool value = true; };


template<typename T>
struct is_subview
  { static const bool value = false; };

template<typename eT>
struct is_subview< subview<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_subview< const subview<eT> >
  { static const bool value = true; };


template<typename T>
struct is_subview_row
  { static const bool value = false; };

template<typename eT>
struct is_subview_row< subview_row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_subview_row< const subview_row<eT> >
  { static const bool value = true; };


template<typename T>
struct is_subview_col
  { static const bool value = false; };

template<typename eT>
struct is_subview_col< subview_col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_subview_col< const subview_col<eT> >
  { static const bool value = true; };


template<typename T>
struct is_subview_elem1
  { static const bool value = false; };

template<typename eT, typename T1>
struct is_subview_elem1< subview_elem1<eT, T1> >
  { static const bool value = true; };

template<typename eT, typename T1>
struct is_subview_elem1< const subview_elem1<eT, T1> >
  { static const bool value = true; };


template<typename T>
struct is_subview_elem2
  { static const bool value = false; };

template<typename eT, typename T1, typename T2>
struct is_subview_elem2< subview_elem2<eT, T1, T2> >
  { static const bool value = true; };

template<typename eT, typename T1, typename T2>
struct is_subview_elem2< const subview_elem2<eT, T1, T2> >
  { static const bool value = true; };



//
//
//



template<typename T>
struct is_Cube
  { static const bool value = false; };

template<typename eT>
struct is_Cube< Cube<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_Cube< const Cube<eT> >
  { static const bool value = true; };

template<typename T>
struct is_subview_cube
  { static const bool value = false; };

template<typename eT>
struct is_subview_cube< subview_cube<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_subview_cube< const subview_cube<eT> >
  { static const bool value = true; };


//
//
//


template<typename T>
struct is_Gen
  { static const bool value = false; };

template<typename T1, typename gen_type>
struct is_Gen< Gen<T1,gen_type> >
  { static const bool value = true; };

template<typename T1, typename gen_type>
struct is_Gen< const Gen<T1,gen_type> >
  { static const bool value = true; };


template<typename T>
struct is_Op
  { static const bool value = false; };

template<typename T1, typename op_type>
struct is_Op< Op<T1,op_type> >
  { static const bool value = true; };

template<typename T1, typename op_type>
struct is_Op< const Op<T1,op_type> >
  { static const bool value = true; };

template<typename T>
struct is_eOp
  { static const bool value = false; };

template<typename T1, typename eop_type>
struct is_eOp< eOp<T1,eop_type> >
  { static const bool value = true; };

template<typename T1, typename eop_type>
struct is_eOp< const eOp<T1,eop_type> >
  { static const bool value = true; };


template<typename T>
struct is_mtOp
  { static const bool value = false; };

template<typename eT, typename T1, typename op_type>
struct is_mtOp< mtOp<eT, T1, op_type> >
  { static const bool value = true; };

template<typename eT, typename T1, typename op_type>
struct is_mtOp< const mtOp<eT, T1, op_type> >
  { static const bool value = true; };


template<typename T>
struct is_Glue
  { static const bool value = false; };

template<typename T1, typename T2, typename glue_type>
struct is_Glue< Glue<T1,T2,glue_type> >
  { static const bool value = true; };

template<typename T1, typename T2, typename glue_type>
struct is_Glue< const Glue<T1,T2,glue_type> >
  { static const bool value = true; };


template<typename T>
struct is_eGlue
  { static const bool value = false; };

template<typename T1, typename T2, typename eglue_type>
struct is_eGlue< eGlue<T1,T2,eglue_type> >
  { static const bool value = true; };

template<typename T1, typename T2, typename eglue_type>
struct is_eGlue< const eGlue<T1,T2,eglue_type> >
  { static const bool value = true; };


template<typename T>
struct is_mtGlue
  { static const bool value = false; };

template<typename eT, typename T1, typename T2, typename glue_type>
struct is_mtGlue< mtGlue<eT, T1, T2, glue_type> >
  { static const bool value = true; };

template<typename eT, typename T1, typename T2, typename glue_type>
struct is_mtGlue< const mtGlue<eT, T1, T2, glue_type> >
  { static const bool value = true; };


//
//


template<typename T>
struct is_glue_times
  { static const bool value = false; };

template<typename T1, typename T2>
struct is_glue_times< Glue<T1,T2,glue_times> >
  { static const bool value = true; };

template<typename T1, typename T2>
struct is_glue_times< const Glue<T1,T2,glue_times> >
  { static const bool value = true; };


template<typename T>
struct is_glue_times_diag
  { static const bool value = false; };

template<typename T1, typename T2>
struct is_glue_times_diag< Glue<T1,T2,glue_times_diag> >
  { static const bool value = true; };

template<typename T1, typename T2>
struct is_glue_times_diag< const Glue<T1,T2,glue_times_diag> >
  { static const bool value = true; };


template<typename T>
struct is_op_diagmat
  { static const bool value = false; };

template<typename T1>
struct is_op_diagmat< Op<T1,op_diagmat> >
  { static const bool value = true; };

template<typename T1>
struct is_op_diagmat< const Op<T1,op_diagmat> >
  { static const bool value = true; };


template<typename T>
struct is_op_strans
  { static const bool value = false; };

template<typename T1>
struct is_op_strans< Op<T1,op_strans> >
  { static const bool value = true; };

template<typename T1>
struct is_op_strans< const Op<T1,op_strans> >
  { static const bool value = true; };


template<typename T>
struct is_op_htrans
  { static const bool value = false; };

template<typename T1>
struct is_op_htrans< Op<T1,op_htrans> >
  { static const bool value = true; };

template<typename T1>
struct is_op_htrans< const Op<T1,op_htrans> >
  { static const bool value = true; };


template<typename T>
struct is_op_htrans2
  { static const bool value = false; };

template<typename T1>
struct is_op_htrans2< Op<T1,op_htrans2> >
  { static const bool value = true; };

template<typename T1>
struct is_op_htrans2< const Op<T1,op_htrans2> >
  { static const bool value = true; };


//
//


template<typename T>
struct is_Mat_trans
  { static const bool value = false; };

template<typename T1>
struct is_Mat_trans< Op<T1,op_htrans> >
  { static const bool value = is_Mat<T1>::value; };

template<typename T1>
struct is_Mat_trans< Op<T1,op_htrans2> >
  { static const bool value = is_Mat<T1>::value; };


//
//


template<typename T>
struct is_GenCube
  { static const bool value = false; };

template<typename eT, typename gen_type>
struct is_GenCube< GenCube<eT,gen_type> >
  { static const bool value = true; };


template<typename T>
struct is_OpCube
  { static const bool value = false; };

template<typename T1, typename op_type>
struct is_OpCube< OpCube<T1,op_type> >
  { static const bool value = true; };


template<typename T>
struct is_eOpCube
  { static const bool value = false; };

template<typename T1, typename eop_type>
struct is_eOpCube< eOpCube<T1,eop_type> >
  { static const bool value = true; };


template<typename T>
struct is_mtOpCube
  { static const bool value = false; };

template<typename eT, typename T1, typename op_type>
struct is_mtOpCube< mtOpCube<eT, T1, op_type> >
  { static const bool value = true; };


template<typename T>
struct is_GlueCube
  { static const bool value = false; };

template<typename T1, typename T2, typename glue_type>
struct is_GlueCube< GlueCube<T1,T2,glue_type> >
  { static const bool value = true; };


template<typename T>
struct is_eGlueCube
  { static const bool value = false; };

template<typename T1, typename T2, typename eglue_type>
struct is_eGlueCube< eGlueCube<T1,T2,eglue_type> >
  { static const bool value = true; };


template<typename T>
struct is_mtGlueCube
  { static const bool value = false; };

template<typename eT, typename T1, typename T2, typename glue_type>
struct is_mtGlueCube< mtGlueCube<eT, T1, T2, glue_type> >
  { static const bool value = true; };


//
//
//


template<typename T>
struct is_op_rel
  { static const bool value = false; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_lt_pre> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_lt_post> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_gt_pre> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_gt_post> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_lteq_pre> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_lteq_post> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_gteq_pre> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_gteq_post> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_eq> >
  { static const bool value = true; };

template<typename out_eT, typename T1>
struct is_op_rel< mtOp<out_eT, T1, op_rel_noteq> >
  { static const bool value = true; };



//
//
//



template<typename T>
struct is_basevec
  { static const bool value = ( is_Row_fixed_only<T>::value || is_Col_fixed_only<T>::value ); };

template<typename eT>
struct is_basevec< Row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< const Row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< Col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< const Col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< subview_row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< const subview_row<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< subview_col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< const subview_col<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< diagview<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_basevec< const diagview<eT> >
  { static const bool value = true; };

template<typename eT, typename T1>
struct is_basevec< subview_elem1<eT,T1> >
  { static const bool value = true; };

template<typename eT, typename T1>
struct is_basevec< const subview_elem1<eT,T1> >
  { static const bool value = true; };


//
//
//



template<typename T1>
struct is_arma_type2
  {
  static const bool value
  =  is_Mat<T1>::value
  || is_Gen<T1>::value
  || is_Op<T1>::value
  || is_Glue<T1>::value
  || is_eOp<T1>::value
  || is_eGlue<T1>::value
  || is_mtOp<T1>::value
  || is_mtGlue<T1>::value
  || is_diagview<T1>::value
  || is_subview<T1>::value
  || is_subview_row<T1>::value
  || is_subview_col<T1>::value
  || is_subview_elem1<T1>::value
  || is_subview_elem2<T1>::value
  ;
  };



// due to rather baroque C++ rules for proving constant expressions,
// certain compilers may get confused with the combination of conditional inheritance, nested classes and the shenanigans in is_Mat_fixed_only.
// below we explicitly ensure the type is forced to be const, which seems to eliminate the confusion.
template<typename T1>
struct is_arma_type
  {
  static const bool value = is_arma_type2<const T1>::value;
  };



template<typename T1>
struct is_arma_cube_type
  {
  static const bool value
  =  is_Cube<T1>::value
  || is_GenCube<T1>::value
  || is_OpCube<T1>::value
  || is_eOpCube<T1>::value
  || is_mtOpCube<T1>::value
  || is_GlueCube<T1>::value
  || is_eGlueCube<T1>::value
  || is_mtGlueCube<T1>::value
  || is_subview_cube<T1>::value
  ;
  };



//
//
//



template<typename T>
struct is_SpMat
  { static const bool value = false; };

template<typename eT>
struct is_SpMat< SpMat<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_SpMat< SpCol<eT> >
  { static const bool value = true; };

template<typename eT>
struct is_SpMat< SpRow<eT> >
  { static const bool value = true; };



template<typename T>
struct is_SpRow
  { static const bool value = false; };

template<typename eT>
struct is_SpRow< SpRow<eT> >
  { static const bool value = true; };



template<typename T>
struct is_SpCol
  { static const bool value = false; };

template<typename eT>
struct is_SpCol< SpCol<eT> >
  { static const bool value = true; };



template<typename T>
struct is_SpSubview
  { static const bool value = false; };

template<typename eT>
struct is_SpSubview< SpSubview<eT> >
  { static const bool value = true; };


template<typename T>
struct is_spdiagview
  { static const bool value = false; };

template<typename eT>
struct is_spdiagview< spdiagview<eT> >
  { static const bool value = true; };


template<typename T>
struct is_SpOp
  { static const bool value = false; };

template<typename T1, typename op_type>
struct is_SpOp< SpOp<T1,op_type> >
  { static const bool value = true; };


template<typename T>
struct is_SpGlue
  { static const bool value = false; };

template<typename T1, typename T2, typename glue_type>
struct is_SpGlue< SpGlue<T1,T2,glue_type> >
  { static const bool value = true; };


template<typename T>
struct is_mtSpOp
  { static const bool value = false; };

template<typename eT, typename T1, typename spop_type>
struct is_mtSpOp< mtSpOp<eT, T1, spop_type> >
  { static const bool value = true; };



template<typename T1>
struct is_arma_sparse_type
  {
  static const bool value
  =  is_SpMat<T1>::value
  || is_SpSubview<T1>::value
  || is_spdiagview<T1>::value
  || is_SpOp<T1>::value
  || is_SpGlue<T1>::value
  || is_mtSpOp<T1>::value
  ;
  };



//
//
//


template<typename T1, typename T2>
struct is_same_type
  {
  static const bool value = false;
  static const bool yes   = false;
  static const bool no    = true;
  };


template<typename T1>
struct is_same_type<T1,T1>
  {
  static const bool value = true;
  static const bool yes   = true;
  static const bool no    = false;
  };



//
//
//


template<typename T1>
struct is_u8
  { static const bool value = false; };

template<>
struct is_u8<u8>
  { static const bool value = true; };



template<typename T1>
struct is_s8
  { static const bool value = false; };

template<>
struct is_s8<s8>
  { static const bool value = true; };



template<typename T1>
struct is_u16
  { static const bool value = false; };

template<>
struct is_u16<u16>
  { static const bool value = true; };



template<typename T1>
struct is_s16
  { static const bool value = false; };

template<>
struct is_s16<s16>
  { static const bool value = true; };



template<typename T1>
struct is_u32
  { static const bool value = false; };

template<>
struct is_u32<u32>
  { static const bool value = true; };



template<typename T1>
struct is_s32
  { static const bool value = false; };

template<>
struct is_s32<s32>
  { static const bool value = true; };



#if defined(ARMA_USE_U64S64)
  template<typename T1>
  struct is_u64
    { static const bool value = false; };

  template<>
  struct is_u64<u64>
    { static const bool value = true; };


  template<typename T1>
  struct is_s64
    { static const bool value = false; };

  template<>
  struct is_s64<s64>
    { static const bool value = true; };
#endif



template<typename T1>
struct is_ulng_t
  { static const bool value = false; };

template<>
struct is_ulng_t<ulng_t>
  { static const bool value = true; };



template<typename T1>
struct is_slng_t
  { static const bool value = false; };

template<>
struct is_slng_t<slng_t>
  { static const bool value = true; };



template<typename T1>
struct is_ulng_t_32
  { static const bool value = false; };

template<>
struct is_ulng_t_32<ulng_t>
  { static const bool value = (sizeof(ulng_t) == 4); };



template<typename T1>
struct is_slng_t_32
  { static const bool value = false; };

template<>
struct is_slng_t_32<slng_t>
  { static const bool value = (sizeof(slng_t) == 4); };



template<typename T1>
struct is_ulng_t_64
  { static const bool value = false; };

template<>
struct is_ulng_t_64<ulng_t>
  { static const bool value = (sizeof(ulng_t) == 8); };



template<typename T1>
struct is_slng_t_64
  { static const bool value = false; };

template<>
struct is_slng_t_64<slng_t>
  { static const bool value = (sizeof(slng_t) == 8); };



template<typename T1>
struct is_uword
  { static const bool value = false; };

template<>
struct is_uword<uword>
  { static const bool value = true; };



template<typename T1>
struct is_sword
  { static const bool value = false; };

template<>
struct is_sword<sword>
  { static const bool value = true; };



template<typename T1>
struct is_float
  { static const bool value = false; };

template<>
struct is_float<float>
  { static const bool value = true; };



template<typename T1>
struct is_double
  { static const bool value = false; };

template<>
struct is_double<double>
  { static const bool value = true; };



template<typename T1>
struct is_real
  { static const bool value = false; };

template<>
struct is_real<float>
  { static const bool value = true; };

template<>
struct is_real<double>
  { static const bool value = true; };




template<typename T1>
struct is_not_complex
  { static const bool value = true; };

template<typename eT>
struct is_not_complex< std::complex<eT> >
  { static const bool value = false; };



template<typename T1>
struct is_complex
  { static const bool value = false; };

// template<>
template<typename eT>
struct is_complex< std::complex<eT> >
  { static const bool value = true; };



template<typename T1>
struct is_complex_float
  { static const bool value = false; };

template<>
struct is_complex_float< std::complex<float> >
  { static const bool value = true; };



template<typename T1>
struct is_complex_double
  { static const bool value = false; };

template<>
struct is_complex_double< std::complex<double> >
  { static const bool value = true; };



template<typename T1>
struct is_complex_strict
  { static const bool value = false; };

template<>
struct is_complex_strict< std::complex<float> >
  { static const bool value = true; };

template<>
struct is_complex_strict< std::complex<double> >
  { static const bool value = true; };



template<typename T1>
struct is_cx
  {
  static const bool value = false;
  static const bool yes   = false;
  static const bool no    = true;
  };

// template<>
template<typename T>
struct is_cx< std::complex<T> >
  {
  static const bool value = true;
  static const bool yes   = true;
  static const bool no    = false;
  };



//! check for a weird implementation of the std::complex class
template<typename T1>
struct is_supported_complex
  { static const bool value = false; };

//template<>
template<typename eT>
struct is_supported_complex< std::complex<eT> >
  { static const bool value = ( sizeof(std::complex<eT>) == 2*sizeof(eT) ); };



template<typename T1>
struct is_supported_complex_float
  { static const bool value = false; };

template<>
struct is_supported_complex_float< std::complex<float> >
  { static const bool value = ( sizeof(std::complex<float>) == 2*sizeof(float) ); };



template<typename T1>
struct is_supported_complex_double
  { static const bool value = false; };

template<>
struct is_supported_complex_double< std::complex<double> >
  { static const bool value = ( sizeof(std::complex<double>) == 2*sizeof(double) ); };



template<typename T1>
struct is_supported_elem_type
  {
  static const bool value = \
    is_u8<T1>::value ||
    is_s8<T1>::value ||
    is_u16<T1>::value ||
    is_s16<T1>::value ||
    is_u32<T1>::value ||
    is_s32<T1>::value ||
#if defined(ARMA_USE_U64S64)
    is_u64<T1>::value ||
    is_s64<T1>::value ||
#endif
#if defined(ARMA_ALLOW_LONG)
    is_ulng_t<T1>::value ||
    is_slng_t<T1>::value ||
#endif
    is_float<T1>::value ||
    is_double<T1>::value ||
    is_supported_complex_float<T1>::value ||
    is_supported_complex_double<T1>::value;
  };



template<typename T1>
struct is_supported_blas_type
  {
  static const bool value = \
    is_float<T1>::value ||
    is_double<T1>::value ||
    is_supported_complex_float<T1>::value ||
    is_supported_complex_double<T1>::value;
  };



template<typename T>
struct is_signed
  {
  static const bool value = true;
  };


template<> struct is_signed<u8>     { static const bool value = false; };
template<> struct is_signed<u16>    { static const bool value = false; };
template<> struct is_signed<u32>    { static const bool value = false; };
#if defined(ARMA_USE_U64S64)
template<> struct is_signed<u64>    { static const bool value = false; };
#endif
#if defined(ARMA_ALLOW_LONG)
template<> struct is_signed<ulng_t> { static const bool value = false; };
#endif


template<typename T>
struct is_non_integral
  {
  static const bool value = false;
  };


template<> struct is_non_integral<              float   > { static const bool value = true; };
template<> struct is_non_integral<              double  > { static const bool value = true; };
template<> struct is_non_integral< std::complex<float>  > { static const bool value = true; };
template<> struct is_non_integral< std::complex<double> > { static const bool value = true; };




//

class arma_junk_class;

template<typename T1, typename T2>
struct force_different_type
  {
  typedef T1 T1_result;
  typedef T2 T2_result;
  };


template<typename T1>
struct force_different_type<T1,T1>
  {
  typedef T1              T1_result;
  typedef arma_junk_class T2_result;
  };



//


template<typename T1>
struct resolves_to_vector_default { static const bool value = false;                    };

template<typename T1>
struct resolves_to_vector_test    { static const bool value = T1::is_col || T1::is_row; };


template<typename T1, bool condition>
struct resolves_to_vector_redirect {};

template<typename T1>
struct resolves_to_vector_redirect<T1, false> { typedef resolves_to_vector_default<T1> result; };

template<typename T1>
struct resolves_to_vector_redirect<T1, true>  { typedef resolves_to_vector_test<T1>    result; };


template<typename T1>
struct resolves_to_vector : public resolves_to_vector_redirect<T1, is_arma_type<T1>::value>::result {};

template<typename T1>
struct resolves_to_sparse_vector : public resolves_to_vector_redirect<T1, is_arma_sparse_type<T1>::value>::result {};

//

template<typename T1>
struct resolves_to_rowvector_default { static const bool value = false;      };

template<typename T1>
struct resolves_to_rowvector_test    { static const bool value = T1::is_row; };


template<typename T1, bool condition>
struct resolves_to_rowvector_redirect {};

template<typename T1>
struct resolves_to_rowvector_redirect<T1, false> { typedef resolves_to_rowvector_default<T1> result; };

template<typename T1>
struct resolves_to_rowvector_redirect<T1, true>  { typedef resolves_to_rowvector_test<T1>    result; };


template<typename T1>
struct resolves_to_rowvector : public resolves_to_rowvector_redirect<T1, is_arma_type<T1>::value>::result {};

//

template<typename T1>
struct resolves_to_colvector_default { static const bool value = false;      };

template<typename T1>
struct resolves_to_colvector_test    { static const bool value = T1::is_col; };


template<typename T1, bool condition>
struct resolves_to_colvector_redirect {};

template<typename T1>
struct resolves_to_colvector_redirect<T1, false> { typedef resolves_to_colvector_default<T1> result; };

template<typename T1>
struct resolves_to_colvector_redirect<T1, true>  { typedef resolves_to_colvector_test<T1>    result; };


template<typename T1>
struct resolves_to_colvector : public resolves_to_colvector_redirect<T1, is_arma_type<T1>::value>::result {};



template<typename glue_type> struct is_glue_mixed_times                   { static const bool value = false; };
template<>                   struct is_glue_mixed_times<glue_mixed_times> { static const bool value = true;  };



template<typename glue_type> struct is_glue_mixed_elem { static const bool value = false; };

template<>                   struct is_glue_mixed_elem<glue_mixed_plus>  { static const bool value = true;  };
template<>                   struct is_glue_mixed_elem<glue_mixed_minus> { static const bool value = true;  };
template<>                   struct is_glue_mixed_elem<glue_mixed_div>   { static const bool value = true;  };
template<>                   struct is_glue_mixed_elem<glue_mixed_schur> { static const bool value = true;  };

template<>                   struct is_glue_mixed_elem<glue_rel_lt>    { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_gt>    { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_lteq>  { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_gteq>  { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_eq>    { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_noteq> { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_and>   { static const bool value = true; };
template<>                   struct is_glue_mixed_elem<glue_rel_or>    { static const bool value = true; };



template<typename op_type> struct is_op_mixed_elem { static const bool value = false; };

template<>                 struct is_op_mixed_elem<op_cx_scalar_times>      { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_cx_scalar_plus>       { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_cx_scalar_minus_pre>  { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_cx_scalar_minus_post> { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_cx_scalar_div_pre>    { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_cx_scalar_div_post>   { static const bool value = true; };

template<>                 struct is_op_mixed_elem<op_rel_lt_pre>    { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_lt_post>   { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_gt_pre>    { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_gt_post>   { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_lteq_pre>  { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_lteq_post> { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_gteq_pre>  { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_gteq_post> { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_eq>        { static const bool value = true; };
template<>                 struct is_op_mixed_elem<op_rel_noteq>     { static const bool value = true; };



template<typename spop_type> struct is_spop_elem                    { static const bool value = false; };
template<>                   struct is_spop_elem<spop_scalar_times> { static const bool value = true;  };


template<typename spglue_type> struct is_spglue_elem                { static const bool value = false; };
template<>                     struct is_spglue_elem<spglue_plus>   { static const bool value = true;  };
template<>                     struct is_spglue_elem<spglue_plus2>  { static const bool value = true;  };
template<>                     struct is_spglue_elem<spglue_minus>  { static const bool value = true;  };
template<>                     struct is_spglue_elem<spglue_minus2> { static const bool value = true;  };


template<typename spglue_type> struct is_spglue_times               { static const bool value = false; };
template<>                     struct is_spglue_times<spglue_times> { static const bool value = true;  };


template<typename spglue_type> struct is_spglue_times2               { static const bool value = false; };
template<>                     struct is_spglue_times<spglue_times2> { static const bool value = true;  };



template<typename T1>
struct is_outer_product
  { static const bool value = false; };

template<typename T1, typename T2>
struct is_outer_product< Glue<T1,T2,glue_times> >
  { static const bool value = (resolves_to_colvector<T1>::value && resolves_to_rowvector<T2>::value); };



template<typename T1>
struct has_op_inv
  { static const bool value = false; };

template<typename T1>
struct has_op_inv< Op<T1,op_inv> >
  { static const bool value = true;  };

template<typename T1, typename T2>
struct has_op_inv< Glue<Op<T1,op_inv>, T2, glue_times> >
  { static const bool value = true;  };

template<typename T1, typename T2>
struct has_op_inv< Glue<T1, Op<T2,op_inv>, glue_times> >
  { static const bool value = true;  };



//! @}
