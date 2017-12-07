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


//! \addtogroup field
//! @{



struct field_prealloc_n_elem
  {
  static const uword val = 16;
  };



//! A lightweight 1D/2D/3D container for arbitrary objects
//! (the objects must have a copy constructor)

template<typename oT>
class field
  {
  public:

  typedef oT object_type;

  const uword n_rows;     //!< number of rows     (read-only)
  const uword n_cols;     //!< number of columns  (read-only)
  const uword n_slices;   //!< number of slices   (read-only)
  const uword n_elem;     //!< number of elements (read-only)


  private:

  arma_aligned oT** mem;                                     //!< pointers to stored objects
  arma_aligned oT*  mem_local[ field_prealloc_n_elem::val ]; //!< local storage, for small fields


  public:

  inline ~field();
  inline  field();

  inline            field(const field& x);
  inline field& operator=(const field& x);

  inline            field(const subview_field<oT>& x);
  inline field& operator=(const subview_field<oT>& x);

  inline explicit field(const uword n_elem_in);
  inline explicit field(const uword n_rows_in, const uword n_cols_in);
  inline explicit field(const uword n_rows_in, const uword n_cols_in, const uword n_slices_in);
  inline explicit field(const SizeMat&  s);
  inline explicit field(const SizeCube& s);

  inline void  set_size(const uword n_obj_in);
  inline void  set_size(const uword n_rows_in, const uword n_cols_in);
  inline void  set_size(const uword n_rows_in, const uword n_cols_in, const uword n_slices_in);
  inline void  set_size(const SizeMat&  s);
  inline void  set_size(const SizeCube& s);

  #if defined(ARMA_USE_CXX11)
  inline            field(const std::initializer_list<oT>& list);
  inline field& operator=(const std::initializer_list<oT>& list);

  inline            field(const std::initializer_list< std::initializer_list<oT> >& list);
  inline field& operator=(const std::initializer_list< std::initializer_list<oT> >& list);

  inline            field(field&& X);
  inline field& operator=(field&& X);
  #endif

  template<typename oT2>
  inline void copy_size(const field<oT2>& x);

  arma_inline       oT& operator[](const uword i);
  arma_inline const oT& operator[](const uword i) const;

  arma_inline       oT&         at(const uword i);
  arma_inline const oT&         at(const uword i) const;

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

  inline field_injector<field> operator<<(const oT& val);
  inline field_injector<field> operator<<(const injector_end_of_row<>& x);


  inline       subview_field<oT> row(const uword row_num);
  inline const subview_field<oT> row(const uword row_num) const;

  inline       subview_field<oT> col(const uword col_num);
  inline const subview_field<oT> col(const uword col_num) const;

  inline       subview_field<oT> slice(const uword slice_num);
  inline const subview_field<oT> slice(const uword slice_num) const;

  inline       subview_field<oT> rows(const uword in_row1, const uword in_row2);
  inline const subview_field<oT> rows(const uword in_row1, const uword in_row2) const;

  inline       subview_field<oT> cols(const uword in_col1, const uword in_col2);
  inline const subview_field<oT> cols(const uword in_col1, const uword in_col2) const;

  inline       subview_field<oT> slices(const uword in_slice1, const uword in_slice2);
  inline const subview_field<oT> slices(const uword in_slice1, const uword in_slice2) const;

  inline       subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2);
  inline const subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const;

  inline       subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_slice1, const uword in_row2, const uword in_col2, const uword in_slice2);
  inline const subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_slice1, const uword in_row2, const uword in_col2, const uword in_slice2) const;

  inline       subview_field<oT> subfield(const uword in_row1, const uword in_col1, const SizeMat& s);
  inline const subview_field<oT> subfield(const uword in_row1, const uword in_col1, const SizeMat& s) const;

  inline       subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s);
  inline const subview_field<oT> subfield(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s) const;

  inline       subview_field<oT> subfield(const span& row_span, const span& col_span);
  inline const subview_field<oT> subfield(const span& row_span, const span& col_span) const;

  inline       subview_field<oT> subfield(const span& row_span, const span& col_span, const span& slice_span);
  inline const subview_field<oT> subfield(const span& row_span, const span& col_span, const span& slice_span) const;

  inline       subview_field<oT> operator()(const span& row_span, const span& col_span);
  inline const subview_field<oT> operator()(const span& row_span, const span& col_span) const;

  inline       subview_field<oT> operator()(const span& row_span, const span& col_span, const span& slice_span);
  inline const subview_field<oT> operator()(const span& row_span, const span& col_span, const span& slice_span) const;

  inline       subview_field<oT> operator()(const uword in_row1, const uword in_col1, const SizeMat& s);
  inline const subview_field<oT> operator()(const uword in_row1, const uword in_col1, const SizeMat& s) const;

  inline       subview_field<oT> operator()(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s);
  inline const subview_field<oT> operator()(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s) const;


  inline void print(                           const std::string extra_text = "") const;
  inline void print(std::ostream& user_stream, const std::string extra_text = "") const;

  #if defined(ARMA_USE_CXX11)
  inline const field& for_each(const std::function< void(      oT&) >& F);
  inline const field& for_each(const std::function< void(const oT&) >& F) const;
  #else
  template<typename functor> inline const field& for_each(functor F);
  template<typename functor> inline const field& for_each(functor F) const;
  #endif

  inline const field& fill(const oT& x);

  inline void reset();
  inline void reset_objects();

  arma_inline bool is_empty() const;


  arma_inline arma_warn_unused bool in_range(const uword i) const;
  arma_inline arma_warn_unused bool in_range(const span& x) const;

  arma_inline arma_warn_unused bool in_range(const uword   in_row, const uword   in_col) const;
  arma_inline arma_warn_unused bool in_range(const span& row_span, const uword   in_col) const;
  arma_inline arma_warn_unused bool in_range(const uword   in_row, const span& col_span) const;
  arma_inline arma_warn_unused bool in_range(const span& row_span, const span& col_span) const;

  arma_inline arma_warn_unused bool in_range(const uword   in_row, const uword   in_col, const SizeMat& s) const;

  arma_inline arma_warn_unused bool in_range(const uword   in_row, const uword   in_col, const uword   in_slice) const;
  arma_inline arma_warn_unused bool in_range(const span& row_span, const span& col_span, const span& slice_span) const;

  arma_inline arma_warn_unused bool in_range(const uword   in_row, const uword in_col, const uword in_slice, const SizeCube& s) const;


  inline bool save(const std::string   name, const file_type type = arma_binary, const bool print_status = true) const;
  inline bool save(      std::ostream& os,   const file_type type = arma_binary, const bool print_status = true) const;

  inline bool load(const std::string   name, const file_type type = auto_detect, const bool print_status = true);
  inline bool load(      std::istream& is,   const file_type type = auto_detect, const bool print_status = true);


  inline bool quiet_save(const std::string   name, const file_type type = arma_binary) const;
  inline bool quiet_save(      std::ostream& os,   const file_type type = arma_binary) const;

  inline bool quiet_load(const std::string   name, const file_type type = auto_detect);
  inline bool quiet_load(      std::istream& is,   const file_type type = auto_detect);


  // for container-like functionality

  typedef oT    value_type;
  typedef uword size_type;


  class iterator
    {
    public:

    inline iterator(field<oT>& in_M, const bool at_end = false);

    inline oT& operator* ();

    inline iterator& operator++();
    inline void      operator++(int);

    inline iterator& operator--();
    inline void      operator--(int);

    inline bool operator!=(const iterator& X) const;
    inline bool operator==(const iterator& X) const;

    arma_aligned field<oT>& M;
    arma_aligned uword      i;
    };


  class const_iterator
    {
    public:

    const_iterator(const field<oT>& in_M, const bool at_end = false);
    const_iterator(const iterator& X);

    inline const oT& operator*() const;

    inline const_iterator& operator++();
    inline void            operator++(int);

    inline const_iterator& operator--();
    inline void            operator--(int);

    inline bool operator!=(const const_iterator& X) const;
    inline bool operator==(const const_iterator& X) const;

    arma_aligned const field<oT>& M;
    arma_aligned       uword      i;
    };

  inline       iterator  begin();
  inline const_iterator  begin() const;
  inline const_iterator cbegin() const;

  inline       iterator  end();
  inline const_iterator  end() const;
  inline const_iterator cend() const;

  inline void  clear();
  inline bool  empty() const;
  inline uword size()  const;


  private:

  inline void init(const field<oT>& x);
  inline void init(const uword n_rows_in, const uword n_cols_in);
  inline void init(const uword n_rows_in, const uword n_cols_in, const uword n_slices_in);

  inline void delete_objects();
  inline void create_objects();

  friend class field_aux;
  friend class subview_field<oT>;


  public:

  #ifdef ARMA_EXTRA_FIELD_PROTO
    #include ARMA_INCFILE_WRAP(ARMA_EXTRA_FIELD_PROTO)
  #endif
  };



