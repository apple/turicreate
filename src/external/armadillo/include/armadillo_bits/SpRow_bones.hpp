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


//! \addtogroup SpRow
//! @{


//! Class for sparse row vectors (sparse matrices with only one row)
template<typename eT>
class SpRow : public SpMat<eT>
  {
  public:

  typedef eT                                elem_type;
  typedef typename get_pod_type<eT>::result pod_type;

  static const bool is_row = true;
  static const bool is_col = false;


  inline          SpRow();
  inline explicit SpRow(const uword N);
  inline explicit SpRow(const uword in_rows, const uword in_cols);
  inline explicit SpRow(const SizeMat& s);

  inline            SpRow(const char*        text);
  inline SpRow& operator=(const char*        text);

  inline            SpRow(const std::string& text);
  inline SpRow& operator=(const std::string& text);

  inline SpRow& operator=(const eT val);

  template<typename T1> inline            SpRow(const Base<eT,T1>& X);
  template<typename T1> inline SpRow& operator=(const Base<eT,T1>& X);

  template<typename T1> inline            SpRow(const SpBase<eT,T1>& X);
  template<typename T1> inline SpRow& operator=(const SpBase<eT,T1>& X);

  template<typename T1, typename T2>
  inline explicit SpRow(const SpBase<pod_type,T1>& A, const SpBase<pod_type,T2>& B);

  inline void shed_col (const uword col_num);
  inline void shed_cols(const uword in_col1, const uword in_col2);

  // inline void insert_cols(const uword col_num, const uword N, const bool set_to_zero = true);


  typedef typename SpMat<eT>::iterator       row_iterator;
  typedef typename SpMat<eT>::const_iterator const_row_iterator;

  inline       row_iterator begin_row(const uword row_num = 0);
  inline const_row_iterator begin_row(const uword row_num = 0) const;

  inline       row_iterator end_row(const uword row_num = 0);
  inline const_row_iterator end_row(const uword row_num = 0) const;

  #ifdef ARMA_EXTRA_SPROW_PROTO
    #include ARMA_INCFILE_WRAP(ARMA_EXTRA_SPROW_PROTO)
  #endif
  };



//! @}
