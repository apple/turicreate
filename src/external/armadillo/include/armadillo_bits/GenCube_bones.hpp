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


//! \addtogroup GenCube
//! @{


//! support class for generator functions (eg. zeros, randu, randn, ...)
template<typename eT, typename gen_type>
class GenCube
  : public BaseCube<eT, GenCube<eT, gen_type> >
  , public GenSpecialiser<eT, is_same_type<gen_type, gen_zeros>::yes, is_same_type<gen_type, gen_ones>::yes, is_same_type<gen_type, gen_randu>::yes, is_same_type<gen_type, gen_randn>::yes>
  {
  public:

  typedef          eT                              elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;

  static const bool use_at    = false;
  static const bool is_simple = (is_same_type<gen_type, gen_ones>::value) || (is_same_type<gen_type, gen_zeros>::value);

  arma_aligned const uword n_rows;
  arma_aligned const uword n_cols;
  arma_aligned const uword n_slices;

  arma_inline  GenCube(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices);
  arma_inline ~GenCube();

  arma_inline eT operator[] (const uword i)                                       const;
  arma_inline eT at         (const uword row, const uword col, const uword slice) const;
  arma_inline eT at_alt     (const uword i)                                       const;

  inline void apply              (Cube<eT>& out) const;
  inline void apply_inplace_plus (Cube<eT>& out) const;
  inline void apply_inplace_minus(Cube<eT>& out) const;
  inline void apply_inplace_schur(Cube<eT>& out) const;
  inline void apply_inplace_div  (Cube<eT>& out) const;

  inline void apply(subview_cube<elem_type>& out) const;
  };



//! @}