class field_aux
  {
  public:

  template<typename oT> inline static void reset_objects(field< oT >& x);
  template<typename eT> inline static void reset_objects(field< Mat<eT> >& x);
  template<typename eT> inline static void reset_objects(field< Col<eT> >& x);
  template<typename eT> inline static void reset_objects(field< Row<eT> >& x);
  template<typename eT> inline static void reset_objects(field< Cube<eT> >& x);
                        inline static void reset_objects(field< std::string >& x);


  template<typename oT> inline static bool save(const field< oT >& x,       const std::string&  name, const file_type type, std::string& err_msg);
  template<typename oT> inline static bool save(const field< oT >& x,             std::ostream& os,   const file_type type, std::string& err_msg);
  template<typename oT> inline static bool load(      field< oT >& x,       const std::string&  name, const file_type type, std::string& err_msg);
  template<typename oT> inline static bool load(      field< oT >& x,             std::istream& is,   const file_type type, std::string& err_msg);

  template<typename eT> inline static bool save(const field< Mat<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool save(const field< Mat<eT> >& x,        std::ostream& os,   const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Mat<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Mat<eT> >& x,        std::istream& is,   const file_type type, std::string& err_msg);

  template<typename eT> inline static bool save(const field< Col<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool save(const field< Col<eT> >& x,        std::ostream& os,   const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Col<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Col<eT> >& x,        std::istream& is,   const file_type type, std::string& err_msg);

  template<typename eT> inline static bool save(const field< Row<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool save(const field< Row<eT> >& x,        std::ostream& os,   const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Row<eT> >& x,  const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Row<eT> >& x,        std::istream& is,   const file_type type, std::string& err_msg);

  template<typename eT> inline static bool save(const field< Cube<eT> >& x, const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool save(const field< Cube<eT> >& x,       std::ostream& os,   const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Cube<eT> >& x, const std::string&  name, const file_type type, std::string& err_msg);
  template<typename eT> inline static bool load(      field< Cube<eT> >& x,       std::istream& is,   const file_type type, std::string& err_msg);

  inline static bool save(const field< std::string >& x, const std::string&  name, const file_type type, std::string& err_msg);
  inline static bool save(const field< std::string >& x,       std::ostream& os,   const file_type type, std::string& err_msg);
  inline static bool load(      field< std::string >& x, const std::string&  name, const file_type type, std::string& err_msg);
  inline static bool load(      field< std::string >& x,       std::istream& is,   const file_type type, std::string& err_msg);

  };


//! @}
