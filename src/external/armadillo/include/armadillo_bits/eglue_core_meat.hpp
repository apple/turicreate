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


//! \addtogroup eglue_core
//! @{



#undef arma_applier_1u
#undef arma_applier_1a
#undef arma_applier_2
#undef arma_applier_3
#undef operatorA
#undef operatorB

#undef arma_applier_1_mp
#undef arma_applier_2_mp
#undef arma_applier_3_mp


#if defined(ARMA_SIMPLE_LOOPS)
  #define arma_applier_1u(operatorA, operatorB) \
    {\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA P1[i] operatorB P2[i];\
      }\
    }
#else
  #define arma_applier_1u(operatorA, operatorB) \
    {\
    uword i,j;\
    \
    for(i=0, j=1; j<n_elem; i+=2, j+=2)\
      {\
      eT tmp_i = P1[i];\
      eT tmp_j = P1[j];\
      \
      tmp_i operatorB##= P2[i];\
      tmp_j operatorB##= P2[j];\
      \
      out_mem[i] operatorA tmp_i;\
      out_mem[j] operatorA tmp_j;\
      }\
    \
    if(i < n_elem)\
      {\
      out_mem[i] operatorA P1[i] operatorB P2[i];\
      }\
    }
#endif


#if defined(ARMA_SIMPLE_LOOPS)
  #define arma_applier_1a(operatorA, operatorB) \
    {\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA P1.at_alt(i) operatorB P2.at_alt(i);\
      }\
    }
#else
  #define arma_applier_1a(operatorA, operatorB) \
    {\
    uword i,j;\
    \
    for(i=0, j=1; j<n_elem; i+=2, j+=2)\
      {\
      eT tmp_i = P1.at_alt(i);\
      eT tmp_j = P1.at_alt(j);\
      \
      tmp_i operatorB##= P2.at_alt(i);\
      tmp_j operatorB##= P2.at_alt(j);\
      \
      out_mem[i] operatorA tmp_i;\
      out_mem[j] operatorA tmp_j;\
      }\
    \
    if(i < n_elem)\
      {\
      out_mem[i] operatorA P1.at_alt(i) operatorB P2.at_alt(i);\
      }\
    }
#endif


#define arma_applier_2(operatorA, operatorB) \
  {\
  if(n_rows != 1)\
    {\
    for(uword col=0; col<n_cols; ++col)\
      {\
      uword i,j;\
      \
      for(i=0, j=1; j<n_rows; i+=2, j+=2)\
        {\
        eT tmp_i = P1.at(i,col);\
        eT tmp_j = P1.at(j,col);\
        \
        tmp_i operatorB##= P2.at(i,col);\
        tmp_j operatorB##= P2.at(j,col);\
        \
        *out_mem operatorA tmp_i; out_mem++; \
        *out_mem operatorA tmp_j; out_mem++; \
        }\
      \
      if(i < n_rows)\
        {\
        *out_mem operatorA P1.at(i,col) operatorB P2.at(i,col); out_mem++; \
        }\
      }\
    }\
  else\
    {\
    uword i,j;\
    for(i=0, j=1; j < n_cols; i+=2, j+=2)\
      {\
      eT tmp_i = P1.at(0,i);\
      eT tmp_j = P1.at(0,j);\
      \
      tmp_i operatorB##= P2.at(0,i);\
      tmp_j operatorB##= P2.at(0,j);\
      \
      out_mem[i] operatorA tmp_i;\
      out_mem[j] operatorA tmp_j;\
      }\
    \
    if(i < n_cols)\
      {\
      out_mem[i] operatorA P1.at(0,i) operatorB P2.at(0,i);\
      }\
    }\
  }



#define arma_applier_3(operatorA, operatorB) \
  {\
  for(uword slice=0; slice<n_slices; ++slice)\
    {\
    for(uword col=0; col<n_cols; ++col)\
      {\
      uword i,j;\
      \
      for(i=0, j=1; j<n_rows; i+=2, j+=2)\
        {\
        eT tmp_i = P1.at(i,col,slice);\
        eT tmp_j = P1.at(j,col,slice);\
        \
        tmp_i operatorB##= P2.at(i,col,slice);\
        tmp_j operatorB##= P2.at(j,col,slice);\
        \
        *out_mem operatorA tmp_i; out_mem++; \
        *out_mem operatorA tmp_j; out_mem++; \
        }\
      \
      if(i < n_rows)\
        {\
        *out_mem operatorA P1.at(i,col,slice) operatorB P2.at(i,col,slice); out_mem++; \
        }\
      }\
    }\
  }



