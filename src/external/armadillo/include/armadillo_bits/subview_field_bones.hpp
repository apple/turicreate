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


//! \addtogroup subview_field
//! @{


//! Class for storing data required to construct or apply operations to a subfield
//! (i.e. where the subfield starts and ends as well as a reference/pointer to the original field),
template<typename oT>
class subview_field
  {
  public:

  typedef oT object_type;

  const field<oT>& f;

  const uword aux_row1;
  const uword aux_col1;
  const uword aux_slice1;

  const uword n_rows;
  const uword n_cols;
  const uword n_slices;
  const uword n_elem;


  protected:

  arma_inline subview_field(const field<oT>& in_f, const uword in_row1, const uword in_col1, const uword in_n_rows, const uword in_n_cols);
  arma_inline subview_field(const field<oT>& in_f, const uword in_row1, const uword in_col1, const uword in_slice1, const uword in_n_rows, const uword in_n_cols, const uword in_n_slices);


  public:

  inline ~subview_field();

  inline void operator= (const field<oT>& x);
  inline void operator= (const subview_field& x);

  arma_inline       oT& operator[](const uword i);
  arma_inline const oT& operator[](const uword i) const;

  arma_inline       oT& operator()(const uword i);
  arma_inline const oT& operator()(const uword i) const;

  arma_inline       oT&         at(const uword row, const uword col);
  arma_inline const oT&         at(const uword row, const uword col) const;

  arma_inline       oT&         at(const uword row, const uword col, const uword slice);
  arma_inline const oT&         at(const uword row, const uword col, const uword slice) const;

  arma_inline       oT& operator()(const uword row, const uword col);
  arma_inline const oT& operator()(const uword row, const uword col) const;

  arma_inline       oT& operator()(const uword row, const uword col, const uword slice);
  arma_inline const oT& operator()(const uword row, const uword col, const uword slice) const;

  arma_inline bool is_empty() const;

  inline bool check_overlap(const subview_field& x) const;

  inline void print(const std::string extra_text = "") const;
  inline void print(std::ostream& user_stream, const std::string extra_text = "") const;

  template<typename functor> inline void for_each(functor F);
  template<typename functor> inline void for_each(functor F) const;

  inline void fill(const oT& x);

  inline static void extract(field<oT>& out, const subview_field& in);


  private:

  friend class field<oT>;


  subview_field();
  //subview_field(const subview_field&);
  };


//! @}
