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


//! \addtogroup eop_core
//! @{


#undef arma_applier_1u
#undef arma_applier_1a
#undef arma_applier_2
#undef arma_applier_3
#undef operatorA

#undef arma_applier_1_mp
#undef arma_applier_2_mp
#undef arma_applier_3_mp


#if defined(ARMA_SIMPLE_LOOPS)
  #define arma_applier_1u(operatorA) \
    {\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA eop_core<eop_type>::process(P[i], k);\
      }\
    }
#else
  #define arma_applier_1u(operatorA) \
    {\
    uword i,j;\
    \
    for(i=0, j=1; j<n_elem; i+=2, j+=2)\
      {\
      eT tmp_i = P[i];\
      eT tmp_j = P[j];\
      \
      tmp_i = eop_core<eop_type>::process(tmp_i, k);\
      tmp_j = eop_core<eop_type>::process(tmp_j, k);\
      \
      out_mem[i] operatorA tmp_i;\
      out_mem[j] operatorA tmp_j;\
      }\
    \
    if(i < n_elem)\
      {\
      out_mem[i] operatorA eop_core<eop_type>::process(P[i], k);\
      }\
    }
#endif



#if defined(ARMA_SIMPLE_LOOPS)
  #define arma_applier_1a(operatorA) \
    {\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA eop_core<eop_type>::process(P.at_alt(i), k);\
      }\
    }
#else
  #define arma_applier_1a(operatorA) \
    {\
    uword i,j;\
    \
    for(i=0, j=1; j<n_elem; i+=2, j+=2)\
      {\
      eT tmp_i = P.at_alt(i);\
      eT tmp_j = P.at_alt(j);\
      \
      tmp_i = eop_core<eop_type>::process(tmp_i, k);\
      tmp_j = eop_core<eop_type>::process(tmp_j, k);\
      \
      out_mem[i] operatorA tmp_i;\
      out_mem[j] operatorA tmp_j;\
      }\
    \
    if(i < n_elem)\
      {\
      out_mem[i] operatorA eop_core<eop_type>::process(P.at_alt(i), k);\
      }\
    }
#endif


#define arma_applier_2(operatorA) \
  {\
  if(n_rows != 1)\
    {\
    for(uword col=0; col<n_cols; ++col)\
      {\
      uword i,j;\
      \
      for(i=0, j=1; j<n_rows; i+=2, j+=2)\
        {\
        eT tmp_i = P.at(i,col);\
        eT tmp_j = P.at(j,col);\
        \
        tmp_i = eop_core<eop_type>::process(tmp_i, k);\
        tmp_j = eop_core<eop_type>::process(tmp_j, k);\
        \
        *out_mem operatorA tmp_i;  out_mem++;\
        *out_mem operatorA tmp_j;  out_mem++;\
        }\
      \
      if(i < n_rows)\
        {\
        *out_mem operatorA eop_core<eop_type>::process(P.at(i,col), k);  out_mem++;\
        }\
      }\
    }\
  else\
    {\
    for(uword count=0; count < n_cols; ++count)\
      {\
      out_mem[count] operatorA eop_core<eop_type>::process(P.at(0,count), k);\
      }\
    }\
  }



#define arma_applier_3(operatorA) \
  {\
  for(uword slice=0; slice<n_slices; ++slice)\
    {\
    for(uword col=0; col<n_cols; ++col)\
      {\
      uword i,j;\
      \
      for(i=0, j=1; j<n_rows; i+=2, j+=2)\
        {\
        eT tmp_i = P.at(i,col,slice);\
        eT tmp_j = P.at(j,col,slice);\
        \
        tmp_i = eop_core<eop_type>::process(tmp_i, k);\
        tmp_j = eop_core<eop_type>::process(tmp_j, k);\
        \
        *out_mem operatorA tmp_i; out_mem++; \
        *out_mem operatorA tmp_j; out_mem++; \
        }\
      \
      if(i < n_rows)\
        {\
        *out_mem operatorA eop_core<eop_type>::process(P.at(i,col,slice), k); out_mem++; \
        }\
      }\
    }\
  }



#if (defined(ARMA_USE_OPENMP) && defined(ARMA_USE_CXX11))

  #define arma_applier_1_mp(operatorA) \
    {\
    const int n_threads = mp_thread_limit::get();\
    _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
    for(uword i=0; i<n_elem; ++i)\
      {\
      out_mem[i] operatorA eop_core<eop_type>::process(P[i], k);\
      }\
    }

  #define arma_applier_2_mp(operatorA) \
    {\
    const int n_threads = mp_thread_limit::get();\
    if(n_cols == 1)\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword count=0; count < n_rows; ++count)\
        {\
        out_mem[count] operatorA eop_core<eop_type>::process(P.at(count,0), k);\
        }\
      }\
    else\
    if(n_rows == 1)\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword count=0; count < n_cols; ++count)\
        {\
        out_mem[count] operatorA eop_core<eop_type>::process(P.at(0,count), k);\
        }\
      }\
    else\
      {\
      _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
      for(uword col=0; col < n_cols; ++col)\
        {\
        for(uword row=0; row < n_rows; ++row)\
          {\
          out.at(row,col) operatorA eop_core<eop_type>::process(P.at(row,col), k);\
          }\
        }\
      }\
    }

  #define arma_applier_3_mp(operatorA) \
    {\
    const int n_threads = mp_thread_limit::get();\
    _Pragma("omp parallel for schedule(static) num_threads(n_threads)")\
    for(uword slice=0; slice<n_slices; ++slice)\
      {\
      for(uword col=0; col<n_cols; ++col)\
      for(uword row=0; row<n_rows; ++row)\
        {\
        out.at(row,col,slice) operatorA eop_core<eop_type>::process(P.at(row,col,slice), k);\
        }\
      }\
    }

