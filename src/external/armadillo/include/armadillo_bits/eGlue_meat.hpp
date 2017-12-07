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


//! \addtogroup eGlue
//! @{



template<typename T1, typename T2, typename eglue_type>
arma_inline
eGlue<T1,T2,eglue_type>::~eGlue()
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
eGlue<T1,T2,eglue_type>::eGlue(const T1& in_A, const T2& in_B)
  : P1(in_A)
  , P2(in_B)
  {
  arma_extra_debug_sigprint();

  // arma_debug_assert_same_size( P1, P2, eglue_type::text() );
  arma_debug_assert_same_size
    (
    P1.get_n_rows(), P1.get_n_cols(),
    P2.get_n_rows(), P2.get_n_cols(),
    eglue_type::text()
    );
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlue<T1,T2,eglue_type>::get_n_rows() const
  {
  return is_row ? 1 : P1.get_n_rows();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlue<T1,T2,eglue_type>::get_n_cols() const
  {
  return is_col ? 1 : P1.get_n_cols();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
uword
eGlue<T1,T2,eglue_type>::get_n_elem() const
  {
  return P1.get_n_elem();
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlue<T1,T2,eglue_type>::operator[] (const uword ii) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1[ii] + P2[ii]; }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1[ii] - P2[ii]; }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1[ii] / P2[ii]; }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1[ii] * P2[ii]; }
  else return eT(0);
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlue<T1,T2,eglue_type>::at(const uword row, const uword col) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1.at(row,col) + P2.at(row,col); }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1.at(row,col) - P2.at(row,col); }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1.at(row,col) / P2.at(row,col); }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1.at(row,col) * P2.at(row,col); }
  else return eT(0);
  }



template<typename T1, typename T2, typename eglue_type>
arma_inline
typename T1::elem_type
eGlue<T1,T2,eglue_type>::at_alt(const uword ii) const
  {
  // the optimiser will keep only one return statement

  typedef typename T1::elem_type eT;

       if(is_same_type<eglue_type, eglue_plus >::yes) { return P1.at_alt(ii) + P2.at_alt(ii); }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return P1.at_alt(ii) - P2.at_alt(ii); }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return P1.at_alt(ii) / P2.at_alt(ii); }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return P1.at_alt(ii) * P2.at_alt(ii); }
  else return eT(0);
  }



//! @}
