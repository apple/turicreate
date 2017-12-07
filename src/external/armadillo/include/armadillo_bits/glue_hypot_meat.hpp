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


//! \addtogroup glue_hypot
//! @{



template<typename T1, typename T2>
inline
void
glue_hypot::apply(Mat<typename T1::elem_type>& out, const Glue<T1, T2, glue_hypot>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P1(expr.A);
  const Proxy<T2> P2(expr.B);

  arma_assert_same_size(P1, P2, "hypot()");

  const bool bad_alias = ( (Proxy<T1>::has_subview && P1.is_alias(out)) || (Proxy<T2>::has_subview && P2.is_alias(out)) );

  if(bad_alias == false)
    {
    glue_hypot::apply_noalias(out, P1, P2);
    }
  else
    {
    Mat<eT> tmp;

    glue_hypot::apply_noalias(tmp, P1, P2);

    out.steal_mem(tmp);
    }
  }



template<typename T1, typename T2>
inline
void
glue_hypot::apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P1, const Proxy<T2>& P2)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = P1.get_n_rows();
  const uword n_cols = P1.get_n_cols();

  out.set_size(n_rows, n_cols);

  eT* out_mem = out.memptr();

  if( (Proxy<T1>::use_at == false) && (Proxy<T2>::use_at == false) )
    {
    typename Proxy<T1>::ea_type eaP1 = P1.get_ea();
    typename Proxy<T2>::ea_type eaP2 = P2.get_ea();

    const uword N = P1.get_n_elem();

    for(uword i=0; i<N; ++i)
      {
      out_mem[i] = arma_hypot( eaP1[i], eaP2[i] );
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = arma_hypot( P1.at(row,col), P2.at(row,col) );
      out_mem++;
      }
    }
  }



template<typename T1, typename T2>
inline
void
glue_hypot::apply(Cube<typename T1::elem_type>& out, const GlueCube<T1, T2, glue_hypot>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const ProxyCube<T1> P1(expr.A);
  const ProxyCube<T2> P2(expr.B);

  arma_assert_same_size(P1, P2, "hypot()");

  const bool bad_alias = ( (ProxyCube<T1>::has_subview && P1.is_alias(out)) || (ProxyCube<T2>::has_subview && P2.is_alias(out)) );

  if(bad_alias == false)
    {
    glue_hypot::apply_noalias(out, P1, P2);
    }
  else
    {
    Cube<eT> tmp;

    glue_hypot::apply_noalias(tmp, P1, P2);

    out.steal_mem(tmp);
    }
  }



template<typename T1, typename T2>
inline
void
glue_hypot::apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P1, const ProxyCube<T2>& P2)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows   = P1.get_n_rows();
  const uword n_cols   = P1.get_n_cols();
  const uword n_slices = P1.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  eT* out_mem = out.memptr();

  if( (ProxyCube<T1>::use_at == false) && (ProxyCube<T2>::use_at == false) )
    {
    typename ProxyCube<T1>::ea_type eaP1 = P1.get_ea();
    typename ProxyCube<T2>::ea_type eaP2 = P2.get_ea();

    const uword N = P1.get_n_elem();

    for(uword i=0; i<N; ++i)
      {
      out_mem[i] = arma_hypot( eaP1[i], eaP2[i] );
      }
    }
  else
    {
    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      *out_mem = arma_hypot( P1.at(row,col,slice), P2.at(row,col,slice) );
      out_mem++;
      }
    }
  }



//! @}
