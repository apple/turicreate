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
class eOpCube : public BaseCube<typename T1::elem_type, eOpCube<T1, eop_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;

  static const bool use_at      = ProxyCube<T1>::use_at;
  static const bool use_mp      = ProxyCube<T1>::use_mp || eop_type::use_mp;
  static const bool has_subview = ProxyCube<T1>::has_subview;

  arma_aligned const ProxyCube<T1> P;
  arma_aligned       elem_type     aux;          //!< storage of auxiliary data, user defined format
  arma_aligned       uword         aux_uword_a;  //!< storage of auxiliary data, uword format
  arma_aligned       uword         aux_uword_b;  //!< storage of auxiliary data, uword format
  arma_aligned       uword         aux_uword_c;  //!< storage of auxiliary data, uword format

  inline         ~eOpCube();
  inline explicit eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m);
  inline          eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const elem_type in_aux);
  inline          eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b);
  inline          eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c);
  inline          eOpCube(const BaseCube<typename T1::elem_type, T1>& in_m, const elem_type in_aux, const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c);

  arma_inline uword get_n_rows()       const;
  arma_inline uword get_n_cols()       const;
  arma_inline uword get_n_elem_slice() const;
  arma_inline uword get_n_slices()     const;
  arma_inline uword get_n_elem()       const;

  arma_inline elem_type operator[] (const uword i)                                       const;
  arma_inline elem_type at         (const uword row, const uword col, const uword slice) const;
  arma_inline elem_type at_alt     (const uword i)                                       const;
  };



//! @}
