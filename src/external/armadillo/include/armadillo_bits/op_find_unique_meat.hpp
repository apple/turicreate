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


//! \addtogroup op_find_unique
//! @{



template<typename T1>
inline
bool
op_find_unique::apply_helper(Mat<uword>& out, const Proxy<T1>& P, const bool ascending_indices)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)  { out.set_size(0,1);             return true; }
  if(n_elem == 1)  { out.set_size(1,1); out[0] = 0; return true; }

  uvec indices(n_elem);

  std::vector< arma_find_unique_packet<eT> > packet_vec(n_elem);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i<n_elem; ++i)
      {
      const eT val = Pea[i];

      if(arma_isnan(val))  { return false; }

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

      if(arma_isnan(val))  { return false; }

      packet_vec[i].val   = val;
      packet_vec[i].index = i;

      ++i;
      }
    }

  arma_find_unique_comparator<eT> comparator;

  std::sort( packet_vec.begin(), packet_vec.end(), comparator );

  uword* indices_mem = indices.memptr();

  indices_mem[0] = packet_vec[0].index;

  uword count = 1;

  for(uword i=1; i < n_elem; ++i)
    {
    const eT diff = packet_vec[i-1].val - packet_vec[i].val;

    if(diff != eT(0))
      {
      indices_mem[count] = packet_vec[i].index;
      ++count;
      }
    }

  out.steal_mem_col(indices,count);

  if(ascending_indices)  { std::sort(out.begin(), out.end()); }

  return true;
  }



template<typename T1>
inline
void
op_find_unique::apply(Mat<uword>& out, const mtOp<uword,T1,op_find_unique>& in)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(in.m);

  const bool ascending_indices = (in.aux_uword_a == uword(1));

  const bool all_non_nan = op_find_unique::apply_helper(out, P, ascending_indices);

  if(all_non_nan == false)
    {
    arma_debug_check( true, "find_unique(): detected NaN" );

    out.reset();
    }
  }



//! @}
