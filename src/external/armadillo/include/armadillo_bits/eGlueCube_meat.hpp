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


//! \addtogroup eGlueCube
//! @{



template<typename T1, typename T2, typename eglue_type>
arma_inline
eGlueCube<T1,T2,eglue_type>::~eGlueCube()
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
eGlueCube<T1,T2,eglue_type>::eGlueCube(const T1& in_A, const T2& in_B)
  : P1(in_A)
  , P2(in_B)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size
    (
    P1.get_n_rows(), P1.get_n_cols(), P1.get_n_slices(),
    P2.get_n_rows(), P2.get_n_cols(), P2.get_n_slices(),
    eglue_type::text()
    );
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlueCube<T1,T2,eglue_type>::get_n_rows() const
  {
  return P1.get_n_rows();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlueCube<T1,T2,eglue_type>::get_n_cols() const
  {
  return P1.get_n_cols();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlueCube<T1,T2,eglue_type>::get_n_slices() const
  {
  return P1.get_n_slices();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlueCube<T1,T2,eglue_type>::get_n_elem_slice() const
  {
  return P1.get_n_elem_slice();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlueCube<T1,T2,eglue_type>::get_n_elem() const
  {
  return P1.get_n_elem();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlueCube<T1,T2,eglue_type>::operator[] (const uword i) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1[i] + P2[i]; }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1[i] - P2[i]; }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1[i] / P2[i]; }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1[i] * P2[i]; }
  else return eT(0);
  }


template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlueCube<T1,T2,eglue_type>::at(const uword row, const uword col, const uword slice) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1.at(row,col,slice) + P2.at(row,col,slice); }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1.at(row,col,slice) - P2.at(row,col,slice); }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1.at(row,col,slice) / P2.at(row,col,slice); }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1.at(row,col,slice) * P2.at(row,col,slice); }
  else return eT(0);
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlueCube<T1,T2,eglue_type>::at_alt(const uword i) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1.at_alt(i) + P2.at_alt(i); }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1.at_alt(i) - P2.at_alt(i); }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1.at_alt(i) / P2.at_alt(i); }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1.at_alt(i) * P2.at_alt(i); }
  else return eT(0);
  }


//! @}
