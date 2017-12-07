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



//! \addtogroup op_clamp
//! @{



template<typename T1>
inline
void
op_clamp::apply(Mat<typename T1::elem_type>& out, const mtOp<typename T1::elem_type, T1, op_clamp>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(in.m);

  if(P.is_alias(out) && (is_Mat<T1>::value == false))
    {
    Mat<eT> tmp;

    op_clamp::apply_noalias(tmp, P, in.aux, in.aux_out_eT);

    out.steal_mem(tmp);
    }
  else
    {
    op_clamp::apply_noalias(out, P, in.aux, in.aux_out_eT);
    }
  }



template<typename T1>
inline
void
op_clamp::apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const typename T1::elem_type min_val, const typename T1::elem_type max_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();

  out.set_size(n_rows, n_cols);

  eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword N = P.get_n_elem();

    typename Proxy<T1>::ea_type A = P.get_ea();

    uword j;
    for(j=1; j<N; j+=2)
      {
      eT val_i = A[j-1];
      eT val_j = A[j  ];

      val_i = (val_i < min_val) ? min_val : ((val_i > max_val) ? max_val : val_i);
      val_j = (val_j < min_val) ? min_val : ((val_j > max_val) ? max_val : val_j);

      (*out_mem) = val_i;  out_mem++;
      (*out_mem) = val_j;  out_mem++;
      }

    const uword i = j-1;

    if(i < N)
      {
      eT val_i = A[i];

      val_i = (val_i < min_val) ? min_val : ((val_i > max_val) ? max_val : val_i);

      (*out_mem) = val_i;
      }
    }
  else
    {
    for(uword col=0; col<n_cols; ++col)
    for(uword row=0; row<n_rows; ++row)
      {
      eT val = P.at(row,col);

      val = (val < min_val) ? min_val : ((val > max_val) ? max_val : val);

      (*out_mem) = val;  out_mem++;
      }
    }
  }



template<typename T1>
inline
void
op_clamp::apply(Cube<typename T1::elem_type>& out, const mtOpCube<typename T1::elem_type, T1, op_clamp>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const ProxyCube<T1> P(in.m);

  if(P.is_alias(out) && (is_Cube<T1>::value == false))
    {
    Cube<eT> tmp;

    op_clamp::apply_noalias(tmp, P, in.aux, in.aux_out_eT);

    out.steal_mem(tmp);
    }
  else
    {
    op_clamp::apply_noalias(out, P, in.aux, in.aux_out_eT);
    }
  }



template<typename T1>
inline
void
op_clamp::apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const typename T1::elem_type min_val, const typename T1::elem_type max_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = P.get_n_rows();
  const uword n_cols   = P.get_n_cols();
  const uword n_slices = P.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  eT* out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    const uword N = P.get_n_elem();

    typename ProxyCube<T1>::ea_type A = P.get_ea();

    uword j;
    for(j=1; j<N; j+=2)
      {
      eT val_i = A[j-1];
      eT val_j = A[j  ];

      val_i = (val_i < min_val) ? min_val : ((val_i > max_val) ? max_val : val_i);
      val_j = (val_j < min_val) ? min_val : ((val_j > max_val) ? max_val : val_j);

      (*out_mem) = val_i;  out_mem++;
      (*out_mem) = val_j;  out_mem++;
      }

    const uword i = j-1;

    if(i < N)
      {
      eT val_i = A[i];

      val_i = (val_i < min_val) ? min_val : ((val_i > max_val) ? max_val : val_i);

      (*out_mem) = val_i;
      }
    }
  else
    {
    for(uword k=0; k < n_slices; ++k)
    for(uword j=0; j < n_cols;   ++j)
    for(uword i=0; i < n_rows;   ++i)
      {
      eT val = P.at(i,j,k);

      val = (val < min_val) ? min_val : ((val > max_val) ? max_val : val);

      (*out_mem) = val;  out_mem++;
      }
    }
  }



//! @}
