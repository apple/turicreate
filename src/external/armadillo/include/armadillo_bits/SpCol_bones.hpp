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


//! \addtogroup SpCol
//! @{


//! Class for sparse column vectors (matrices with only one column)
template<typename eT>
class SpCol : public SpMat<eT>
  {
  public:

  typedef eT                                elem_type;
  typedef typename get_pod_type<eT>::result pod_type;

  static const bool is_row = false;
  static const bool is_col = true;


  inline          SpCol();
  inline explicit SpCol(const uword n_elem);
  inline explicit SpCol(const uword in_rows, const uword in_cols);
  inline explicit SpCol(const SizeMat& s);

  inline            SpCol(const char*        text);
  inline SpCol& operator=(const char*        text);

  inline            SpCol(const std::string& text);
  inline SpCol& operator=(const std::string& text);

  inline SpCol& operator=(const eT val);

  template<typename T1> inline            SpCol(const Base<eT,T1>& X);
  template<typename T1> inline SpCol& operator=(const Base<eT,T1>& X);

  template<typename T1> inline            SpCol(const SpBase<eT,T1>& X);
  template<typename T1> inline SpCol& operator=(const SpBase<eT,T1>& X);

  template<typename T1, typename T2>
  inline explicit SpCol(const SpBase<pod_type,T1>& A, const SpBase<pod_type,T2>& B);

  inline void shed_row (const uword row_num);
  inline void shed_rows(const uword in_row1, const uword in_row2);

  // inline void insert_rows(const uword row_num, const uword N, const bool set_to_zero = true);


  typedef typename SpMat<eT>::iterator       row_iterator;
  typedef typename SpMat<eT>::const_iterator const_row_iterator;

  inline       row_iterator begin_row(const uword row_num = 0);
  inline const_row_iterator begin_row(const uword row_num = 0) const;

  inline       row_iterator end_row  (const uword row_num = 0);
  inline const_row_iterator end_row  (const uword row_num = 0) const;


  #ifdef ARMA_EXTRA_SPCOL_PROTO
    #include ARMA_INCFILE_WRAP(ARMA_EXTRA_SPCOL_PROTO)
  #endif
  };
