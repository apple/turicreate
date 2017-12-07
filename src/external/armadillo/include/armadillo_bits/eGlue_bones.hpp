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
class eGlue : public Base<typename T1::elem_type, eGlue<T1, T2, eglue_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;
  typedef          Proxy<T1>                       proxy1_type;
  typedef          Proxy<T2>                       proxy2_type;

  static const bool use_at      = (Proxy<T1>::use_at      || Proxy<T2>::use_at     );
  static const bool use_mp      = (Proxy<T1>::use_mp      || Proxy<T2>::use_mp     );
  static const bool has_subview = (Proxy<T1>::has_subview || Proxy<T2>::has_subview);
  static const bool fake_mat    = (Proxy<T1>::fake_mat    || Proxy<T2>::fake_mat   );

  static const bool is_col = (Proxy<T1>::is_col || Proxy<T2>::is_col);
  static const bool is_row = (Proxy<T1>::is_row || Proxy<T2>::is_row);

  arma_aligned const Proxy<T1> P1;
  arma_aligned const Proxy<T2> P2;

  arma_inline ~eGlue();
  arma_inline  eGlue(const T1& in_A, const T2& in_B);

  arma_inline uword get_n_rows() const;
  arma_inline uword get_n_cols() const;
  arma_inline uword get_n_elem() const;

  arma_inline elem_type operator[] (const uword ii)                   const;
  arma_inline elem_type at         (const uword row, const uword col) const;
  arma_inline elem_type at_alt     (const uword ii)                   const;
  };



//! @}