#else

  #define arma_applier_1_mp(operatorA)  arma_applier_1u(operatorA)
  #define arma_applier_2_mp(operatorA)  arma_applier_2(operatorA)
  #define arma_applier_3_mp(operatorA)  arma_applier_3(operatorA)

#endif



//
// matrices



template<typename eop_type>
template<typename outT, typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply(outT& out, const eOp<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // NOTE: we're assuming that the matrix has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Mat contructor or operator=()

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOp<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(=);
          }
        else
          {
          typename Proxy<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(=);
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(=);
        }
      }
    }
  else
    {
    const uword n_rows = x.get_n_rows();
    const uword n_cols = x.get_n_cols();

    const Proxy<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_2_mp(=);
      }
    else
      {
      arma_applier_2(=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_plus(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "addition");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOp<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(+=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(+=);
          }
        else
          {
          typename Proxy<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(+=);
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(+=);
        }
      }
    }
  else
    {
    const Proxy<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_2_mp(+=);
      }
    else
      {
      arma_applier_2(+=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_minus(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "subtraction");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOp<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(-=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(-=);
          }
        else
          {
          typename Proxy<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(-=);
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(-=);
        }
      }
    }
  else
    {
    const Proxy<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_2_mp(-=);
      }
    else
      {
      arma_applier_2(-=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_schur(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "element-wise multiplication");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOp<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(*=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(*=);
          }
        else
          {
          typename Proxy<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(*=);
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(*=);
        }
      }
    }
  else
    {
    const Proxy<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_2_mp(*=);
      }
    else
      {
      arma_applier_2(*=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_div(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = x.get_n_rows();
  const uword n_cols = x.get_n_cols();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, n_rows, n_cols, "element-wise division");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOp<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = x.get_n_elem();

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename Proxy<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(/=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename Proxy<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(/=);
          }
        else
          {
          typename Proxy<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(/=);
          }
        }
      else
        {
        typename Proxy<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(/=);
        }
      }
    }
  else
    {
    const Proxy<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_2_mp(/=);
      }
    else
      {
      arma_applier_2(/=);
      }
    }
  }