#if (defined(ARMA_USE_OPENMP) && defined(ARMA_USE_CXX11))

  #define arma_applier_1_mp(operatorA, operatorB) \
    {\
    const int n_threads = mp_thread_limit::get();\
    _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA P1[i] operatorB P2[i];\
      }\
    }

  #define arma_applier_2_mp(operatorA, operatorB) \
    {\
    const int n_threads = mp_thread_limit::get();\
    if(n_cols == 1)\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword count=0; count < n_rows; ++count)\
        {\
        out_mem[count] operatorA P1.at(count,0) operatorB P2.at(count,0);\
        }\
      }\
    else\
    if(n_rows == 1)\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword count=0; count < n_cols; ++count)\
        {\
        out_mem[count] operatorA P1.at(0,count) operatorB P2.at(0,count);\
        }\
      }\
    else\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword col=0; col<n_cols; ++col)\
        {\
        for(uword row=0; row<n_rows; ++row)\
          {\
          out.at(row,col) operatorA P1.at(row,col) operatorB P2.at(row,col);\
          }\
        }\
      }\
    }

  #define arma_applier_3_mp(operatorA, operatorB) \
    {\
    const int n_threads = mp_thread_limit::get();\
    _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
    for(uword slice=0; slice<n_slices; ++slice)\
      {\
      for(uword col=0; col<n_cols; ++col)\
      for(uword row=0; row<n_rows; ++row)\
        {\
        out.at(row,col,slice) operatorA P1.at(row,col,slice) operatorB P2.at(row,col,slice);\
        }\
      }\
    }

#else

  #define arma_applier_1_mp(operatorA, operatorB)  arma_applier_1u(operatorA, operatorB)
  #define arma_applier_2_mp(operatorA, operatorB)  arma_applier_2(operatorA, operatorB)
  #define arma_applier_3_mp(operatorA, operatorB)  arma_applier_3(operatorA, operatorB)

#endif



//
// matrices



