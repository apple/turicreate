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


//! \addtogroup SpMat
//! @{


//! Sparse matrix class, with data stored in compressed sparse column (CSC) format
template<typename eT>
class SpMat : public SpBase< eT, SpMat<eT> >
  {
  public:

  typedef eT                                elem_type;  //!< the type of elements stored in the matrix
  typedef typename get_pod_type<eT>::result  pod_type;  //!< if eT is std::complex<T>, pod_type is T; otherwise pod_type is eT

  static const bool is_row = false;
  static const bool is_col = false;

  const uword n_rows;    //!< number of rows             (read-only)
  const uword n_cols;    //!< number of columns          (read-only)
  const uword n_elem;    //!< number of elements         (read-only)
  const uword n_nonzero; //!< number of nonzero elements (read-only)
  const uword vec_state; //!< 0: matrix; 1: column vector; 2: row vector


  // The memory used to store the values of the matrix.
  // In accordance with the CSC format, this stores only the actual values.
  // The correct locations of the values are assembled from the row indices and column pointers.
  //
  // The length of this array is (n_nonzero + 1).
  // The final value values[n_nonzero] must be zero to ensure integrity of iterators.
  // Use mem_resize(new_n_nonzero) to resize this array.
  //
  // WARNING: the 'values' array is only valid after sync() is called;
  // WARNING: there is a separate cache for fast element insertion

  arma_aligned const eT* const values;


  // The row indices of each value.  row_indices[i] is the row of values[i].
  //
  // The length of this array is (n_nonzero + 1).
  // The final value row_indices[n_nonzero] must be zero to ensure integrity of iterators.
  // Use mem_resize(new_n_nonzero) to resize this array.
  //
  // WARNING: the 'row_indices' array is only valid after sync() is called;
  // WARNING: there is a separate cache for fast element insertion

  arma_aligned const uword* const row_indices;


  // The column pointers.  This stores the index of the first item in column i.
  // That is, values[col_ptrs[i]] is the first value in column i,
  // and it is in the row indicated by row_indices[col_ptrs[i]].
  //
  // The length of this array is (n_cols + 2).
  // The element col_ptrs[n_cols] must be equal to n_nonzero.
  // The element col_ptrs[n_cols + 1] must be an invalid very large value to ensure integrity of iterators.
  //
  // The col_ptrs array is set by the init() function
  // (which is called by constructors, set_size() and other functions that change the matrix size).
  //
  // WARNING: the 'col_ptrs' array is only valid after sync() is called;
  // WARNING: there is a separate cache for fast element insertion

  arma_aligned const uword* const col_ptrs;

  inline  SpMat();
  inline ~SpMat();

  inline explicit SpMat(const uword in_rows, const uword in_cols);
  inline explicit SpMat(const SizeMat& s);

  inline            SpMat(const char*        text);
  inline SpMat& operator=(const char*        text);
  inline            SpMat(const std::string& text);
  inline SpMat& operator=(const std::string& text);
  inline            SpMat(const SpMat<eT>&   x);

  #if defined(ARMA_USE_CXX11)
  inline            SpMat(SpMat&& m);
  inline SpMat& operator=(SpMat&& m);
  #endif

  inline explicit    SpMat(const MapMat<eT>& x);
  inline SpMat&  operator=(const MapMat<eT>& x);

  template<typename T1, typename T2, typename T3>
  inline SpMat(const Base<uword,T1>& rowind, const Base<uword,T2>& colptr, const Base<eT,T3>& values, const uword n_rows, const uword n_cols);

  template<typename T1, typename T2>
  inline SpMat(const Base<uword,T1>& locations, const Base<eT,T2>& values, const bool sort_locations = true);

  template<typename T1, typename T2>
  inline SpMat(const Base<uword,T1>& locations, const Base<eT,T2>& values, const uword n_rows, const uword n_cols, const bool sort_locations = true, const bool check_for_zeros = true);

  template<typename T1, typename T2>
  inline SpMat(const bool add_values, const Base<uword,T1>& locations, const Base<eT,T2>& values, const uword n_rows, const uword n_cols, const bool sort_locations = true, const bool check_for_zeros = true);

  inline SpMat&  operator=(const eT val); //! sets size to 1x1
  inline SpMat& operator*=(const eT val);
  inline SpMat& operator/=(const eT val);
  // operator+=(val) and operator-=(val) are not defined as they don't make sense for sparse matrices

  inline SpMat&  operator=(const SpMat& m);
  inline SpMat& operator+=(const SpMat& m);
  inline SpMat& operator-=(const SpMat& m);
  inline SpMat& operator*=(const SpMat& m);
  inline SpMat& operator%=(const SpMat& m);
  inline SpMat& operator/=(const SpMat& m);

  template<typename T1> inline explicit    SpMat(const Base<eT, T1>& m);
  template<typename T1> inline SpMat&  operator=(const Base<eT, T1>& m);
  template<typename T1> inline SpMat& operator+=(const Base<eT, T1>& m);
  template<typename T1> inline SpMat& operator-=(const Base<eT, T1>& m);
  template<typename T1> inline SpMat& operator*=(const Base<eT, T1>& m);
  template<typename T1> inline SpMat& operator/=(const Base<eT, T1>& m);
  template<typename T1> inline SpMat& operator%=(const Base<eT, T1>& m);


  //! construction of complex matrix out of two non-complex matrices
  template<typename T1, typename T2>
  inline explicit SpMat(const SpBase<pod_type, T1>& A, const SpBase<pod_type, T2>& B);

  inline             SpMat(const SpSubview<eT>& X);
  inline SpMat&  operator=(const SpSubview<eT>& X);
  inline SpMat& operator+=(const SpSubview<eT>& X);
  inline SpMat& operator-=(const SpSubview<eT>& X);
  inline SpMat& operator*=(const SpSubview<eT>& X);
  inline SpMat& operator%=(const SpSubview<eT>& X);
  inline SpMat& operator/=(const SpSubview<eT>& X);

  inline             SpMat(const spdiagview<eT>& X);
  inline SpMat&  operator=(const spdiagview<eT>& X);
  inline SpMat& operator+=(const spdiagview<eT>& X);
  inline SpMat& operator-=(const spdiagview<eT>& X);
  inline SpMat& operator*=(const spdiagview<eT>& X);
  inline SpMat& operator%=(const spdiagview<eT>& X);
  inline SpMat& operator/=(const spdiagview<eT>& X);

  // delayed unary ops
  template<typename T1, typename spop_type> inline             SpMat(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat&  operator=(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator+=(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator-=(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator*=(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator%=(const SpOp<T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator/=(const SpOp<T1, spop_type>& X);

  // delayed binary ops
  template<typename T1, typename T2, typename spglue_type> inline             SpMat(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat&  operator=(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat& operator+=(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat& operator-=(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat& operator*=(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat& operator%=(const SpGlue<T1, T2, spglue_type>& X);
  template<typename T1, typename T2, typename spglue_type> inline SpMat& operator/=(const SpGlue<T1, T2, spglue_type>& X);

  // delayed mixed-type unary ops
  template<typename T1, typename spop_type> inline             SpMat(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat&  operator=(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator+=(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator-=(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator*=(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator%=(const mtSpOp<eT, T1, spop_type>& X);
  template<typename T1, typename spop_type> inline SpMat& operator/=(const mtSpOp<eT, T1, spop_type>& X);


  arma_inline       SpSubview<eT> row(const uword row_num);
  arma_inline const SpSubview<eT> row(const uword row_num) const;

  inline            SpSubview<eT> operator()(const uword row_num, const span& col_span);
  inline      const SpSubview<eT> operator()(const uword row_num, const span& col_span) const;

  arma_inline       SpSubview<eT> col(const uword col_num);
  arma_inline const SpSubview<eT> col(const uword col_num) const;

  inline            SpSubview<eT> operator()(const span& row_span, const uword col_num);
  inline      const SpSubview<eT> operator()(const span& row_span, const uword col_num) const;

  arma_inline       SpSubview<eT> rows(const uword in_row1, const uword in_row2);
  arma_inline const SpSubview<eT> rows(const uword in_row1, const uword in_row2) const;

  arma_inline       SpSubview<eT> cols(const uword in_col1, const uword in_col2);
  arma_inline const SpSubview<eT> cols(const uword in_col1, const uword in_col2) const;

  arma_inline       SpSubview<eT> submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2);
  arma_inline const SpSubview<eT> submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const;

  arma_inline       SpSubview<eT> submat(const uword in_row1, const uword in_col1, const SizeMat& s);
  arma_inline const SpSubview<eT> submat(const uword in_row1, const uword in_col1, const SizeMat& s) const;

  inline            SpSubview<eT> submat    (const span& row_span, const span& col_span);
  inline      const SpSubview<eT> submat    (const span& row_span, const span& col_span) const;

  inline            SpSubview<eT> operator()(const span& row_span, const span& col_span);
  inline      const SpSubview<eT> operator()(const span& row_span, const span& col_span) const;

  arma_inline       SpSubview<eT> operator()(const uword in_row1, const uword in_col1, const SizeMat& s);
  arma_inline const SpSubview<eT> operator()(const uword in_row1, const uword in_col1, const SizeMat& s) const;


  inline       SpSubview<eT> head_rows(const uword N);
  inline const SpSubview<eT> head_rows(const uword N) const;

  inline       SpSubview<eT> tail_rows(const uword N);
  inline const SpSubview<eT> tail_rows(const uword N) const;

  inline       SpSubview<eT> head_cols(const uword N);
  inline const SpSubview<eT> head_cols(const uword N) const;

  inline       SpSubview<eT> tail_cols(const uword N);
  inline const SpSubview<eT> tail_cols(const uword N) const;


  inline       spdiagview<eT> diag(const sword in_id = 0);
  inline const spdiagview<eT> diag(const sword in_id = 0) const;


  inline void swap_rows(const uword in_row1, const uword in_row2);
  inline void swap_cols(const uword in_col1, const uword in_col2);

  inline void shed_row(const uword row_num);
  inline void shed_col(const uword col_num);

  inline void shed_rows(const uword in_row1, const uword in_row2);
  inline void shed_cols(const uword in_col1, const uword in_col2);


  // access the i-th element; if there is nothing at element i, 0 is returned
  arma_inline arma_warn_unused MapMat_elem<eT> operator[] (const uword i);
  arma_inline arma_warn_unused eT              operator[] (const uword i) const;
  arma_inline arma_warn_unused MapMat_elem<eT> at         (const uword i);
  arma_inline arma_warn_unused eT              at         (const uword i) const;
  arma_inline arma_warn_unused MapMat_elem<eT> operator() (const uword i);
  arma_inline arma_warn_unused eT              operator() (const uword i) const;

  // access the element at the given row and column; if there is nothing at that position, 0 is returned
  arma_inline arma_warn_unused MapMat_elem<eT> at         (const uword in_row, const uword in_col);
  arma_inline arma_warn_unused eT              at         (const uword in_row, const uword in_col) const;
  arma_inline arma_warn_unused MapMat_elem<eT> operator() (const uword in_row, const uword in_col);
  arma_inline arma_warn_unused eT              operator() (const uword in_row, const uword in_col) const;


  arma_inline arma_warn_unused bool is_empty()  const;
  arma_inline arma_warn_unused bool is_vec()    const;
  arma_inline arma_warn_unused bool is_rowvec() const;
  arma_inline arma_warn_unused bool is_colvec() const;
  arma_inline arma_warn_unused bool is_square() const;
       inline arma_warn_unused bool is_finite() const;

  inline arma_warn_unused bool has_inf() const;
  inline arma_warn_unused bool has_nan() const;

  arma_inline arma_warn_unused bool in_range(const uword i) const;
  arma_inline arma_warn_unused bool in_range(const span& x) const;

  arma_inline arma_warn_unused bool in_range(const uword   in_row, const uword   in_col) const;
  arma_inline arma_warn_unused bool in_range(const span& row_span, const uword   in_col) const;
  arma_inline arma_warn_unused bool in_range(const uword   in_row, const span& col_span) const;
  arma_inline arma_warn_unused bool in_range(const span& row_span, const span& col_span) const;

  arma_inline arma_warn_unused bool in_range(const uword in_row, const uword in_col, const SizeMat& s) const;


  inline void impl_print(                           const std::string& extra_text) const;
  inline void impl_print(std::ostream& user_stream, const std::string& extra_text) const;

  inline void impl_raw_print(                           const std::string& extra_text) const;
  inline void impl_raw_print(std::ostream& user_stream, const std::string& extra_text) const;

  inline void impl_print_dense(                           const std::string& extra_text) const;
  inline void impl_print_dense(std::ostream& user_stream, const std::string& extra_text) const;

  inline void impl_raw_print_dense(                           const std::string& extra_text) const;
  inline void impl_raw_print_dense(std::ostream& user_stream, const std::string& extra_text) const;


  template<typename eT2> inline void copy_size(const SpMat<eT2>& m);
  template<typename eT2> inline void copy_size(const   Mat<eT2>& m);

  inline void set_size(const uword in_elem);
  inline void set_size(const uword in_rows, const uword in_cols);
  inline void set_size(const SizeMat& s);

  inline void   resize(const uword in_rows, const uword in_cols);
  inline void   resize(const SizeMat& s);

  inline void  reshape(const uword in_rows, const uword in_cols);
  inline void  reshape(const SizeMat& s);
  inline void  reshape(const uword in_rows, const uword in_cols, const uword dim);  // this form is deprecated: don't use it

  inline const SpMat& replace(const eT old_val, const eT new_val);

  inline const SpMat& zeros();
  inline const SpMat& zeros(const uword in_elem);
  inline const SpMat& zeros(const uword in_rows, const uword in_cols);
  inline const SpMat& zeros(const SizeMat& s);

  inline const SpMat& eye();
  inline const SpMat& eye(const uword in_rows, const uword in_cols);
  inline const SpMat& eye(const SizeMat& s);

  inline const SpMat& speye();
  inline const SpMat& speye(const uword in_rows, const uword in_cols);
  inline const SpMat& speye(const SizeMat& s);

  inline const SpMat& sprandu(const uword in_rows, const uword in_cols, const double density);
  inline const SpMat& sprandu(const SizeMat& s,                         const double density);

  inline const SpMat& sprandn(const uword in_rows, const uword in_cols, const double density);
  inline const SpMat& sprandn(const SizeMat& s,                         const double density);

  inline void reset();


  template<typename T1> inline void set_real(const SpBase<pod_type,T1>& X);
  template<typename T1> inline void set_imag(const SpBase<pod_type,T1>& X);


  // saving and loading
  // TODO: implement auto_detect for sparse matrices

  inline bool save(const std::string   name, const file_type type = arma_binary, const bool print_status = true) const;
  inline bool save(      std::ostream& os,   const file_type type = arma_binary, const bool print_status = true) const;

  inline bool load(const std::string   name, const file_type type = arma_binary, const bool print_status = true);
  inline bool load(      std::istream& is,   const file_type type = arma_binary, const bool print_status = true);

  inline bool quiet_save(const std::string   name, const file_type type = arma_binary) const;
  inline bool quiet_save(      std::ostream& os,   const file_type type = arma_binary) const;

  inline bool quiet_load(const std::string   name, const file_type type = arma_binary);
  inline bool quiet_load(      std::istream& is,   const file_type type = arma_binary);



  // necessary forward declarations
  class iterator_base;
  class iterator;
  class const_iterator;
  class row_iterator;
  class const_row_iterator;

  // iterator_base provides basic operators but not how to compare or how to iterate
  class iterator_base
    {
    public:

    inline iterator_base();
    inline iterator_base(const SpMat& in_M);
    inline iterator_base(const SpMat& in_M, const uword col, const uword pos);

    arma_inline eT operator*() const;

    // don't hold location internally; call "dummy" methods to get that information
    arma_inline uword row() const { return M->row_indices[internal_pos]; }
    arma_inline uword col() const { return internal_col;                 }
    arma_inline uword pos() const { return internal_pos;                 }

    arma_aligned const SpMat* M;
    arma_aligned       uword  internal_col;
    arma_aligned       uword  internal_pos;

    // so that we satisfy the STL iterator types
    typedef std::bidirectional_iterator_tag iterator_category;
    typedef eT                              value_type;
    typedef uword                           difference_type; // not certain on this one
    typedef const eT*                       pointer;
    typedef const eT&                       reference;
    };

  class const_iterator : public iterator_base
    {
    public:

    inline const_iterator();
    inline const_iterator(const SpMat& in_M, uword initial_pos = 0); // assumes initial_pos is valid
    //! once initialised, will be at the first nonzero value after the given position (using forward columnwise traversal)
    inline const_iterator(const SpMat& in_M, uword in_row, uword in_col);
    //! if you know the exact position of the iterator;  in_row is a dummy argument
    inline const_iterator(const SpMat& in_M, uword in_row, uword in_col, uword in_pos);
    inline const_iterator(const const_iterator& other);

    inline arma_hot const_iterator& operator++();
    inline arma_hot const_iterator  operator++(int);

    inline arma_hot const_iterator& operator--();
    inline arma_hot const_iterator  operator--(int);

    inline arma_hot bool operator==(const const_iterator& rhs) const;
    inline arma_hot bool operator!=(const const_iterator& rhs) const;

    inline arma_hot bool operator==(const typename SpSubview<eT>::const_iterator& rhs) const;
    inline arma_hot bool operator!=(const typename SpSubview<eT>::const_iterator& rhs) const;

    inline arma_hot bool operator==(const const_row_iterator& rhs) const;
    inline arma_hot bool operator!=(const const_row_iterator& rhs) const;

    inline arma_hot bool operator==(const typename SpSubview<eT>::const_row_iterator& rhs) const;
    inline arma_hot bool operator!=(const typename SpSubview<eT>::const_row_iterator& rhs) const;
    };

  /**
   * So that we can iterate over nonzero values, we need an iterator implementation.
   * This can't be as simple as for Mat, which is just a pointer to an eT.
   * If a value is set to 0 using this iterator, the iterator is no longer valid!
   */
  class iterator : public const_iterator
    {
    public:

    inline iterator() : const_iterator() { }
    inline iterator(SpMat& in_M, uword initial_pos = 0) : const_iterator(in_M, initial_pos) { }
    inline iterator(SpMat& in_M, uword in_row, uword in_col) : const_iterator(in_M, in_row, in_col) { }
    inline iterator(SpMat& in_M, uword in_row, uword in_col, uword in_pos) : const_iterator(in_M, in_row, in_col, in_pos) { }
    inline iterator(const iterator& other) : const_iterator(other) { }

    inline arma_hot SpValProxy<SpMat<eT> > operator*();

    // overloads needed for return type correctness
    inline arma_hot iterator& operator++();
    inline arma_hot iterator  operator++(int);

    inline arma_hot iterator& operator--();
    inline arma_hot iterator  operator--(int);

    // this has a different value_type than iterator_base
    typedef SpValProxy<SpMat<eT> >         value_type;
    typedef const SpValProxy<SpMat<eT> >*  pointer;
    typedef const SpValProxy<SpMat<eT> >&  reference;
    };

  class const_row_iterator : public iterator_base
    {
    public:

    inline const_row_iterator();
    inline const_row_iterator(const SpMat& in_M, uword initial_pos = 0);
    //! once initialised, will be at the first nonzero value after the given position (using forward row-wise traversal)
    inline const_row_iterator(const SpMat& in_M, uword in_row, uword in_col);
    inline const_row_iterator(const const_row_iterator& other);

    inline arma_hot const_row_iterator& operator++();
    inline arma_hot const_row_iterator  operator++(int);

    inline arma_hot const_row_iterator& operator--();
    inline arma_hot const_row_iterator  operator--(int);

    uword internal_row; // hold row internally because we use internal_pos differently
    uword actual_pos;   // actual position in matrix

    arma_inline eT operator*() const { return iterator_base::M->values[actual_pos]; }

    arma_inline uword row() const { return internal_row; }

    inline arma_hot bool operator==(const const_iterator& rhs) const;
    inline arma_hot bool operator!=(const const_iterator& rhs) const;

    inline arma_hot bool operator==(const typename SpSubview<eT>::const_iterator& rhs) const;
    inline arma_hot bool operator!=(const typename SpSubview<eT>::const_iterator& rhs) const;

    inline arma_hot bool operator==(const const_row_iterator& rhs) const;
    inline arma_hot bool operator!=(const const_row_iterator& rhs) const;

    inline arma_hot bool operator==(const typename SpSubview<eT>::const_row_iterator& rhs) const;
    inline arma_hot bool operator!=(const typename SpSubview<eT>::const_row_iterator& rhs) const;
    };

  class row_iterator : public const_row_iterator
    {
    public:

    inline row_iterator() : const_row_iterator() {}
    inline row_iterator(SpMat& in_M, uword initial_pos = 0) : const_row_iterator(in_M, initial_pos) { }
    //! once initialised, will be at the first nonzero value after the given position (using forward row-wise traversal)
    inline row_iterator(SpMat& in_M, uword in_row, uword in_col) : const_row_iterator(in_M, in_row, in_col) { }
    inline row_iterator(const row_iterator& other) : const_row_iterator(other) { }

    inline arma_hot SpValProxy<SpMat<eT> > operator*();

    // overloads required for return type correctness
    inline arma_hot row_iterator& operator++();
    inline arma_hot row_iterator  operator++(int);

    inline arma_hot row_iterator& operator--();
    inline arma_hot row_iterator  operator--(int);

    // this has a different value_type than iterator_base
    typedef SpValProxy<SpMat<eT> >         value_type;
    typedef const SpValProxy<SpMat<eT> >*  pointer;
    typedef const SpValProxy<SpMat<eT> >&  reference;
    };


  typedef       iterator       row_col_iterator;
  typedef const_iterator const_row_col_iterator;


  inline       iterator     begin();
  inline const_iterator     begin() const;

  inline       iterator     end();
  inline const_iterator     end() const;

  inline       iterator     begin_col(const uword col_num);
  inline const_iterator     begin_col(const uword col_num) const;

  inline       iterator     end_col(const uword col_num);
  inline const_iterator     end_col(const uword col_num) const;

  inline       row_iterator begin_row(const uword row_num = 0);
  inline const_row_iterator begin_row(const uword row_num = 0) const;

  inline       row_iterator end_row();
  inline const_row_iterator end_row() const;

  inline       row_iterator end_row(const uword row_num);
  inline const_row_iterator end_row(const uword row_num) const;

  inline       row_col_iterator begin_row_col();
  inline const_row_col_iterator begin_row_col() const;

  inline       row_col_iterator end_row_col();
  inline const_row_col_iterator end_row_col() const;


  inline void  clear();
  inline bool  empty() const;
  inline uword size()  const;

  // Resize memory.
  // If the new size is larger, the column pointers and new memory still need to be correctly set.
  // If the new size is smaller, the first new_n_nonzero elements will be copied.
  // n_nonzero is updated.
  inline void mem_resize(const uword new_n_nonzero);

  //! synchronise CSC from cache
  inline void sync() const;

  //! don't use this unless you're writing internal Armadillo code
  inline void remove_zeros();

  //! don't use this unless you're writing internal Armadillo code
  inline void steal_mem(SpMat& X);

  //! don't use this unless you're writing internal Armadillo code
  inline void steal_mem_simple(SpMat& X);

  //! don't use this unless you're writing internal Armadillo code
  template<              typename T1, typename Functor> arma_hot inline void init_xform   (const SpBase<eT, T1>& x, const Functor& func);
  template<typename eT2, typename T1, typename Functor> arma_hot inline void init_xform_mt(const SpBase<eT2,T1>& x, const Functor& func);


  protected:

  inline void init(uword in_rows, uword in_cols);
  inline void init(const std::string& text);
  inline void init(const  SpMat<eT>& x);
  inline void init(const MapMat<eT>& x);


  inline void init_batch_std(const Mat<uword>& locations, const Mat<eT>& values, const bool sort_locations);
  inline void init_batch_add(const Mat<uword>& locations, const Mat<eT>& values, const bool sort_locations);

  inline SpMat(const arma_vec_indicator&, const uword in_vec_state);
  inline SpMat(const arma_vec_indicator&, const uword in_n_rows, const uword in_n_cols, const uword in_vec_state);


  private:

  inline arma_hot arma_warn_unused SpValProxy<SpMat<eT> > get_value(const uword i);
  inline arma_hot arma_warn_unused eT                     get_value(const uword i) const;

  inline arma_hot arma_warn_unused SpValProxy<SpMat<eT> > get_value(const uword in_row, const uword in_col);
  inline arma_hot arma_warn_unused eT                     get_value(const uword in_row, const uword in_col) const;


  arma_inline arma_hot arma_warn_unused uword get_position(const uword i) const;
  arma_inline arma_hot                  void  get_position(const uword i, uword& row_of_i, uword& col_of_i) const;

  /**
   * Add an element at the given position, and return a reference to it.
   * The element will be set to 0 (unless otherwise specified).
   * If the element already exists, its value will be overwritten.
   */
  inline arma_hot arma_warn_unused eT& add_element(const uword in_row, const uword in_col, const eT in_val = eT(0));

  inline arma_hot void delete_element(const uword in_row, const uword in_col);


  // cache related

  arma_aligned mutable MapMat<eT> cache;
  arma_aligned mutable uword      sync_state;
  // 0: cache needs to be updated from CSC
  // 1: CSC needs to be updated from cache
  // 2: no update required

  arma_inline void invalidate_cache() const;
  arma_inline void invalidate_csc()   const;

  arma_inline void sync_cache() const;
  arma_inline void sync_csc()   const;


  friend class SpValProxy< SpMat<eT> >;  // so that SpValProxy can call add_element() and delete_element()
  friend class SpSubview<eT>;
  friend class SpRow<eT>;
  friend class SpCol<eT>;


  public:

  #ifdef ARMA_EXTRA_SPMAT_PROTO
    #include ARMA_INCFILE_WRAP(ARMA_EXTRA_SPMAT_PROTO)
  #endif
  };



class SpMat_aux
  {
  public:

  template<typename eT, typename T1> inline static void set_real(SpMat<eT>&                out, const SpBase<eT,T1>& X);
  template<typename T,  typename T1> inline static void set_real(SpMat< std::complex<T> >& out, const SpBase< T,T1>& X);

  template<typename eT, typename T1> inline static void set_imag(SpMat<eT>&                out, const SpBase<eT,T1>& X);
  template<typename T,  typename T1> inline static void set_imag(SpMat< std::complex<T> >& out, const SpBase< T,T1>& X);
  };



#define ARMA_HAS_SPMAT



//! @}
