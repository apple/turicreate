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


//! \addtogroup xvec_htrans
//! @{


template<typename eT>
inline
xvec_htrans<eT>::xvec_htrans(const eT* const in_mem, const uword in_n_rows, const uword in_n_cols)
  : mem   (in_mem             )
  , n_rows(in_n_cols          )  // deliberately swapped
  , n_cols(in_n_rows          )
  , n_elem(in_n_rows*in_n_cols)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
xvec_htrans<eT>::extract(Mat<eT>& out) const
  {
  arma_extra_debug_sigprint();

  // NOTE: this function assumes that matrix 'out' has already been set to the correct size

  const eT*  in_mem = mem;
        eT* out_mem = out.memptr();

  const uword N = n_elem;

  for(uword ii=0; ii < N; ++ii)
    {
    out_mem[ii] = access::alt_conj( in_mem[ii] );
    }
  }



template<typename eT>
inline
eT
xvec_htrans<eT>::operator[](const uword ii) const
  {
  return access::alt_conj( mem[ii] );
  }



template<typename eT>
inline
eT
xvec_htrans<eT>::at_alt(const uword ii) const
  {
  return access::alt_conj( mem[ii] );
  }



template<typename eT>
inline
eT
xvec_htrans<eT>::at(const uword in_row, const uword in_col) const
  {
  //return (n_rows == 1) ? access::alt_conj( mem[in_col] ) : access::alt_conj( mem[in_row] );

  return access::alt_conj( mem[in_row + in_col] );  // either in_row or in_col must be zero, as we're storing a vector
  }



//! @}