template<typename eglue_type>
template<typename outT, typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply(outT& out, const eGlue<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);
  const bool use_mp = (Proxy<T1>::use_mp || Proxy<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  // NOTE: we're assuming that the matrix has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Mat contructor or operator=()


  eT* out_mem = out.memptr();

  if(use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
      typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename Proxy<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(=, *); }
          }
        else
          {
          typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
          typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(=, *); }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
        typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(=, *); }
        }
      }
    }
  else
    {
    const uword n_rows = x.get_n_rows();
    const uword n_cols = x.get_n_cols();

    const Proxy<T1>& P1 = x.P1;
    const Proxy<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2_mp(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2_mp(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2_mp(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2_mp(=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2(=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_plus(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "addition");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);
  const bool use_mp = (Proxy<T1>::use_mp || Proxy<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
      typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(+=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename Proxy<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(+=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(+=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(+=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(+=, *); }
          }
        else
          {
          typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
          typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(+=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(+=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(+=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(+=, *); }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
        typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(+=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(+=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(+=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(+=, *); }
        }
      }
    }
  else
    {
    const Proxy<T1>& P1 = x.P1;
    const Proxy<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2_mp(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2_mp(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2_mp(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2_mp(+=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2(+=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_minus(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "subtraction");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);
  const bool use_mp = (Proxy<T1>::use_mp || Proxy<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
      typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(-=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename Proxy<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(-=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(-=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(-=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(-=, *); }
          }
        else
          {
          typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
          typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(-=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(-=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(-=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(-=, *); }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
        typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(-=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(-=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(-=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(-=, *); }
        }
      }
    }
  else
    {
    const Proxy<T1>& P1 = x.P1;
    const Proxy<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2_mp(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2_mp(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2_mp(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2_mp(-=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2(-=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_schur(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "element-wise multiplication");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);
  const bool use_mp = (Proxy<T1>::use_mp || Proxy<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
      typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(*=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename Proxy<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(*=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(*=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(*=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(*=, *); }
          }
        else
          {
          typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
          typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(*=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(*=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(*=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(*=, *); }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
        typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(*=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(*=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(*=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(*=, *); }
        }
      }
    }
  else
    {
    const Proxy<T1>& P1 = x.P1;
    const Proxy<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2_mp(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2_mp(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2_mp(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2_mp(*=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2(*=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_div(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "element-wise division");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);
  const bool use_mp = (Proxy<T1>::use_mp || Proxy<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
      typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(/=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename Proxy<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(/=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(/=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(/=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(/=, *); }
          }
        else
          {
          typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
          typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(/=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(/=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(/=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(/=, *); }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P1 = x.P1.get_ea();
        typename Proxy<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(/=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(/=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(/=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(/=, *); }
        }
      }
    }
  else
    {
    const Proxy<T1>& P1 = x.P1;
    const Proxy<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (Proxy<T1>::use_mp && Proxy<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2_mp(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2_mp(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2_mp(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2_mp(/=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_2(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_2(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_2(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_2(/=, *); }
      }
    }
  }



//
// cubes



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);
  const bool use_mp = (ProxyCube<T1>::use_mp || ProxyCube<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  // NOTE: we're assuming that the cube has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Cube contructor or operator=()


  eT* out_mem = out.memptr();

  if(use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
      typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename ProxyCube<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(=, *); }
          }
        else
          {
          typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
          typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(=, *); }
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
        typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(=, *); }
        }
      }
    }
  else
    {
    const uword n_rows   = x.get_n_rows();
    const uword n_cols   = x.get_n_cols();
    const uword n_slices = x.get_n_slices();

    const ProxyCube<T1>& P1 = x.P1;
    const ProxyCube<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3_mp(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3_mp(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3_mp(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3_mp(=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3(=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3(=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3(=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3(=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_plus(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "addition");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);
  const bool use_mp = (ProxyCube<T1>::use_mp || ProxyCube<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
      typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(+=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename ProxyCube<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(+=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(+=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(+=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(+=, *); }
          }
        else
          {
          typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
          typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(+=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(+=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(+=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(+=, *); }
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
        typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(+=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(+=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(+=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(+=, *); }
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P1 = x.P1;
    const ProxyCube<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3_mp(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3_mp(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3_mp(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3_mp(+=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3(+=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3(+=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3(+=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3(+=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_minus(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "subtraction");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);
  const bool use_mp = (ProxyCube<T1>::use_mp || ProxyCube<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
      typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(-=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename ProxyCube<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(-=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(-=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(-=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(-=, *); }
          }
        else
          {
          typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
          typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(-=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(-=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(-=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(-=, *); }
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
        typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(-=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(-=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(-=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(-=, *); }
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P1 = x.P1;
    const ProxyCube<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3_mp(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3_mp(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3_mp(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3_mp(-=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3(-=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3(-=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3(-=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3(-=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_schur(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise multiplication");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);
  const bool use_mp = (ProxyCube<T1>::use_mp || ProxyCube<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
      typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(*=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename ProxyCube<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(*=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(*=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(*=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(*=, *); }
          }
        else
          {
          typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
          typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(*=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(*=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(*=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(*=, *); }
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
        typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(*=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(*=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(*=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(*=, *); }
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P1 = x.P1;
    const ProxyCube<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3_mp(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3_mp(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3_mp(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3_mp(*=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3(*=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3(*=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3(*=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3(*=, *); }
      }
    }
  }



template<typename eglue_type>
template<typename T1, typename T2>
arma_hot
inline
void
eglue_core<eglue_type>::apply_inplace_div(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x)
  {
  arma_extra_debug_sigprint();

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise division");

  typedef typename T1::elem_type eT;

  eT* out_mem = out.memptr();

  const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);
  const bool use_mp = (ProxyCube<T1>::use_mp || ProxyCube<T2>::use_mp) && (arma_config::cxx11 && arma_config::openmp);

  if(use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
      typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1_mp(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1_mp(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1_mp(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1_mp(/=, *); }
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P1.is_aligned() && x.P2.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P1 = x.P1.get_aligned_ea();
          typename ProxyCube<T2>::aligned_ea_type P2 = x.P2.get_aligned_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1a(/=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1a(/=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1a(/=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1a(/=, *); }
          }
        else
          {
          typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
          typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

               if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(/=, +); }
          else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(/=, -); }
          else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(/=, /); }
          else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(/=, *); }
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P1 = x.P1.get_ea();
        typename ProxyCube<T2>::ea_type P2 = x.P2.get_ea();

             if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_1u(/=, +); }
        else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_1u(/=, -); }
        else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_1u(/=, /); }
        else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_1u(/=, *); }
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P1 = x.P1;
    const ProxyCube<T2>& P2 = x.P2;

    if(use_mp && mp_gate<eT, (ProxyCube<T1>::use_mp && ProxyCube<T2>::use_mp)>::eval(x.get_n_elem()))
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3_mp(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3_mp(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3_mp(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3_mp(/=, *); }
      }
    else
      {
           if(is_same_type<eglue_type, eglue_plus >::yes) { arma_applier_3(/=, +); }
      else if(is_same_type<eglue_type, eglue_minus>::yes) { arma_applier_3(/=, -); }
      else if(is_same_type<eglue_type, eglue_div  >::yes) { arma_applier_3(/=, /); }
      else if(is_same_type<eglue_type, eglue_schur>::yes) { arma_applier_3(/=, *); }
      }
    }
  }



#undef arma_applier_1u
#undef arma_applier_1a
#undef arma_applier_2
#undef arma_applier_3

#undef arma_applier_1_mp
#undef arma_applier_2_mp
#undef arma_applier_3_mp


//! @}
