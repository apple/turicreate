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


//! \addtogroup spop_misc
//! @{



namespace priv
  {
  template<typename eT>
  struct functor_scalar_times
    {
    const eT k;

    functor_scalar_times(const eT in_k) : k(in_k) {}

    arma_inline eT operator()(const eT val) const { return val * k; }
    };
  }



template<typename T1>
inline
void
spop_scalar_times::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_scalar_times>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if(in.aux != eT(0))
    {
    out.init_xform(in.m, priv::functor_scalar_times<eT>(in.aux));
    }
  else
    {
    const SpProxy<T1> P(in.m);

    out.zeros( P.get_n_rows(), P.get_n_cols() );
    }
  }



namespace priv
  {
  struct functor_square
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return val*val; }
    };
  }



template<typename T1>
inline
void
spop_square::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_square>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_square());
  }



namespace priv
  {
  struct functor_sqrt
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::sqrt(val); }
    };
  }



template<typename T1>
inline
void
spop_sqrt::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_sqrt>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_sqrt());
  }



namespace priv
  {
  struct functor_abs
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::arma_abs(val); }
    };
  }



template<typename T1>
inline
void
spop_abs::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_abs>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_abs());
  }



namespace priv
  {
  struct functor_cx_abs
    {
    template<typename T>
    arma_inline T operator()(const std::complex<T>& val) const { return std::abs(val); }
    };
  }



template<typename T1>
inline
void
spop_cx_abs::apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_cx_abs>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform_mt(in.m, priv::functor_cx_abs());
  }



namespace priv
  {
  struct functor_arg
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return arma_arg<eT>::eval(val); }
    };
  }



template<typename T1>
inline
void
spop_arg::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_arg>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_arg());
  }



namespace priv
  {
  struct functor_cx_arg
    {
    template<typename T>
    arma_inline T operator()(const std::complex<T>& val) const { return std::arg(val); }
    };
  }



template<typename T1>
inline
void
spop_cx_arg::apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_cx_arg>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform_mt(in.m, priv::functor_cx_arg());
  }



namespace priv
  {
  struct functor_real
    {
    template<typename T>
    arma_inline T operator()(const std::complex<T>& val) const { return val.real(); }
    };
  }



template<typename T1>
inline
void
spop_real::apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_real>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform_mt(in.m, priv::functor_real());
  }



namespace priv
  {
  struct functor_imag
    {
    template<typename T>
    arma_inline T operator()(const std::complex<T>& val) const { return val.imag(); }
    };
  }



template<typename T1>
inline
void
spop_imag::apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_imag>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform_mt(in.m, priv::functor_imag());
  }



namespace priv
  {
  struct functor_conj
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::conj(val); }
    };
  }



template<typename T1>
inline
void
spop_conj::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_conj>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_conj());
  }



template<typename T1>
inline
void
spop_repmat::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_repmat>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_spmat<T1> U(in.m);
  const SpMat<eT>& X =   U.M;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  const uword copies_per_row = in.aux_uword_a;
  const uword copies_per_col = in.aux_uword_b;

  // out.set_size(X_n_rows * copies_per_row, X_n_cols * copies_per_col);
  //
  // const uword out_n_rows = out.n_rows;
  // const uword out_n_cols = out.n_cols;
  //
  // if( (out_n_rows > 0) && (out_n_cols > 0) )
  //   {
  //   for(uword col = 0; col < out_n_cols; col += X_n_cols)
  //   for(uword row = 0; row < out_n_rows; row += X_n_rows)
  //     {
  //     out.submat(row, col, row+X_n_rows-1, col+X_n_cols-1) = X;
  //     }
  //   }

  SpMat<eT> tmp(X_n_rows * copies_per_row, X_n_cols);

  if(tmp.n_elem > 0)
    {
    for(uword row = 0; row < tmp.n_rows; row += X_n_rows)
      {
      tmp.submat(row, 0, row+X_n_rows-1, X_n_cols-1) = X;
      }
    }

  // tmp contains copies of the input matrix, so no need to check for aliasing

  out.set_size(X_n_rows * copies_per_row, X_n_cols * copies_per_col);

  const uword out_n_rows = out.n_rows;
  const uword out_n_cols = out.n_cols;

  if( (out_n_rows > 0) && (out_n_cols > 0) )
    {
    for(uword col = 0; col < out_n_cols; col += X_n_cols)
      {
      out.submat(0, col, out_n_rows-1, col+X_n_cols-1) = tmp;
      }
    }
  }



template<typename T1>
inline
void
spop_reshape::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_reshape>& in)
  {
  arma_extra_debug_sigprint();

  out = in.m;

  out.reshape(in.aux_uword_a, in.aux_uword_b);
  }



template<typename T1>
inline
void
spop_resize::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_resize>& in)
  {
  arma_extra_debug_sigprint();

  out = in.m;

  out.resize(in.aux_uword_a, in.aux_uword_b);
  }



namespace priv
  {
  struct functor_floor
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::floor(val); }
    };
  }



template<typename T1>
inline
void
spop_floor::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_floor>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_floor());
  }



namespace priv
  {
  struct functor_ceil
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::ceil(val); }
    };
  }



template<typename T1>
inline
void
spop_ceil::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_ceil>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_ceil());
  }



namespace priv
  {
  struct functor_round
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::round(val); }
    };
  }



template<typename T1>
inline
void
spop_round::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_round>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_round());
  }



namespace priv
  {
  struct functor_trunc
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::trunc(val); }
    };
  }



template<typename T1>
inline
void
spop_trunc::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_trunc>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_trunc());
  }



namespace priv
  {
  struct functor_sign
    {
    template<typename eT>
    arma_inline eT operator()(const eT val) const { return eop_aux::sign(val); }
    };
  }



template<typename T1>
inline
void
spop_sign::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_sign>& in)
  {
  arma_extra_debug_sigprint();

  out.init_xform(in.m, priv::functor_sign());
  }



template<typename T1>
inline
void
spop_diagvec::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_diagvec>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_spmat<T1> U(in.m);

  const SpMat<eT>& X = U.M;

  const uword a = in.aux_uword_a;
  const uword b = in.aux_uword_b;

  const uword row_offset = (b >  0) ? a : 0;
  const uword col_offset = (b == 0) ? a : 0;

  arma_debug_check
    (
    ((row_offset > 0) && (row_offset >= X.n_rows)) || ((col_offset > 0) && (col_offset >= X.n_cols)),
    "diagvec(): requested diagonal out of bounds"
    );

  const uword len = (std::min)(X.n_rows - row_offset, X.n_cols - col_offset);

  Col<eT> cache(len);
  eT* cache_mem = cache.memptr();

  uword n_nonzero = 0;

  for(uword i=0; i < len; ++i)
    {
    const eT val = X.at(i + row_offset, i + col_offset);

    cache_mem[i] = val;

    n_nonzero += (val != eT(0)) ? uword(1) : uword(0);
    }

  out.set_size(len, 1);

  out.mem_resize(n_nonzero);

  uword count = 0;
  for(uword i=0; i < len; ++i)
    {
    const eT val = cache_mem[i];

    if(val != eT(0))
      {
      access::rw(out.row_indices[count]) = i;
      access::rw(out.values[count])      = val;
      ++count;
      }
    }

  access::rw(out.col_ptrs[0]) = 0;
  access::rw(out.col_ptrs[1]) = n_nonzero;
  }



//! @}
