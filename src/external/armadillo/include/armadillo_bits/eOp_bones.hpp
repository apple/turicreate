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


//! \addtogroup eOp
//! @{



template<typename T1, typename eop_type>
class eOp : public Base<typename T1::elem_type, eOp<T1, eop_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef          Proxy<T1>                       proxy_type;

  static const bool use_at      = Proxy<T1>::use_at;
  static const bool use_mp      = Proxy<T1>::use_mp || eop_type::use_mp;
  static const bool has_subview = Proxy<T1>::has_subview;
  static const bool fake_mat    = Proxy<T1>::fake_mat;

  static const bool is_row = Proxy<T1>::is_row;
  static const bool is_col = Proxy<T1>::is_col;

  arma_aligned const Proxy<T1> P;

  arma_aligned       elem_type aux;          //!< storage of auxiliary data, user defined format
  arma_aligned       uword     aux_uword_a;  //!< storage of auxiliary data, uword format
  arma_aligned       uword     aux_uword_b;  //!< storage of auxiliary data, uword format

  inline         ~eOp();
  inline explicit eOp(const T1& in_m);
  inline          eOp(const T1& in_m, const elem_type in_aux);
  inline          eOp(const T1& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b);
  inline          eOp(const T1& in_m, const elem_type in_aux, const uword in_aux_uword_a, const uword in_aux_uword_b);

  arma_inline uword get_n_rows() const;
  arma_inline uword get_n_cols() const;
  arma_inline uword get_n_elem() const;

  arma_inline elem_type operator[] (const uword ii)                   const;
  arma_inline elem_type at         (const uword row, const uword col) const;
  arma_inline elem_type at_alt     (const uword ii)                   const;
  };



//! @}
