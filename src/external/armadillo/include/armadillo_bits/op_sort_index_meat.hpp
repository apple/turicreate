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


//! \addtogroup op_sort_index
//! @{



template<typename T1, bool sort_stable>
inline
bool
arma_sort_index_helper(Mat<uword>& out, const Proxy<T1>& P, const uword sort_type)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_elem = P.get_n_elem();

  out.set_size(n_elem, 1);

  std::vector< arma_sort_index_packet<eT> > packet_vec(n_elem);

  if(Proxy<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      const eT val = P[i];

      if(arma_isnan(val))  { out.soft_reset(); return false; }

      packet_vec[i].val   = val;
      packet_vec[i].index = i;
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    uword i = 0;

    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      const eT val = P.at(row,col);

      if(arma_isnan(val))  { out.soft_reset(); return false; }

      packet_vec[i].val   = val;
      packet_vec[i].index = i;

      ++i;
      }
    }


  if(sort_type == 0)
    {
    // ascend

    arma_sort_index_helper_ascend<eT> comparator;

    if(sort_stable == false)
      {
      std::sort( packet_vec.begin(), packet_vec.end(), comparator );
      }
    else
      {
      std::stable_sort( packet_vec.begin(), packet_vec.end(), comparator );
      }
    }
  else
    {
    // descend

    arma_sort_index_helper_descend<eT> comparator;

    if(sort_stable == false)
      {
      std::sort( packet_vec.begin(), packet_vec.end(), comparator );
      }
    else
      {
      std::stable_sort( packet_vec.begin(), packet_vec.end(), comparator );
      }
    }

  uword* out_mem = out.memptr();

  for(uword i=0; i<n_elem; ++i)
    {
    out_mem[i] = packet_vec[i].index;
    }

  return true;
  }



template<typename T1>
inline
bool
op_sort_index::apply_noalias(Mat<uword>& out, const Proxy<T1>& P, const uword sort_type)
  {
  arma_extra_debug_sigprint();

  return arma_sort_index_helper<T1,false>(out, P, sort_type);
  }



template<typename T1>
inline
void
op_sort_index::apply(Mat<uword>& out, const mtOp<uword,T1,op_sort_index>& in)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(in.m);

  if(P.get_n_elem() == 0)  { out.set_size(0,1); return; }

  const uword sort_type = in.aux_uword_a;

  bool all_non_nan = false;

  if(P.is_alias(out))
    {
    Mat<uword> out2;

    all_non_nan = op_sort_index::apply_noalias(out2, P, sort_type);

    out.steal_mem(out2);
    }
  else
    {
    all_non_nan = op_sort_index::apply_noalias(out, P, sort_type);
    }

  arma_debug_check( (all_non_nan == false), "sort_index(): detected NaN" );
  }



template<typename T1>
inline
bool
op_stable_sort_index::apply_noalias(Mat<uword>& out, const Proxy<T1>& P, const uword sort_type)
  {
  arma_extra_debug_sigprint();

  return arma_sort_index_helper<T1,true>(out, P, sort_type);
  }



template<typename T1>
inline
void
op_stable_sort_index::apply(Mat<uword>& out, const mtOp<uword,T1,op_stable_sort_index>& in)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(in.m);

  if(P.get_n_elem() == 0)  { out.set_size(0,1); return; }

  const uword sort_type = in.aux_uword_a;

  bool all_non_nan = false;

  if(P.is_alias(out))
    {
    Mat<uword> out2;

    all_non_nan = op_stable_sort_index::apply_noalias(out2, P, sort_type);

    out.steal_mem(out2);
    }
  else
    {
    all_non_nan = op_stable_sort_index::apply_noalias(out, P, sort_type);
    }

  arma_debug_check( (all_non_nan == false), "stable_sort_index(): detected NaN" );
  }



//! @}
