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


//! \addtogroup eOpCube
//! @{



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m)
  : P (in_m.get_ref())
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const typename T1::elem_type in_aux)
  : P   (in_m.get_ref())
  , aux (in_aux)
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b)
  : P           (in_m.get_ref())
  , aux_uword_a (in_aux_uword_a)
  , aux_uword_b (in_aux_uword_b)
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c)
  : P           (in_m.get_ref())
  , aux_uword_a (in_aux_uword_a)
  , aux_uword_b (in_aux_uword_b)
  , aux_uword_c (in_aux_uword_c)
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const typename T1::elem_type in_aux, const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c)
  : P           (in_m.get_ref())
  , aux         (in_aux)
  , aux_uword_a (in_aux_uword_a)
  , aux_uword_b (in_aux_uword_b)
  , aux_uword_c (in_aux_uword_c)
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
inline
eOpCube<T1, eop_type>::~eOpCube()
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename eop_type>
arma_inline
uword
eOpCube<T1, eop_type>::get_n_rows() const
  {
  return P.get_n_rows();
  }



template<typename T1, typename eop_type>
arma_inline
uword
eOpCube<T1, eop_type>::get_n_cols() const
  {
  return P.get_n_cols();
  }



template<typename T1, typename eop_type>
arma_inline
uword
eOpCube<T1, eop_type>::get_n_elem_slice() const
  {
  return P.get_n_elem_slice();
  }



template<typename T1, typename eop_type>
arma_inline
uword
eOpCube<T1, eop_type>::get_n_slices() const
  {
  return P.get_n_slices();
  }



template<typename T1, typename eop_type>
arma_inline
uword
eOpCube<T1, eop_type>::get_n_elem() const
  {
  return P.get_n_elem();
  }



template<typename T1, typename eop_type>
arma_inline
typename T1::elem_type
eOpCube<T1, eop_type>::operator[] (const uword i) const
  {
  return eop_core<eop_type>::process(P[i], aux);
  }



template<typename T1, typename eop_type>
arma_inline
typename T1::elem_type
eOpCube<T1, eop_type>::at(const uword row, const uword col, const uword slice) const
  {
  return eop_core<eop_type>::process(P.at(row, col, slice), aux);
  }



template<typename T1, typename eop_type>
arma_inline
typename T1::elem_type
eOpCube<T1, eop_type>::at_alt(const uword i) const
  {
  return eop_core<eop_type>::process(P.at_alt(i), aux);
  }



//! @}
