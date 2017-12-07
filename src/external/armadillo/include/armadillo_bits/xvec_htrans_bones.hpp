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
class xvec_htrans : public Base<eT, xvec_htrans<eT> >
  {
  public:

  typedef eT                                       elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;

  static const bool is_row = false;
  static const bool is_col = false;

  arma_aligned const eT* const mem;

  const uword n_rows;
  const uword n_cols;
  const uword n_elem;


  inline explicit xvec_htrans(const eT* const in_mem, const uword in_n_rows, const uword in_n_cols);

  inline void extract(Mat<eT>& out) const;

  inline eT operator[](const uword ii) const;
  inline eT at_alt    (const uword ii) const;

  inline eT at        (const uword in_row, const uword in_col) const;
  };



//! @}
