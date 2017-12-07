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


//! \addtogroup xtrans_mat
//! @{


template<typename eT, bool do_conj>
class xtrans_mat : public Base<eT, xtrans_mat<eT, do_conj> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;

  static const bool is_row = false;
  static const bool is_col = false;

  arma_aligned const   Mat<eT>& X;
  arma_aligned mutable Mat<eT>  Y;

  arma_aligned const uword n_rows;
  arma_aligned const uword n_cols;
  arma_aligned const uword n_elem;

  inline explicit xtrans_mat(const Mat<eT>& in_X);

  inline void extract(Mat<eT>& out) const;

  inline eT operator[](const uword ii) const;
  inline eT at_alt    (const uword ii) const;

  arma_inline eT at(const uword in_row, const uword in_col) const;
  };



//! @}
