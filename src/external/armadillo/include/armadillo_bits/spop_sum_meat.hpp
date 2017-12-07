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


//! \addtogroup spop_sum
//! @{



template<typename T1>
arma_hot
inline
void
spop_sum::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_sum>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "sum(): parameter 'dim' must be 0 or 1" );

  const SpProxy<T1> p(in.m);

  const uword p_n_rows = p.get_n_rows();
  const uword p_n_cols = p.get_n_cols();

  if(p.get_n_nonzero() == 0)
    {
    if(dim == 0)  { out.zeros(1,p_n_cols); }
    if(dim == 1)  { out.zeros(p_n_rows,1); }

    return;
    }

  if(dim == 0) // find the sum in each column
    {
    Row<eT> acc(p_n_cols, fill::zeros);

    if(SpProxy<T1>::use_iterator)
      {
      typename SpProxy<T1>::const_iterator_type it     = p.begin();
      typename SpProxy<T1>::const_iterator_type it_end = p.end();

      while(it != it_end)  { acc[it.col()] += (*it);  ++it; }
      }
    else
      {
      for(uword col = 0; col < p_n_cols; ++col)
        {
        acc[col] = arrayops::accumulate
          (
          &p.get_values()[p.get_col_ptrs()[col]],
          p.get_col_ptrs()[col + 1] - p.get_col_ptrs()[col]
          );
        }
      }

    out = acc;
    }
  else
  if(dim == 1)  // find the sum in each row
    {
    Col<eT> acc(p_n_rows, fill::zeros);

    typename SpProxy<T1>::const_iterator_type it     = p.begin();
    typename SpProxy<T1>::const_iterator_type it_end = p.end();

    while(it != it_end)  { acc[it.row()] += (*it);  ++it; }

    out = acc;
    }
  }



//! @}