//
// cubes



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // NOTE: we're assuming that the matrix has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Mat contructor or operator=()

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOpCube<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(ProxyCube<T1>::use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(=);
          }
        else
          {
          typename ProxyCube<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(=);
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(=);
        }
      }
    }
  else
    {
    const uword n_rows   = x.get_n_rows();
    const uword n_cols   = x.get_n_cols();
    const uword n_slices = x.get_n_slices();

    const ProxyCube<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_3_mp(=);
      }
    else
      {
      arma_applier_3(=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_plus(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "addition");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOpCube<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(ProxyCube<T1>::use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(+=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(+=);
          }
        else
          {
          typename ProxyCube<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(+=);
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(+=);
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_3_mp(+=);
      }
    else
      {
      arma_applier_3(+=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_minus(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "subtraction");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOpCube<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(ProxyCube<T1>::use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(-=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(-=);
          }
        else
          {
          typename ProxyCube<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(-=);
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(-=);
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_3_mp(-=);
      }
    else
      {
      arma_applier_3(-=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_schur(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise multiplication");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOpCube<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(ProxyCube<T1>::use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(*=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(*=);
          }
        else
          {
          typename ProxyCube<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(*=);
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(*=);
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_3_mp(*=);
      }
    else
      {
      arma_applier_3(*=);
      }
    }
  }



template<typename eop_type>
template<typename T1>
arma_hot
inline
void
eop_core<eop_type>::apply_inplace_div(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = x.get_n_rows();
  const uword n_cols   = x.get_n_cols();
  const uword n_slices = x.get_n_slices();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise division");

  const eT  k       = x.aux;
        eT* out_mem = out.memptr();

  const bool use_mp = (arma_config::cxx11 && arma_config::openmp) && (eOpCube<T1, eop_type>::use_mp || (is_same_type<eop_type, eop_pow>::value && (is_cx<eT>::yes || x.aux != eT(2))));

  if(ProxyCube<T1>::use_at == false)
    {
    const uword n_elem = out.n_elem;

    if(use_mp && mp_gate<eT>::eval(n_elem))
      {
      typename ProxyCube<T1>::ea_type P = x.P.get_ea();

      arma_applier_1_mp(/=);
      }
    else
      {
      if(memory::is_aligned(out_mem))
        {
        memory::mark_as_aligned(out_mem);

        if(x.P.is_aligned())
          {
          typename ProxyCube<T1>::aligned_ea_type P = x.P.get_aligned_ea();

          arma_applier_1a(/=);
          }
        else
          {
          typename ProxyCube<T1>::ea_type P = x.P.get_ea();

          arma_applier_1u(/=);
          }
        }
      else
        {
        typename ProxyCube<T1>::ea_type P = x.P.get_ea();

        arma_applier_1u(/=);
        }
      }
    }
  else
    {
    const ProxyCube<T1>& P = x.P;

    if(use_mp && mp_gate<eT>::eval(x.get_n_elem()))
      {
      arma_applier_3_mp(/=);
      }
    else
      {
      arma_applier_3(/=);
      }
    }
  }



//
// common



template<typename eop_type>
template<typename eT>
arma_hot
arma_inline
eT
eop_core<eop_type>::process(const eT, const eT)
  {
  arma_stop_logic_error("eop_core::process(): unhandled eop_type");
  return eT(0);
  }



template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_plus      >::process(const eT val, const eT k) { return val + k;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_minus_pre >::process(const eT val, const eT k) { return k - val;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_minus_post>::process(const eT val, const eT k) { return val - k;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_times     >::process(const eT val, const eT k) { return val * k;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_div_pre   >::process(const eT val, const eT k) { return k / val;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_scalar_div_post  >::process(const eT val, const eT k) { return val / k;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_square           >::process(const eT val, const eT  ) { return val*val;                  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_neg              >::process(const eT val, const eT  ) { return eop_aux::neg(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_sqrt             >::process(const eT val, const eT  ) { return eop_aux::sqrt(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_log              >::process(const eT val, const eT  ) { return eop_aux::log(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_log2             >::process(const eT val, const eT  ) { return eop_aux::log2(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_log10            >::process(const eT val, const eT  ) { return eop_aux::log10(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_trunc_log        >::process(const eT val, const eT  ) { return    arma::trunc_log(val);  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_exp              >::process(const eT val, const eT  ) { return eop_aux::exp(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_exp2             >::process(const eT val, const eT  ) { return eop_aux::exp2(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_exp10            >::process(const eT val, const eT  ) { return eop_aux::exp10(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_trunc_exp        >::process(const eT val, const eT  ) { return    arma::trunc_exp(val);  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_cos              >::process(const eT val, const eT  ) { return eop_aux::cos(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_sin              >::process(const eT val, const eT  ) { return eop_aux::sin(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_tan              >::process(const eT val, const eT  ) { return eop_aux::tan(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_acos             >::process(const eT val, const eT  ) { return eop_aux::acos(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_asin             >::process(const eT val, const eT  ) { return eop_aux::asin(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_atan             >::process(const eT val, const eT  ) { return eop_aux::atan(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_cosh             >::process(const eT val, const eT  ) { return eop_aux::cosh(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_sinh             >::process(const eT val, const eT  ) { return eop_aux::sinh(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_tanh             >::process(const eT val, const eT  ) { return eop_aux::tanh(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_acosh            >::process(const eT val, const eT  ) { return eop_aux::acosh(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_asinh            >::process(const eT val, const eT  ) { return eop_aux::asinh(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_atanh            >::process(const eT val, const eT  ) { return eop_aux::atanh(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_eps              >::process(const eT val, const eT  ) { return eop_aux::direct_eps(val); }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_abs              >::process(const eT val, const eT  ) { return eop_aux::arma_abs(val);   }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_arg              >::process(const eT val, const eT  ) { return arma_arg<eT>::eval(val);  }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_conj             >::process(const eT val, const eT  ) { return eop_aux::conj(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_pow              >::process(const eT val, const eT k) { return eop_aux::pow(val, k);     }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_floor            >::process(const eT val, const eT  ) { return eop_aux::floor(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_ceil             >::process(const eT val, const eT  ) { return eop_aux::ceil(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_round            >::process(const eT val, const eT  ) { return eop_aux::round(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_trunc            >::process(const eT val, const eT  ) { return eop_aux::trunc(val);      }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_sign             >::process(const eT val, const eT  ) { return eop_aux::sign(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_erf              >::process(const eT val, const eT  ) { return eop_aux::erf(val);        }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_erfc             >::process(const eT val, const eT  ) { return eop_aux::erfc(val);       }

template<> template<typename eT> arma_hot arma_inline eT
eop_core<eop_lgamma           >::process(const eT val, const eT  ) { return eop_aux::lgamma(val);     }


#undef arma_applier_1u
#undef arma_applier_1a
#undef arma_applier_2
#undef arma_applier_3

#undef arma_applier_1_mp
#undef arma_applier_2_mp
#undef arma_applier_3_mp


//! @}
