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


//! \addtogroup Mat
//! @{


template<typename eT>
inline
Mat<eT>::~Mat()
  {
  arma_extra_debug_sigprint_this(this);

  if( (mem_state == 0) && (n_elem > arma_config::mat_prealloc) )
    {
    memory::release( access::rw(mem) );
    }

  // try to expose buggy user code that accesses deleted objects
  if(arma_config::debug)  { access::rw(mem) = 0; }

  arma_type_check(( is_supported_elem_type<eT>::value == false ));
  }



template<typename eT>
inline
Mat<eT>::Mat()
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);
  }



//! construct the matrix to have user specified dimensions
template<typename eT>
inline
Mat<eT>::Mat(const uword in_n_rows, const uword in_n_cols)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows*in_n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();
  }



template<typename eT>
inline
Mat<eT>::Mat(const SizeMat& s)
  : n_rows(s.n_rows)
  , n_cols(s.n_cols)
  , n_elem(s.n_rows*s.n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();
  }



//! construct the matrix to have user specified dimensions and fill with specified pattern
template<typename eT>
template<typename fill_type>
inline
Mat<eT>::Mat(const uword in_n_rows, const uword in_n_cols, const fill::fill_class<fill_type>& f)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows*in_n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  (*this).fill(f);
  }



template<typename eT>
template<typename fill_type>
inline
Mat<eT>::Mat(const SizeMat& s, const fill::fill_class<fill_type>& f)
  : n_rows(s.n_rows)
  , n_cols(s.n_cols)
  , n_elem(s.n_rows*s.n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  (*this).fill(f);
  }



//! constructor used by Row and Col classes
template<typename eT>
inline
Mat<eT>::Mat(const arma_vec_indicator&, const uhword in_vec_state)
  : n_rows( (in_vec_state == 2) ? 1 : 0 )
  , n_cols( (in_vec_state == 1) ? 1 : 0 )
  , n_elem(0)
  , vec_state(in_vec_state)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);
  }



//! constructor used by Row and Col classes
template<typename eT>
inline
Mat<eT>::Mat(const arma_vec_indicator&, const uword in_n_rows, const uword in_n_cols, const uhword in_vec_state)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows*in_n_cols)
  , vec_state(in_vec_state)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();
  }



template<typename eT>
inline
Mat<eT>::Mat(const arma_fixed_indicator&, const uword in_n_rows, const uword in_n_cols, const uhword in_vec_state, const eT* in_mem)
  : n_rows    (in_n_rows)
  , n_cols    (in_n_cols)
  , n_elem    (in_n_rows*in_n_cols)
  , vec_state (in_vec_state)
  , mem_state (3)
  , mem       (in_mem)
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename eT>
inline
void
Mat<eT>::init_cold()
  {
  arma_extra_debug_sigprint( arma_str::format("n_rows = %d, n_cols = %d") % n_rows % n_cols );

  // ensure that n_elem can hold the result of (n_rows * n_cols)

  #if (defined(ARMA_USE_CXX11) || defined(ARMA_64BIT_WORD))
    const char* error_message = "Mat::init(): requested size is too large";
  #else
    const char* error_message = "Mat::init(): requested size is too large; suggest to compile in C++11 mode or enable ARMA_64BIT_WORD";
  #endif

  arma_debug_check
    (
      (
      ( (n_rows > ARMA_MAX_UHWORD) || (n_cols > ARMA_MAX_UHWORD) )
        ? ( (double(n_rows) * double(n_cols)) > double(ARMA_MAX_UWORD) )
        : false
      ),
    error_message
    );

  if(n_elem <= arma_config::mat_prealloc)
    {
    if(n_elem == 0)
      {
      access::rw(mem) = NULL;
      }
    else
      {
      arma_extra_debug_print("Mat::init(): using local memory");
      access::rw(mem) = mem_local;
      }
    }
  else
    {
    arma_extra_debug_print("Mat::init(): acquiring memory");
    access::rw(mem) = memory::acquire<eT>(n_elem);
    }
  }



template<typename eT>
inline
void
Mat<eT>::init_warm(uword in_n_rows, uword in_n_cols)
  {
  arma_extra_debug_sigprint( arma_str::format("in_n_rows = %d, in_n_cols = %d") % in_n_rows % in_n_cols );

  if( (n_rows == in_n_rows) && (n_cols == in_n_cols) )  { return; }

  bool  err_state = false;
  char* err_msg   = 0;

  const uhword t_vec_state = vec_state;
  const uhword t_mem_state = mem_state;

  arma_debug_set_error( err_state, err_msg, (t_mem_state == 3), "Mat::init(): size is fixed and hence cannot be changed" );

  if(t_vec_state > 0)
    {
    if( (in_n_rows == 0) && (in_n_cols == 0) )
      {
      if(t_vec_state == 1)  { in_n_cols = 1; }
      if(t_vec_state == 2)  { in_n_rows = 1; }
      }
    else
      {
      if(t_vec_state == 1)  { arma_debug_set_error( err_state, err_msg, (in_n_cols != 1), "Mat::init(): requested size is not compatible with column vector layout" ); }
      if(t_vec_state == 2)  { arma_debug_set_error( err_state, err_msg, (in_n_rows != 1), "Mat::init(): requested size is not compatible with row vector layout"    ); }
      }
    }

  // ensure that n_elem can hold the result of (n_rows * n_cols)

  #if (defined(ARMA_USE_CXX11) || defined(ARMA_64BIT_WORD))
    const char* error_message = "Mat::init(): requested size is too large";
  #else
    const char* error_message = "Mat::init(): requested size is too large; suggest to compile in C++11 mode or enable ARMA_64BIT_WORD";
  #endif

  arma_debug_set_error
    (
    err_state,
    err_msg,
      (
      ( (in_n_rows > ARMA_MAX_UHWORD) || (in_n_cols > ARMA_MAX_UHWORD) )
        ? ( (double(in_n_rows) * double(in_n_cols)) > double(ARMA_MAX_UWORD) )
        : false
      ),
    error_message
    );

  arma_debug_check(err_state, err_msg);

  const uword old_n_elem = n_elem;
  const uword new_n_elem = in_n_rows * in_n_cols;

  if(old_n_elem == new_n_elem)
    {
    arma_extra_debug_print("Mat::init(): reusing memory");
    access::rw(n_rows) = in_n_rows;
    access::rw(n_cols) = in_n_cols;
    }
  else  // condition: old_n_elem != new_n_elem
    {
    arma_debug_check( (t_mem_state == 2), "Mat::init(): mismatch between size of auxiliary memory and requested size" );

    if(new_n_elem < old_n_elem)  // reuse existing memory if possible
      {
      if( (t_mem_state == 0) && (new_n_elem <= arma_config::mat_prealloc) )
        {
        if(old_n_elem > arma_config::mat_prealloc)
          {
          arma_extra_debug_print("Mat::init(): releasing memory");
          memory::release( access::rw(mem) );
          }

        if(new_n_elem == 0)
          {
          access::rw(mem) = NULL;
          }
        else
          {
          arma_extra_debug_print("Mat::init(): using local memory");
          access::rw(mem) = mem_local;
          }
        }
      else
        {
        arma_extra_debug_print("Mat::init(): reusing memory");
        }
      }
    else  // condition: new_n_elem > old_n_elem
      {
      if( (t_mem_state == 0) && (old_n_elem > arma_config::mat_prealloc) )
        {
        arma_extra_debug_print("Mat::init(): releasing memory");
        memory::release( access::rw(mem) );
        }

      if(new_n_elem <= arma_config::mat_prealloc)
        {
        arma_extra_debug_print("Mat::init(): using local memory");
        access::rw(mem) = mem_local;
        }
      else
        {
        arma_extra_debug_print("Mat::init(): acquiring memory");
        access::rw(mem) = memory::acquire<eT>(new_n_elem);
        }

      access::rw(mem_state) = 0;
      }

    access::rw(n_rows) = in_n_rows;
    access::rw(n_cols) = in_n_cols;
    access::rw(n_elem) = new_n_elem;
    }
  }



//! create the matrix from a textual description
template<typename eT>
inline
Mat<eT>::Mat(const char* text)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init( std::string(text) );
  }



//! create the matrix from a textual description
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const char* text)
  {
  arma_extra_debug_sigprint();

  init( std::string(text) );
  return *this;
  }



//! create the matrix from a textual description
template<typename eT>
inline
Mat<eT>::Mat(const std::string& text)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init(text);
  }



//! create the matrix from a textual description
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const std::string& text)
  {
  arma_extra_debug_sigprint();

  init(text);
  return *this;
  }



//! internal function to create the matrix from a textual description
template<typename eT>
inline
void
Mat<eT>::init(const std::string& text_orig)
  {
  arma_extra_debug_sigprint();

  const bool replace_commas = (is_cx<eT>::yes) ? false : ( text_orig.find(',') != std::string::npos );

  std::string text_mod;

  if(replace_commas)  { text_mod = text_orig;  std::replace(text_mod.begin(), text_mod.end(), ',', ' '); }

  const std::string& text = (replace_commas) ? text_mod : text_orig;

  //
  // work out the size

  uword t_n_rows = 0;
  uword t_n_cols = 0;

  bool t_n_cols_found = false;

  std::string token;

  std::string::size_type line_start = 0;
  std::string::size_type   line_end = 0;

  std::stringstream line_stream;

  while( line_start < text.length() )
    {
    line_end = text.find(';', line_start);

    if(line_end == std::string::npos)
      {
      line_end = text.length()-1;
      }

    std::string::size_type line_len = line_end - line_start + 1;

    line_stream.clear();
    line_stream.str( text.substr(line_start,line_len) );

    uword line_n_cols = 0;
    while(line_stream >> token)
      {
      ++line_n_cols;
      }

    if(line_n_cols > 0)
      {
      if(t_n_cols_found == false)
        {
        t_n_cols       = line_n_cols;
        t_n_cols_found = true;
        }
      else
        {
        arma_check( (line_n_cols != t_n_cols), "Mat::init(): inconsistent number of columns in given string");
        }

      ++t_n_rows;
      }

    line_start = line_end+1;
    }


  Mat<eT>& x = *this;
  x.set_size(t_n_rows, t_n_cols);

  line_start = 0;
  line_end   = 0;

  uword urow = 0;

  while( line_start < text.length() )
    {
    line_end = text.find(';', line_start);

    if(line_end == std::string::npos)
      {
      line_end = text.length()-1;
      }

    std::string::size_type line_len = line_end - line_start + 1;

    line_stream.clear();
    line_stream.str( text.substr(line_start,line_len) );

//     uword ucol = 0;
//     while(line_stream >> token)
//       {
//       x.at(urow,ucol) = strtod(token.c_str(), 0);
//       ++ucol;
//       }

    uword ucol = 0;
    eT val;
    while(line_stream >> val)
      {
      x(urow,ucol) = val;
      ++ucol;
      }

    ++urow;
    line_start = line_end+1;
    }
  }



//! create the matrix from std::vector
template<typename eT>
inline
Mat<eT>::Mat(const std::vector<eT>& x)
  : n_rows(uword(x.size()))
  , n_cols(1)
  , n_elem(uword(x.size()))
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  if(n_elem > 0)
    {
    arrayops::copy( memptr(), &(x[0]), n_elem );
    }
  }



//! create the matrix from std::vector
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const std::vector<eT>& x)
  {
  arma_extra_debug_sigprint();

  init_warm(uword(x.size()), 1);

  if(x.size() > 0)
    {
    arrayops::copy( memptr(), &(x[0]), uword(x.size()) );
    }

  return *this;
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  inline
  Mat<eT>::Mat(const std::initializer_list<eT>& list)
    : n_rows(0)
    , n_cols(0)
    , n_elem(0)
    , vec_state(0)
    , mem_state(0)
    , mem()
    {
    arma_extra_debug_sigprint_this(this);

    init(list);
    }



  template<typename eT>
  inline
  Mat<eT>&
  Mat<eT>::operator=(const std::initializer_list<eT>& list)
    {
    arma_extra_debug_sigprint();

    init(list);

    return *this;
    }



  template<typename eT>
  inline
  Mat<eT>::Mat(const std::initializer_list< std::initializer_list<eT> >& list)
    : n_rows(0)
    , n_cols(0)
    , n_elem(0)
    , vec_state(0)
    , mem_state(0)
    , mem()
    {
    arma_extra_debug_sigprint_this(this);

    init(list);
    }



  template<typename eT>
  inline
  Mat<eT>&
  Mat<eT>::operator=(const std::initializer_list< std::initializer_list<eT> >& list)
    {
    arma_extra_debug_sigprint();

    init(list);

    return *this;
    }



  template<typename eT>
  inline
  Mat<eT>::Mat(Mat<eT>&& X)
    : n_rows   (X.n_rows)
    , n_cols   (X.n_cols)
    , n_elem   (X.n_elem)
    , vec_state(0       )
    , mem_state(0       )
    , mem      (        )
    {
    arma_extra_debug_sigprint(arma_str::format("this = %x   X = %x") % this % &X);

    if( ((X.mem_state == 0) && (X.n_elem > arma_config::mat_prealloc)) || (X.mem_state == 1) || (X.mem_state == 2) )
      {
      access::rw(mem_state) = X.mem_state;
      access::rw(mem)       = X.mem;

      access::rw(X.n_rows)    = 0;
      access::rw(X.n_cols)    = 0;
      access::rw(X.n_elem)    = 0;
      access::rw(X.mem_state) = 0;
      access::rw(X.mem)       = 0;
      }
    else
      {
      init_cold();

      arrayops::copy( memptr(), X.mem, X.n_elem );

      if( (X.mem_state == 0) && (X.n_elem <= arma_config::mat_prealloc) )
        {
        access::rw(X.n_rows) = 0;
        access::rw(X.n_cols) = 0;
        access::rw(X.n_elem) = 0;
        access::rw(X.mem)    = 0;
        }
      }
    }



  template<typename eT>
  inline
  Mat<eT>&
  Mat<eT>::operator=(Mat<eT>&& X)
    {
    arma_extra_debug_sigprint(arma_str::format("this = %x   X = %x") % this % &X);

    (*this).steal_mem(X);

    if( (X.mem_state == 0) && (X.n_elem <= arma_config::mat_prealloc) && (this != &X) )
      {
      access::rw(X.n_rows) = 0;
      access::rw(X.n_cols) = 0;
      access::rw(X.n_elem) = 0;
      access::rw(X.mem)    = 0;
      }

    return *this;
    }

#endif



//! Set the matrix to be equal to the specified scalar.
//! NOTE: the size of the matrix will be 1x1
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  init_warm(1,1);
  access::rw(mem[0]) = val;
  return *this;
  }



//! In-place addition of a scalar to all elements of the matrix
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_plus( memptr(), val, n_elem );

  return *this;
  }



//! In-place subtraction of a scalar from all elements of the matrix
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_minus( memptr(), val, n_elem );

  return *this;
  }



//! In-place multiplication of all elements of the matrix with a scalar
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_mul( memptr(), val, n_elem );

  return *this;
  }



//! In-place division of all elements of the matrix with a scalar
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_div( memptr(), val, n_elem );

  return *this;
  }



//! construct a matrix from a given matrix
template<typename eT>
inline
Mat<eT>::Mat(const Mat<eT>& in_mat)
  : n_rows(in_mat.n_rows)
  , n_cols(in_mat.n_cols)
  , n_elem(in_mat.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint(arma_str::format("this = %x   in_mat = %x") % this % &in_mat);

  init_cold();

  arrayops::copy( memptr(), in_mat.mem, in_mat.n_elem );
  }



//! construct a matrix from a given matrix
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const Mat<eT>& in_mat)
  {
  arma_extra_debug_sigprint(arma_str::format("this = %x   in_mat = %x") % this % &in_mat);

  if(this != &in_mat)
    {
    init_warm(in_mat.n_rows, in_mat.n_cols);

    arrayops::copy( memptr(), in_mat.mem, in_mat.n_elem );
    }

  return *this;
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  inline
  void
  Mat<eT>::init(const std::initializer_list<eT>& list)
    {
    arma_extra_debug_sigprint();

    const uword N = uword(list.size());

    set_size(1, N);

    arrayops::copy( memptr(), list.begin(), N );
    }



  template<typename eT>
  inline
  void
  Mat<eT>::init(const std::initializer_list< std::initializer_list<eT> >& list)
    {
    arma_extra_debug_sigprint();

    uword x_n_rows = uword(list.size());
    uword x_n_cols = 0;

    bool x_n_cols_found = false;

    auto it     = list.begin();
    auto it_end = list.end();

    for(; it != it_end; ++it)
      {
      if(x_n_cols_found == false)
        {
        x_n_cols       = uword((*it).size());
        x_n_cols_found = true;
        }
      else
        {
        arma_check( (uword((*it).size()) != x_n_cols), "Mat::init(): inconsistent number of columns in initialiser list" );
        }
      }

    Mat<eT>& t = (*this);

    if(t.mem_state == 3)
      {
      arma_debug_check( ((x_n_rows != t.n_rows) || (x_n_cols != t.n_cols)), "Mat::init(): size mismatch between fixed size matrix and initialiser list" );
      }
    else
      {
      t.set_size(x_n_rows, x_n_cols);
      }

    uword row_num = 0;

    auto row_it     = list.begin();
    auto row_it_end = list.end();

    for(; row_it != row_it_end; ++row_it)
      {
      uword col_num = 0;

      auto col_it     = (*row_it).begin();
      auto col_it_end = (*row_it).end();

      for(; col_it != col_it_end; ++col_it)
        {
        t.at(row_num, col_num) = (*col_it);
        ++col_num;
        }

      ++row_num;
      }
    }

#endif



//! for constructing a complex matrix out of two non-complex matrices
template<typename eT>
template<typename T1, typename T2>
inline
void
Mat<eT>::init
  (
  const Base<typename Mat<eT>::pod_type, T1>& X,
  const Base<typename Mat<eT>::pod_type, T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type T;

  arma_type_check(( is_complex<eT>::value == false ));   //!< compile-time abort if eT is not std::complex
  arma_type_check(( is_complex< T>::value == true  ));   //!< compile-time abort if  T is     std::complex

  arma_type_check(( is_same_type< std::complex<T>, eT >::no ));   //!< compile-time abort if types are not compatible

  const Proxy<T1> PX(X.get_ref());
  const Proxy<T2> PY(Y.get_ref());

  arma_debug_assert_same_size(PX, PY, "Mat()");

  const uword local_n_rows = PX.get_n_rows();
  const uword local_n_cols = PX.get_n_cols();

  init_warm(local_n_rows, local_n_cols);

  eT* out_mem = (*this).memptr();

  const bool use_at = ( Proxy<T1>::use_at || Proxy<T2>::use_at );

  if(use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type1;
    typedef typename Proxy<T2>::ea_type ea_type2;

    const uword N = n_elem;

    ea_type1 A = PX.get_ea();
    ea_type2 B = PY.get_ea();

    for(uword ii=0; ii < N; ++ii)
      {
      out_mem[ii] = std::complex<T>(A[ii], B[ii]);
      }
    }
  else
    {
    for(uword ucol=0; ucol < local_n_cols; ++ucol)
    for(uword urow=0; urow < local_n_rows; ++urow)
      {
      *out_mem = std::complex<T>(PX.at(urow,ucol), PY.at(urow,ucol));
      out_mem++;
      }
    }
  }



//! swap the contents of this matrix, denoted as matrix A, with given matrix B
template<typename eT>
inline
void
Mat<eT>::swap(Mat<eT>& B)
  {
  Mat<eT>& A = (*this);

  arma_extra_debug_sigprint(arma_str::format("A = %x   B = %x") % &A % &B);

  bool layout_ok = false;

  if(A.vec_state == B.vec_state)
    {
    layout_ok = true;
    }
  else
    {
    const uhword A_vec_state = A.vec_state;
    const uhword B_vec_state = B.vec_state;

    const bool A_absorbs_B = (A_vec_state == 0) || ( (A_vec_state == 1) && (B.n_cols == 1) ) || ( (A_vec_state == 2) && (B.n_rows == 1) );
    const bool B_absorbs_A = (B_vec_state == 0) || ( (B_vec_state == 1) && (A.n_cols == 1) ) || ( (B_vec_state == 2) && (A.n_rows == 1) );

    layout_ok = A_absorbs_B && B_absorbs_A;
    }

  const uhword A_mem_state = A.mem_state;
  const uhword B_mem_state = B.mem_state;

  if( (A_mem_state == 0) && (B_mem_state == 0) && layout_ok )
    {
    const uword A_n_elem = A.n_elem;
    const uword B_n_elem = B.n_elem;

    const bool A_use_local_mem = (A_n_elem <= arma_config::mat_prealloc);
    const bool B_use_local_mem = (B_n_elem <= arma_config::mat_prealloc);

    if( (A_use_local_mem == false) && (B_use_local_mem == false) )
      {
      std::swap( access::rw(A.mem), access::rw(B.mem) );
      }
    else
    if( (A_use_local_mem == true) && (B_use_local_mem == true) )
      {
      eT* A_mem_local = &(A.mem_local[0]);
      eT* B_mem_local = &(B.mem_local[0]);

      access::rw(A.mem) = A_mem_local;
      access::rw(B.mem) = B_mem_local;

      const uword N = (std::max)(A_n_elem, B_n_elem);

      for(uword ii=0; ii < N; ++ii)  { std::swap( A_mem_local[ii], B_mem_local[ii] ); }
      }
    else
    if( (A_use_local_mem == true) && (B_use_local_mem == false) )
      {
      eT* A_mem_local = &(A.mem_local[0]);
      eT* B_mem_local = &(B.mem_local[0]);

      arrayops::copy(B_mem_local, A_mem_local, A_n_elem);

      access::rw(A.mem) = B.mem;
      access::rw(B.mem) = B_mem_local;
      }
    else
    if( (A_use_local_mem == false) && (B_use_local_mem == true) )
      {
      eT* A_mem_local = &(A.mem_local[0]);
      eT* B_mem_local = &(B.mem_local[0]);

      arrayops::copy(A_mem_local, B_mem_local, B_n_elem);

      access::rw(B.mem) = A.mem;
      access::rw(A.mem) = A_mem_local;
      }

    std::swap( access::rw(A.n_rows), access::rw(B.n_rows) );
    std::swap( access::rw(A.n_cols), access::rw(B.n_cols) );
    std::swap( access::rw(A.n_elem), access::rw(B.n_elem) );
    }
  else
  if( (A_mem_state <= 2) && (B_mem_state <= 2) && (A.n_elem == B.n_elem) && layout_ok )
    {
    std::swap( access::rw(A.n_rows), access::rw(B.n_rows) );
    std::swap( access::rw(A.n_cols), access::rw(B.n_cols) );

    const uword N = A.n_elem;

    eT* A_mem = A.memptr();
    eT* B_mem = B.memptr();

    for(uword ii=0; ii < N; ++ii)  { std::swap(A_mem[ii], B_mem[ii]); }
    }
  else
  if( (A.n_rows == B.n_rows) && (A.n_cols == B.n_cols) )
    {
    const uword N = A.n_elem;

    eT* A_mem = A.memptr();
    eT* B_mem = B.memptr();

    for(uword ii=0; ii < N; ++ii)  { std::swap(A_mem[ii], B_mem[ii]); }
    }
  else
    {
    // generic swap to handle remaining cases

    if(A.n_elem <= B.n_elem)
      {
      Mat<eT> C = A;

      A.steal_mem(B);
      B.steal_mem(C);
      }
    else
      {
      Mat<eT> C = B;

      B.steal_mem(A);
      A.steal_mem(C);
      }
    }
  }



//! try to steal the memory from a given matrix;
//! if memory can't be stolen, copy the given matrix
template<typename eT>
inline
void
Mat<eT>::steal_mem(Mat<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(this == &x)  { return; }

  const uword  x_n_rows    = x.n_rows;
  const uword  x_n_cols    = x.n_cols;
  const uword  x_n_elem    = x.n_elem;
  const uhword x_vec_state = x.vec_state;
  const uhword x_mem_state = x.mem_state;

  const uhword t_vec_state = vec_state;
  const uhword t_mem_state = mem_state;

  bool layout_ok = false;

  if(t_vec_state == x_vec_state)
    {
    layout_ok = true;
    }
  else
    {
    if( (t_vec_state == 1) && (x_n_cols == 1) )  { layout_ok = true; }
    if( (t_vec_state == 2) && (x_n_rows == 1) )  { layout_ok = true; }
    }


  if( (t_mem_state <= 1) && ( ((x_mem_state == 0) && (x_n_elem > arma_config::mat_prealloc)) || (x_mem_state == 1) ) && layout_ok )
    {
    reset();

    access::rw(n_rows)    = x_n_rows;
    access::rw(n_cols)    = x_n_cols;
    access::rw(n_elem)    = x_n_elem;
    access::rw(mem_state) = x_mem_state;
    access::rw(mem)       = x.mem;

    access::rw(x.n_rows)    = 0;
    access::rw(x.n_cols)    = 0;
    access::rw(x.n_elem)    = 0;
    access::rw(x.mem_state) = 0;
    access::rw(x.mem)       = 0;
    }
  else
    {
    (*this).operator=(x);
    }
  }



template<typename eT>
inline
void
Mat<eT>::steal_mem_col(Mat<eT>& x, const uword max_n_rows)
  {
  arma_extra_debug_sigprint();

  const uword  x_n_elem    = x.n_elem;
  const uhword x_mem_state = x.mem_state;

  const uhword t_vec_state = vec_state;
  const uhword t_mem_state = mem_state;

  const uword alt_n_rows = (std::min)(x.n_rows, max_n_rows);

  if((x_n_elem == 0) || (alt_n_rows == 0))
    {
    (*this).set_size(0,1);

    return;
    }

  if( (this != &x) && (t_vec_state <= 1) && (t_mem_state <= 1) && (x_mem_state <= 1) )
    {
    if( (x_mem_state == 0) && ((x_n_elem <= arma_config::mat_prealloc) || (alt_n_rows <= arma_config::mat_prealloc)) )
      {
      (*this).set_size(alt_n_rows, uword(1));

      arrayops::copy( (*this).memptr(), x.memptr(), alt_n_rows );
      }
    else
      {
      reset();

      access::rw(n_rows)    = alt_n_rows;
      access::rw(n_cols)    = 1;
      access::rw(n_elem)    = alt_n_rows;
      access::rw(mem_state) = x_mem_state;
      access::rw(mem)       = x.mem;

      access::rw(x.n_rows)    = 0;
      access::rw(x.n_cols)    = 0;
      access::rw(x.n_elem)    = 0;
      access::rw(x.mem_state) = 0;
      access::rw(x.mem)       = 0;
      }
    }
  else
    {
    Mat<eT> tmp(alt_n_rows, 1);

    arrayops::copy( tmp.memptr(), x.memptr(), alt_n_rows );

    steal_mem(tmp);
    }
  }



//! construct a matrix from a given auxiliary array of eTs.
//! if copy_aux_mem is true, new memory is allocated and the array is copied.
//! if copy_aux_mem is false, the auxiliary array is used directly (without allocating memory and copying).
//! the default is to copy the array.

template<typename eT>
inline
Mat<eT>::Mat(eT* aux_mem, const uword aux_n_rows, const uword aux_n_cols, const bool copy_aux_mem, const bool strict)
  : n_rows   ( aux_n_rows                            )
  , n_cols   ( aux_n_cols                            )
  , n_elem   ( aux_n_rows*aux_n_cols                 )
  , vec_state( 0                                     )
  , mem_state( copy_aux_mem ? 0 : ( strict ? 2 : 1 ) )
  , mem      ( copy_aux_mem ? 0 : aux_mem            )
  {
  arma_extra_debug_sigprint_this(this);

  if(copy_aux_mem == true)
    {
    init_cold();

    arrayops::copy( memptr(), aux_mem, n_elem );
    }
  }



//! construct a matrix from a given auxiliary read-only array of eTs.
//! the array is copied.
template<typename eT>
inline
Mat<eT>::Mat(const eT* aux_mem, const uword aux_n_rows, const uword aux_n_cols)
  : n_rows(aux_n_rows)
  , n_cols(aux_n_cols)
  , n_elem(aux_n_rows*aux_n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  arrayops::copy( memptr(), aux_mem, n_elem );
  }



//! DANGEROUS! Construct a temporary matrix, using auxiliary memory.
//! This constructor is NOT intended for usage by user code.
//! Its sole purpose is to be used by the Cube class.

template<typename eT>
inline
Mat<eT>::Mat(const char junk, const eT* aux_mem, const uword aux_n_rows, const uword aux_n_cols)
  : n_rows   (aux_n_rows           )
  , n_cols   (aux_n_cols           )
  , n_elem   (aux_n_rows*aux_n_cols)
  , vec_state(0                    )
  , mem_state(3                    )
  , mem      (aux_mem              )
  {
  arma_extra_debug_sigprint_this(this);
  arma_ignore(junk);
  }



//! in-place matrix addition
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const Mat<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "addition");

  arrayops::inplace_plus( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place matrix subtraction
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const Mat<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "subtraction");

  arrayops::inplace_minus( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place matrix multiplication
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const Mat<eT>& m)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace(*this, m);

  return *this;
  }



//! in-place element-wise matrix multiplication
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator%=(const Mat<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "element-wise multiplication");

  arrayops::inplace_mul( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place element-wise matrix division
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const Mat<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "element-wise division");

  arrayops::inplace_div( memptr(), m.memptr(), n_elem );

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>::Mat(const BaseCube<eT,T1>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  (*this).operator=(X);
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& out = *this;

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& in  = tmp.M;

  arma_debug_assert_cube_as_mat(out, in, "copy into matrix", false);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    out.set_size(in_n_rows, in_n_cols);

    for(uword ucol=0; ucol < in_n_cols; ++ucol)
      {
      arrayops::copy( out.colptr(ucol), in.slice_colptr(0, ucol), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if(in_n_cols == 1)
        {
        out.set_size(in_n_rows, in_n_slices);

        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::copy( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if(in_n_rows == 1)
        {
        out.set_size(in_n_cols, in_n_slices);

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = in.at(0, i, slice);
            const eT tmp_j = in.at(0, j, slice);

            out_colptr[i] = tmp_i;
            out_colptr[j] = tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] = in.at(0, i, slice);
            }
          }
        }
      }
    else
      {
      out.set_size(in_n_slices);

      eT* out_mem = out.memptr();

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] = in.at(0, 0, i);
        }
      }
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator+=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& out = *this;

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& in  = tmp.M;

  arma_debug_assert_cube_as_mat(out, in, "addition", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    for(uword ucol=0; ucol < in_n_cols; ++ucol)
      {
      arrayops::inplace_plus( out.colptr(ucol), in.slice_colptr(0, ucol), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_plus( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = in.at(0, i, slice);
            const eT tmp_j = in.at(0, j, slice);

            out_colptr[i] += tmp_i;
            out_colptr[j] += tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] += in.at(0, i, slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] += in.at(0, 0, i);
        }
      }
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator-=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& out = *this;

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& in  = tmp.M;

  arma_debug_assert_cube_as_mat(out, in, "subtraction", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    for(uword ucol=0; ucol < in_n_cols; ++ucol)
      {
      arrayops::inplace_minus( out.colptr(ucol), in.slice_colptr(0, ucol), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_minus( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = in.at(0, i, slice);
            const eT tmp_j = in.at(0, j, slice);

            out_colptr[i] -= tmp_i;
            out_colptr[j] -= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] -= in.at(0, i, slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] -= in.at(0, 0, i);
        }
      }
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator*=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> B(X);

  (*this).operator*=(B);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator%=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& out = *this;

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& in  = tmp.M;

  arma_debug_assert_cube_as_mat(out, in, "element-wise multiplication", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    for(uword ucol=0; ucol < in_n_cols; ++ucol)
      {
      arrayops::inplace_mul( out.colptr(ucol), in.slice_colptr(0, ucol), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_mul( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = in.at(0, i, slice);
            const eT tmp_j = in.at(0, j, slice);

            out_colptr[i] *= tmp_i;
            out_colptr[j] *= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] *= in.at(0, i, slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] *= in.at(0, 0, i);
        }
      }
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator/=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& out = *this;

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& in  = tmp.M;

  arma_debug_assert_cube_as_mat(out, in, "element-wise division", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    for(uword ucol=0; ucol < in_n_cols; ++ucol)
      {
      arrayops::inplace_div( out.colptr(ucol), in.slice_colptr(0, ucol), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_div( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = in.at(0, i, slice);
            const eT tmp_j = in.at(0, j, slice);

            out_colptr[i] /= tmp_i;
            out_colptr[j] /= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] /= in.at(0, i, slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] /= in.at(0, 0, i);
        }
      }
    }

  return *this;
  }



//! for constructing a complex matrix out of two non-complex matrices
template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>::Mat
  (
  const Base<typename Mat<eT>::pod_type,T1>& A,
  const Base<typename Mat<eT>::pod_type,T2>& B
  )
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init(A,B);
  }



template<typename eT>
inline
Mat<eT>::Mat(const subview<eT>& X, const bool use_colmem)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(use_colmem ? 3 : 0)
  , mem      (use_colmem ? X.colptr(0) : NULL)
  {
  arma_extra_debug_sigprint_this(this);

  if(use_colmem)
    {
    arma_extra_debug_print("Mat::Mat(): using existing memory in a submatrix");
    }
  else
    {
    init_cold();

    subview<eT>::extract(*this, X);
    }
  }



//! construct a matrix from subview (e.g. construct a matrix from a delayed submatrix operation)
template<typename eT>
inline
Mat<eT>::Mat(const subview<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  subview<eT>::extract(*this, X);
  }



//! construct a matrix from subview (e.g. construct a matrix from a delayed submatrix operation)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const bool alias = (this == &(X.m));

  if(alias == false)
    {
    init_warm(X.n_rows, X.n_cols);

    subview<eT>::extract(*this, X);
    }
  else
    {
    Mat<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }


//! in-place matrix addition (using a submatrix on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::plus_inplace(*this, X);

  return *this;
  }


//! in-place matrix subtraction (using a submatrix on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::minus_inplace(*this, X);

  return *this;
  }



//! in-place matrix mutiplication (using a submatrix on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace(*this, X);

  return *this;
  }



//! in-place element-wise matrix mutiplication (using a submatrix on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator%=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::schur_inplace(*this, X);

  return *this;
  }



//! in-place element-wise matrix division (using a submatrix on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::div_inplace(*this, X);

  return *this;
  }



template<typename eT>
inline
Mat<eT>::Mat(const subview_row_strans<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  X.extract(*this);
  }



template<typename eT>
inline
Mat<eT>::Mat(const subview_row_htrans<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  X.extract(*this);
  }



template<typename eT>
inline
Mat<eT>::Mat(const xvec_htrans<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  X.extract(*this);
  }



template<typename eT>
template<bool do_conj>
inline
Mat<eT>::Mat(const xtrans_mat<eT,do_conj>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  X.extract(*this);
  }



//! construct a matrix from a subview_cube instance
template<typename eT>
inline
Mat<eT>::Mat(const subview_cube<eT>& x)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  this->operator=(x);
  }



//! construct a matrix from a subview_cube instance
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::extract(*this, X);

  return *this;
  }



//! in-place matrix addition (using a single-slice subcube on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::plus_inplace(*this, X);

  return *this;
  }



//! in-place matrix subtraction (using a single-slice subcube on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::minus_inplace(*this, X);

  return *this;
  }



//! in-place matrix mutiplication (using a single-slice subcube on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);
  glue_times::apply_inplace(*this, tmp);

  return *this;
  }



//! in-place element-wise matrix mutiplication (using a single-slice subcube on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator%=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::schur_inplace(*this, X);

  return *this;
  }



//! in-place element-wise matrix division (using a single-slice subcube on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::div_inplace(*this, X);

  return *this;
  }



//! construct a matrix from diagview (e.g. construct a matrix from a delayed diag operation)
template<typename eT>
inline
Mat<eT>::Mat(const diagview<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  diagview<eT>::extract(*this, X);
  }



//! construct a matrix from diagview (e.g. construct a matrix from a delayed diag operation)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const bool alias = (this == &(X.m));

  if(alias == false)
    {
    init_warm(X.n_rows, X.n_cols);

    diagview<eT>::extract(*this, X);
    }
  else
    {
    Mat<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



//! in-place matrix addition (using a diagview on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  diagview<eT>::plus_inplace(*this, X);

  return *this;
  }



//! in-place matrix subtraction (using a diagview on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  diagview<eT>::minus_inplace(*this, X);

  return *this;
  }



//! in-place matrix mutiplication (using a diagview on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace(*this, X);

  return *this;
  }



//! in-place element-wise matrix mutiplication (using a diagview on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator%=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  diagview<eT>::schur_inplace(*this, X);

  return *this;
  }



//! in-place element-wise matrix division (using a diagview on the right-hand-side)
template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  diagview<eT>::div_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>::Mat(const subview_elem1<eT,T1>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  this->operator=(X);
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem1<eT,T1>::extract(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator+=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem1<eT,T1>::plus_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator-=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem1<eT,T1>::minus_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator*=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator%=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem1<eT,T1>::schur_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator/=(const subview_elem1<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem1<eT,T1>::div_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>::Mat(const subview_elem2<eT,T1,T2>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  this->operator=(X);
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem2<eT,T1,T2>::extract(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator+=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem2<eT,T1,T2>::plus_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator-=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem2<eT,T1,T2>::minus_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator*=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator%=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem2<eT,T1,T2>::schur_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator/=(const subview_elem2<eT,T1,T2>& X)
  {
  arma_extra_debug_sigprint();

  subview_elem2<eT,T1,T2>::div_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>::Mat(const SpBase<eT, T1>& m)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  const SpProxy<T1> p(m.get_ref());

  access::rw(n_rows) = p.get_n_rows();
  access::rw(n_cols) = p.get_n_cols();
  access::rw(n_elem) = p.get_n_elem();

  init_cold();

  zeros();

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  while(it != it_end)
    {
    at(it.row(), it.col()) = (*it);
    ++it;
    }
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(m.get_ref());

  init_warm(p.get_n_rows(), p.get_n_cols());

  zeros();

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  while(it != it_end)
    {
    at(it.row(), it.col()) = (*it);
    ++it;
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator+=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(m.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "addition");

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  while(it != it_end)
    {
    at(it.row(), it.col()) += (*it);
    ++it;
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator-=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(m.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "subtraction");

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  while(it != it_end)
    {
    at(it.row(), it.col()) -= (*it);
    ++it;
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator*=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  Mat<eT> z = (*this) * m.get_ref();

  steal_mem(z);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator%=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(m.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "element-wise multiplication");

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  // We have to zero everything that isn't being used.
  arrayops::inplace_set(memptr(), eT(0), (it.col() * n_rows) + it.row());

  while(it != it_end)
    {
    const uword cur_loc = (it.col() * n_rows) + it.row();

    access::rw(mem[cur_loc]) *= (*it);

    ++it;

    const uword next_loc = (it == it_end)
      ? (p.get_n_cols() * n_rows)
      : (it.col() * n_rows) + it.row();

    arrayops::inplace_set(memptr() + cur_loc + 1, eT(0), (next_loc - cur_loc - 1));
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Mat<eT>&
Mat<eT>::operator/=(const SpBase<eT, T1>& m)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(m.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "element-wise division");

  // If you use this method, you are probably stupid or misguided, but for completeness it is implemented.
  // Unfortunately the best way to do this is loop over every element.
  for(uword c = 0; c < n_cols; ++c)
  for(uword r = 0; r < n_rows; ++r)
    {
    at(r, c) /= p.at(r, c);
    }

  return *this;
  }



template<typename eT>
inline
Mat<eT>::Mat(const spdiagview<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(X.n_elem)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  spdiagview<eT>::extract(*this, X);
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  init_warm(X.n_rows, X.n_cols);

  spdiagview<eT>::extract(*this, X);

  return *this;
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator+=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);

  (*this).operator+=(tmp);

  return *this;
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator-=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);

  (*this).operator-=(tmp);

  return *this;
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator*=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);

  (*this).operator*=(tmp);

  return *this;
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator%=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);

  (*this).operator%=(tmp);

  return *this;
  }



template<typename eT>
inline
Mat<eT>&
Mat<eT>::operator/=(const spdiagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(X);

  (*this).operator/=(tmp);

  return *this;
  }



template<typename eT>
inline
mat_injector< Mat<eT> >
Mat<eT>::operator<<(const eT val)
  {
  return mat_injector< Mat<eT> >(*this, val);
  }



template<typename eT>
inline
mat_injector< Mat<eT> >
Mat<eT>::operator<<(const injector_end_of_row<>& x)
  {
  return mat_injector< Mat<eT> >(*this, x);
  }



//! creation of subview (row vector)
template<typename eT>
arma_inline
subview_row<eT>
Mat<eT>::row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= n_rows, "Mat::row(): index out of bounds" );

  return subview_row<eT>(*this, row_num);
  }



//! creation of subview (row vector)
template<typename eT>
arma_inline
const subview_row<eT>
Mat<eT>::row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= n_rows, "Mat::row(): index out of bounds" );

  return subview_row<eT>(*this, row_num);
  }



template<typename eT>
inline
subview_row<eT>
Mat<eT>::operator()(const uword row_num, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    (row_num >= n_rows)
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::operator(): indices out of bounds or incorrectly used"
    );

  return subview_row<eT>(*this, row_num, in_col1, submat_n_cols);
  }



template<typename eT>
inline
const subview_row<eT>
Mat<eT>::operator()(const uword row_num, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    (row_num >= n_rows)
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::operator(): indices out of bounds or incorrectly used"
    );

  return subview_row<eT>(*this, row_num, in_col1, submat_n_cols);
  }



//! creation of subview (column vector)
template<typename eT>
arma_inline
subview_col<eT>
Mat<eT>::col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "Mat::col(): index out of bounds");

  return subview_col<eT>(*this, col_num);
  }



//! creation of subview (column vector)
template<typename eT>
arma_inline
const subview_col<eT>
Mat<eT>::col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "Mat::col(): index out of bounds");

  return subview_col<eT>(*this, col_num);
  }



template<typename eT>
inline
subview_col<eT>
Mat<eT>::operator()(const span& row_span, const uword col_num)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  arma_debug_check
    (
    (col_num >= n_cols)
    ||
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "Mat::operator(): indices out of bounds or incorrectly used"
    );

  return subview_col<eT>(*this, col_num, in_row1, submat_n_rows);
  }



template<typename eT>
inline
const subview_col<eT>
Mat<eT>::operator()(const span& row_span, const uword col_num) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  arma_debug_check
    (
    (col_num >= n_cols)
    ||
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "Mat::operator(): indices out of bounds or incorrectly used"
    );

  return subview_col<eT>(*this, col_num, in_row1, submat_n_rows);
  }



//! create a Col object which uses memory from an existing matrix object.
//! this approach is currently not alias safe
//! and does not take into account that the parent matrix object could be deleted.
//! if deleted memory is accessed by the created Col object,
//! it will cause memory corruption and/or a crash
template<typename eT>
inline
Col<eT>
Mat<eT>::unsafe_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "Mat::unsafe_col(): index out of bounds");

  return Col<eT>(colptr(col_num), n_rows, false, true);
  }



//! create a Col object which uses memory from an existing matrix object.
//! this approach is currently not alias safe
//! and does not take into account that the parent matrix object could be deleted.
//! if deleted memory is accessed by the created Col object,
//! it will cause memory corruption and/or a crash
template<typename eT>
inline
const Col<eT>
Mat<eT>::unsafe_col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "Mat::unsafe_col(): index out of bounds");

  typedef const Col<eT> out_type;

  return out_type(const_cast<eT*>(colptr(col_num)), n_rows, false, true);
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
arma_inline
subview<eT>
Mat<eT>::rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "Mat::rows(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  return subview<eT>(*this, in_row1, 0, subview_n_rows, n_cols );
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
arma_inline
const subview<eT>
Mat<eT>::rows(const uword in_row1, const uword in_row2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "Mat::rows(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  return subview<eT>(*this, in_row1, 0, subview_n_rows, n_cols );
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
arma_inline
subview<eT>
Mat<eT>::cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "Mat::cols(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview<eT>(*this, 0, in_col1, n_rows, subview_n_cols);
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
arma_inline
const subview<eT>
Mat<eT>::cols(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "Mat::cols(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview<eT>(*this, 0, in_col1, n_rows, subview_n_cols);
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
inline
subview<eT>
Mat<eT>::rows(const span& row_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "Mat::rows(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, in_row1, 0, submat_n_rows, n_cols);
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
inline
const subview<eT>
Mat<eT>::rows(const span& row_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "Mat::rows(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, in_row1, 0, submat_n_rows, n_cols);
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
arma_inline
subview<eT>
Mat<eT>::cols(const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::cols(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, 0, in_col1, n_rows, submat_n_cols);
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
arma_inline
const subview<eT>
Mat<eT>::cols(const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::cols(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, 0, in_col1, n_rows, submat_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
arma_inline
subview<eT>
Mat<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 >  in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "Mat::submat(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview<eT>(*this, in_row1, in_col1, subview_n_rows, subview_n_cols);
  }



//! creation of subview (generic submatrix)
template<typename eT>
arma_inline
const subview<eT>
Mat<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 >  in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "Mat::submat(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview<eT>(*this, in_row1, in_col1, subview_n_rows, subview_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
arma_inline
subview<eT>
Mat<eT>::submat(const uword in_row1, const uword in_col1, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows = n_rows;
  const uword l_n_cols = n_cols;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  arma_debug_check
    (
    ((in_row1 >= l_n_rows) || (in_col1 >= l_n_cols) || ((in_row1 + s_n_rows) > l_n_rows) || ((in_col1 + s_n_cols) > l_n_cols)),
    "Mat::submat(): indices or size out of bounds"
    );

  return subview<eT>(*this, in_row1, in_col1, s_n_rows, s_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
arma_inline
const subview<eT>
Mat<eT>::submat(const uword in_row1, const uword in_col1, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows = n_rows;
  const uword l_n_cols = n_cols;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  arma_debug_check
    (
    ((in_row1 >= l_n_rows) || (in_col1 >= l_n_cols) || ((in_row1 + s_n_rows) > l_n_rows) || ((in_col1 + s_n_cols) > l_n_cols)),
    "Mat::submat(): indices or size out of bounds"
    );

  return subview<eT>(*this, in_row1, in_col1, s_n_rows, s_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
inline
subview<eT>
Mat<eT>::submat(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::submat(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, in_row1, in_col1, submat_n_rows, submat_n_cols);
  }



//! creation of subview (generic submatrix)
template<typename eT>
inline
const subview<eT>
Mat<eT>::submat(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Mat::submat(): indices out of bounds or incorrectly used"
    );

  return subview<eT>(*this, in_row1, in_col1, submat_n_rows, submat_n_cols);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::operator()(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  return (*this).submat(row_span, col_span);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::operator()(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return (*this).submat(row_span, col_span);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::operator()(const uword in_row1, const uword in_col1, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).submat(in_row1, in_col1, s);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::operator()(const uword in_row1, const uword in_col1, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  return (*this).submat(in_row1, in_col1, s);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::head_rows(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_rows), "Mat::head_rows(): size out of bounds");

  return subview<eT>(*this, 0, 0, N, n_cols);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::head_rows(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_rows), "Mat::head_rows(): size out of bounds");

  return subview<eT>(*this, 0, 0, N, n_cols);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::tail_rows(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_rows), "Mat::tail_rows(): size out of bounds");

  const uword start_row = n_rows - N;

  return subview<eT>(*this, start_row, 0, N, n_cols);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::tail_rows(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_rows), "Mat::tail_rows(): size out of bounds");

  const uword start_row = n_rows - N;

  return subview<eT>(*this, start_row, 0, N, n_cols);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::head_cols(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_cols), "Mat::head_cols(): size out of bounds");

  return subview<eT>(*this, 0, 0, n_rows, N);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::head_cols(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_cols), "Mat::head_cols(): size out of bounds");

  return subview<eT>(*this, 0, 0, n_rows, N);
  }



template<typename eT>
inline
subview<eT>
Mat<eT>::tail_cols(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_cols), "Mat::tail_cols(): size out of bounds");

  const uword start_col = n_cols - N;

  return subview<eT>(*this, 0, start_col, n_rows, N);
  }



template<typename eT>
inline
const subview<eT>
Mat<eT>::tail_cols(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_cols), "Mat::tail_cols(): size out of bounds");

  const uword start_col = n_cols - N;

  return subview<eT>(*this, 0, start_col, n_rows, N);
  }



template<typename eT>
template<typename T1>
arma_inline
subview_elem1<eT,T1>
Mat<eT>::elem(const Base<uword,T1>& a)
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
const subview_elem1<eT,T1>
Mat<eT>::elem(const Base<uword,T1>& a) const
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
subview_elem1<eT,T1>
Mat<eT>::operator()(const Base<uword,T1>& a)
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
const subview_elem1<eT,T1>
Mat<eT>::operator()(const Base<uword,T1>& a) const
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
subview_elem2<eT,T1,T2>
Mat<eT>::elem(const Base<uword,T1>& ri, const Base<uword,T2>& ci)
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
const subview_elem2<eT,T1,T2>
Mat<eT>::elem(const Base<uword,T1>& ri, const Base<uword,T2>& ci) const
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
subview_elem2<eT,T1,T2>
Mat<eT>::submat(const Base<uword,T1>& ri, const Base<uword,T2>& ci)
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
const subview_elem2<eT,T1,T2>
Mat<eT>::submat(const Base<uword,T1>& ri, const Base<uword,T2>& ci) const
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
subview_elem2<eT,T1,T2>
Mat<eT>::operator()(const Base<uword,T1>& ri, const Base<uword,T2>& ci)
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1, typename T2>
arma_inline
const subview_elem2<eT,T1,T2>
Mat<eT>::operator()(const Base<uword,T1>& ri, const Base<uword,T2>& ci) const
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T2>(*this, ri, ci, false, false);
  }



template<typename eT>
template<typename T1>
arma_inline
subview_elem2<eT,T1,T1>
Mat<eT>::rows(const Base<uword,T1>& ri)
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T1>(*this, ri, ri, false, true);
  }



template<typename eT>
template<typename T1>
arma_inline
const subview_elem2<eT,T1,T1>
Mat<eT>::rows(const Base<uword,T1>& ri) const
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T1,T1>(*this, ri, ri, false, true);
  }



template<typename eT>
template<typename T2>
arma_inline
subview_elem2<eT,T2,T2>
Mat<eT>::cols(const Base<uword,T2>& ci)
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T2,T2>(*this, ci, ci, true, false);
  }



template<typename eT>
template<typename T2>
arma_inline
const subview_elem2<eT,T2,T2>
Mat<eT>::cols(const Base<uword,T2>& ci) const
  {
  arma_extra_debug_sigprint();

  return subview_elem2<eT,T2,T2>(*this, ci, ci, true, false);
  }



template<typename eT>
arma_inline
subview_each1< Mat<eT>, 0 >
Mat<eT>::each_col()
  {
  arma_extra_debug_sigprint();

  return subview_each1< Mat<eT>, 0>(*this);
  }



template<typename eT>
arma_inline
subview_each1< Mat<eT>, 1 >
Mat<eT>::each_row()
  {
  arma_extra_debug_sigprint();

  return subview_each1< Mat<eT>, 1>(*this);
  }



template<typename eT>
arma_inline
const subview_each1< Mat<eT>, 0 >
Mat<eT>::each_col() const
  {
  arma_extra_debug_sigprint();

  return subview_each1< Mat<eT>, 0>(*this);
  }



template<typename eT>
arma_inline
const subview_each1< Mat<eT>, 1 >
Mat<eT>::each_row() const
  {
  arma_extra_debug_sigprint();

  return subview_each1< Mat<eT>, 1>(*this);
  }



template<typename eT>
template<typename T1>
inline
subview_each2< Mat<eT>, 0, T1 >
Mat<eT>::each_col(const Base<uword, T1>& indices)
  {
  arma_extra_debug_sigprint();

  return subview_each2< Mat<eT>, 0, T1 >(*this, indices);
  }



template<typename eT>
template<typename T1>
inline
subview_each2< Mat<eT>, 1, T1 >
Mat<eT>::each_row(const Base<uword, T1>& indices)
  {
  arma_extra_debug_sigprint();

  return subview_each2< Mat<eT>, 1, T1 >(*this, indices);
  }



template<typename eT>
template<typename T1>
inline
const subview_each2< Mat<eT>, 0, T1 >
Mat<eT>::each_col(const Base<uword, T1>& indices) const
  {
  arma_extra_debug_sigprint();

  return subview_each2< Mat<eT>, 0, T1 >(*this, indices);
  }



template<typename eT>
template<typename T1>
inline
const subview_each2< Mat<eT>, 1, T1 >
Mat<eT>::each_row(const Base<uword, T1>& indices) const
  {
  arma_extra_debug_sigprint();

  return subview_each2< Mat<eT>, 1, T1 >(*this, indices);
  }



#if defined(ARMA_USE_CXX11)

  //! apply a lambda function to each column, where each column is interpreted as a column vector
  template<typename eT>
  inline
  const Mat<eT>&
  Mat<eT>::each_col(const std::function< void(Col<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    for(uword ii=0; ii < n_cols; ++ii)
      {
      Col<eT> tmp(colptr(ii), n_rows, false, true);
      F(tmp);
      }

    return *this;
    }



  template<typename eT>
  inline
  const Mat<eT>&
  Mat<eT>::each_col(const std::function< void(const Col<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

    for(uword ii=0; ii < n_cols; ++ii)
      {
      const Col<eT> tmp(const_cast<eT*>(colptr(ii)), n_rows, false, true);
      F(tmp);
      }

    return *this;
    }



  //! apply a lambda function to each row, where each row is interpreted as a row vector
  template<typename eT>
  inline
  const Mat<eT>&
  Mat<eT>::each_row(const std::function< void(Row<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    podarray<eT> array1(n_cols);
    podarray<eT> array2(n_cols);

    Row<eT> tmp1( array1.memptr(), n_cols, false, true );
    Row<eT> tmp2( array2.memptr(), n_cols, false, true );

    eT* tmp1_mem = tmp1.memptr();
    eT* tmp2_mem = tmp2.memptr();

    uword ii, jj;

    for(ii=0, jj=1; jj < n_rows; ii+=2, jj+=2)
      {
      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        const eT* col_mem = colptr(col_id);

        tmp1_mem[col_id] = col_mem[ii];
        tmp2_mem[col_id] = col_mem[jj];
        }

      F(tmp1);
      F(tmp2);

      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        eT* col_mem = colptr(col_id);

        col_mem[ii] = tmp1_mem[col_id];
        col_mem[jj] = tmp2_mem[col_id];
        }
      }

    if(ii < n_rows)
      {
      tmp1 = (*this).row(ii);

      F(tmp1);

      (*this).row(ii) = tmp1;
      }

    return *this;
    }



  template<typename eT>
  inline
  const Mat<eT>&
  Mat<eT>::each_row(const std::function< void(const Row<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

    podarray<eT> array1(n_cols);
    podarray<eT> array2(n_cols);

    Row<eT> tmp1( array1.memptr(), n_cols, false, true );
    Row<eT> tmp2( array2.memptr(), n_cols, false, true );

    eT* tmp1_mem = tmp1.memptr();
    eT* tmp2_mem = tmp2.memptr();

    uword ii, jj;

    for(ii=0, jj=1; jj < n_rows; ii+=2, jj+=2)
      {
      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        const eT* col_mem = colptr(col_id);

        tmp1_mem[col_id] = col_mem[ii];
        tmp2_mem[col_id] = col_mem[jj];
        }

      F(tmp1);
      F(tmp2);
      }

    if(ii < n_rows)
      {
      tmp1 = (*this).row(ii);

      F(tmp1);
      }

    return *this;
    }

#endif



//! creation of diagview (diagonal)
template<typename eT>
arma_inline
diagview<eT>
Mat<eT>::diag(const sword in_id)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (in_id < 0) ? uword(-in_id) : 0;
  const uword col_offset = (in_id > 0) ? uword( in_id) : 0;

  arma_debug_check
    (
    ((row_offset > 0) && (row_offset >= n_rows)) || ((col_offset > 0) && (col_offset >= n_cols)),
    "Mat::diag(): requested diagonal out of bounds"
    );

  const uword len = (std::min)(n_rows - row_offset, n_cols - col_offset);

  return diagview<eT>(*this, row_offset, col_offset, len);
  }



//! creation of diagview (diagonal)
template<typename eT>
arma_inline
const diagview<eT>
Mat<eT>::diag(const sword in_id) const
  {
  arma_extra_debug_sigprint();

  const uword row_offset = uword( (in_id < 0) ? -in_id : 0 );
  const uword col_offset = uword( (in_id > 0) ?  in_id : 0 );

  arma_debug_check
    (
    ((row_offset > 0) && (row_offset >= n_rows)) || ((col_offset > 0) && (col_offset >= n_cols)),
    "Mat::diag(): requested diagonal out of bounds"
    );

  const uword len = (std::min)(n_rows - row_offset, n_cols - col_offset);

  return diagview<eT>(*this, row_offset, col_offset, len);
  }



template<typename eT>
inline
void
Mat<eT>::swap_rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  arma_debug_check
    (
    (in_row1 >= local_n_rows) || (in_row2 >= local_n_rows),
    "Mat::swap_rows(): index out of bounds"
    );

  if(n_elem > 0)
    {
    for(uword ucol=0; ucol < local_n_cols; ++ucol)
      {
      const uword offset = ucol * local_n_rows;
      const uword pos1   = in_row1 + offset;
      const uword pos2   = in_row2 + offset;

      std::swap( access::rw(mem[pos1]), access::rw(mem[pos2]) );
      }
    }
  }



template<typename eT>
inline
void
Mat<eT>::swap_cols(const uword in_colA, const uword in_colB)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  arma_debug_check
    (
    (in_colA >= local_n_cols) || (in_colB >= local_n_cols),
    "Mat::swap_cols(): index out of bounds"
    );

  if(n_elem > 0)
    {
    eT* ptrA = colptr(in_colA);
    eT* ptrB = colptr(in_colB);

    eT tmp_i;
    eT tmp_j;

    uword iq,jq;
    for(iq=0, jq=1; jq < local_n_rows; iq+=2, jq+=2)
      {
      tmp_i = ptrA[iq];
      tmp_j = ptrA[jq];

      ptrA[iq] = ptrB[iq];
      ptrA[jq] = ptrB[jq];

      ptrB[iq] = tmp_i;
      ptrB[jq] = tmp_j;
      }

    if(iq < local_n_rows)
      {
      std::swap( ptrA[iq], ptrB[iq] );
      }
    }
  }



//! remove specified row
template<typename eT>
inline
void
Mat<eT>::shed_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= n_rows, "Mat::shed_row(): index out of bounds");

  shed_rows(row_num, row_num);
  }



//! remove specified column
template<typename eT>
inline
void
Mat<eT>::shed_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "Mat::shed_col(): index out of bounds");

  shed_cols(col_num, col_num);
  }



//! remove specified rows
template<typename eT>
inline
void
Mat<eT>::shed_rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "Mat::shed_rows(): indices out of bounds or incorrectly used"
    );

  const uword n_keep_front = in_row1;
  const uword n_keep_back  = n_rows - (in_row2 + 1);

  Mat<eT> X(n_keep_front + n_keep_back, n_cols);

  if(n_keep_front > 0)
    {
    X.rows( 0, (n_keep_front-1) ) = rows( 0, (in_row1-1) );
    }

  if(n_keep_back > 0)
    {
    X.rows( n_keep_front,  (n_keep_front+n_keep_back-1) ) = rows( (in_row2+1), (n_rows-1) );
    }

  steal_mem(X);
  }



//! remove specified columns
template<typename eT>
inline
void
Mat<eT>::shed_cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "Mat::shed_cols(): indices out of bounds or incorrectly used"
    );

  const uword n_keep_front = in_col1;
  const uword n_keep_back  = n_cols - (in_col2 + 1);

  Mat<eT> X(n_rows, n_keep_front + n_keep_back);

  if(n_keep_front > 0)
    {
    X.cols( 0, (n_keep_front-1) ) = cols( 0, (in_col1-1) );
    }

  if(n_keep_back > 0)
    {
    X.cols( n_keep_front,  (n_keep_front+n_keep_back-1) ) = cols( (in_col2+1), (n_cols-1) );
    }

  steal_mem(X);
  }



//! insert N rows at the specified row position,
//! optionally setting the elements of the inserted rows to zero
template<typename eT>
inline
void
Mat<eT>::insert_rows(const uword row_num, const uword N, const bool set_to_zero)
  {
  arma_extra_debug_sigprint();

  const uword t_n_rows = n_rows;
  const uword t_n_cols = n_cols;

  const uword A_n_rows = row_num;
  const uword B_n_rows = t_n_rows - row_num;

  // insertion at row_num == n_rows is in effect an append operation
  arma_debug_check( (row_num > t_n_rows), "Mat::insert_rows(): index out of bounds");

  if(N > 0)
    {
    Mat<eT> out(t_n_rows + N, t_n_cols);

    if(A_n_rows > 0)
      {
      out.rows(0, A_n_rows-1) = rows(0, A_n_rows-1);
      }

    if(B_n_rows > 0)
      {
      out.rows(row_num + N, t_n_rows + N - 1) = rows(row_num, t_n_rows-1);
      }

    if(set_to_zero == true)
      {
      out.rows(row_num, row_num + N - 1).zeros();
      }

    steal_mem(out);
    }
  }



//! insert N columns at the specified column position,
//! optionally setting the elements of the inserted columns to zero
template<typename eT>
inline
void
Mat<eT>::insert_cols(const uword col_num, const uword N, const bool set_to_zero)
  {
  arma_extra_debug_sigprint();

  const uword t_n_rows = n_rows;
  const uword t_n_cols = n_cols;

  const uword A_n_cols = col_num;
  const uword B_n_cols = t_n_cols - col_num;

  // insertion at col_num == n_cols is in effect an append operation
  arma_debug_check( (col_num > t_n_cols), "Mat::insert_cols(): index out of bounds");

  if(N > 0)
    {
    Mat<eT> out(t_n_rows, t_n_cols + N);

    if(A_n_cols > 0)
      {
      out.cols(0, A_n_cols-1) = cols(0, A_n_cols-1);
      }

    if(B_n_cols > 0)
      {
      out.cols(col_num + N, t_n_cols + N - 1) = cols(col_num, t_n_cols-1);
      }

    if(set_to_zero == true)
      {
      out.cols(col_num, col_num + N - 1).zeros();
      }

    steal_mem(out);
    }
  }



//! insert the given object at the specified row position;
//! the given object must have the same number of columns as the matrix
template<typename eT>
template<typename T1>
inline
void
Mat<eT>::insert_rows(const uword row_num, const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& C = tmp.M;

  const uword C_n_rows = C.n_rows;
  const uword C_n_cols = C.n_cols;

  const uword t_n_rows = n_rows;
  const uword t_n_cols = n_cols;

  const uword A_n_rows = row_num;
  const uword B_n_rows = t_n_rows - row_num;

  bool  err_state = false;
  char* err_msg   = 0;

  // insertion at row_num == n_rows is in effect an append operation

  arma_debug_set_error
    (
    err_state,
    err_msg,
    (row_num > t_n_rows),
    "Mat::insert_rows(): index out of bounds"
    );

  arma_debug_set_error
    (
    err_state,
    err_msg,
    ( (C_n_cols != t_n_cols) && ( (t_n_rows > 0) || (t_n_cols > 0) ) && ( (C_n_rows > 0) || (C_n_cols > 0) ) ),
    "Mat::insert_rows(): given object has an incompatible number of columns"
    );

  arma_debug_check(err_state, err_msg);

  if(C_n_rows > 0)
    {
    Mat<eT> out( t_n_rows + C_n_rows, (std::max)(t_n_cols, C_n_cols) );

    if(t_n_cols > 0)
      {
      if(A_n_rows > 0)
        {
        out.rows(0, A_n_rows-1) = rows(0, A_n_rows-1);
        }

      if( (t_n_cols > 0) && (B_n_rows > 0) )
        {
        out.rows(row_num + C_n_rows, t_n_rows + C_n_rows - 1) = rows(row_num, t_n_rows - 1);
        }
      }

    if(C_n_cols > 0)
      {
      out.rows(row_num, row_num + C_n_rows - 1) = C;
      }

    steal_mem(out);
    }
  }



//! insert the given object at the specified column position;
//! the given object must have the same number of rows as the matrix
template<typename eT>
template<typename T1>
inline
void
Mat<eT>::insert_cols(const uword col_num, const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& C = tmp.M;

  const uword C_n_rows = C.n_rows;
  const uword C_n_cols = C.n_cols;

  const uword t_n_rows = n_rows;
  const uword t_n_cols = n_cols;

  const uword A_n_cols = col_num;
  const uword B_n_cols = t_n_cols - col_num;

  bool  err_state = false;
  char* err_msg   = 0;

  // insertion at col_num == n_cols is in effect an append operation

  arma_debug_set_error
    (
    err_state,
    err_msg,
    (col_num > t_n_cols),
    "Mat::insert_cols(): index out of bounds"
    );

  arma_debug_set_error
    (
    err_state,
    err_msg,
    ( (C_n_rows != t_n_rows) && ( (t_n_rows > 0) || (t_n_cols > 0) ) && ( (C_n_rows > 0) || (C_n_cols > 0) ) ),
    "Mat::insert_cols(): given object has an incompatible number of rows"
    );

  arma_debug_check(err_state, err_msg);

  if(C_n_cols > 0)
    {
    Mat<eT> out( (std::max)(t_n_rows, C_n_rows), t_n_cols + C_n_cols );

    if(t_n_rows > 0)
      {
      if(A_n_cols > 0)
        {
        out.cols(0, A_n_cols-1) = cols(0, A_n_cols-1);
        }

      if(B_n_cols > 0)
        {
        out.cols(col_num + C_n_cols, t_n_cols + C_n_cols - 1) = cols(col_num, t_n_cols - 1);
        }
      }

    if(C_n_rows > 0)
      {
      out.cols(col_num, col_num + C_n_cols - 1) = C;
      }

    steal_mem(out);
    }
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>::Mat(const Gen<T1, gen_type>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem(n_rows*n_cols)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  init_cold();

  X.apply(*this);
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  init_warm(X.n_rows, X.n_cols);

  X.apply(*this);

  return *this;
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  X.apply_inplace_plus(*this);

  return *this;
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  X.apply_inplace_minus(*this);

  return *this;
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Mat<eT> tmp(X);

  return (*this).operator*=(tmp);
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  X.apply_inplace_schur(*this);

  return *this;
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const Gen<T1, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  X.apply_inplace_div(*this);

  return *this;
  }



//! create a matrix from Op, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>::Mat(const Op<T1, op_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  op_type::apply(*this, X);
  }



//! create a matrix from Op, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  op_type::apply(*this, X);

  return *this;
  }



//! in-place matrix addition, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator+=(m);
  }



//! in-place matrix subtraction, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator-=(m);
  }



//! in-place matrix multiplication, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  glue_times::apply_inplace(*this, X);

  return *this;
  }



//! in-place matrix element-wise multiplication, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator%=(m);
  }



//! in-place matrix element-wise division, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const Op<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator/=(m);
  }



//! create a matrix from eOp, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>::Mat(const eOp<T1, eop_type>& X)
  : n_rows(X.get_n_rows())
  , n_cols(X.get_n_cols())
  , n_elem(X.get_n_elem())
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  init_cold();

  eop_type::apply(*this, X);
  }



//! create a matrix from eOp, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const bool bad_alias = (eOp<T1, eop_type>::proxy_type::has_subview  &&  X.P.is_alias(*this));

  if(bad_alias == false)
    {
    init_warm(X.get_n_rows(), X.get_n_cols());

    eop_type::apply(*this, X);
    }
  else
    {
    arma_extra_debug_print("bad_alias = true");

    Mat<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_plus(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_minus(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  glue_times::apply_inplace(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_schur(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename eop_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_div(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>::Mat(const mtOp<eT, T1, op_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  op_type::apply(*this, X);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  op_type::apply(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator+=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator-=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator*=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator%=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const mtOp<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator/=(m);
  }



//! create a matrix from Glue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>::Mat(const Glue<T1, T2, glue_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  glue_type::apply(*this, X);
  }



//! create a matrix from Glue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  glue_type::apply(*this, X);

  return *this;
  }



//! in-place matrix addition, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator+=(m);
  }



//! in-place matrix subtraction, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator-=(m);
  }



//! in-place matrix multiplications, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  glue_times::apply_inplace(*this, X);

  return *this;
  }



//! in-place matrix element-wise multiplication, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator%=(m);
  }



//! in-place matrix element-wise division, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const Glue<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Mat<eT> m(X);

  return (*this).operator/=(m);
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator+=(const Glue<T1, T2, glue_times>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace_plus(*this, X, sword(+1));

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Mat<eT>&
Mat<eT>::operator-=(const Glue<T1, T2, glue_times>& X)
  {
  arma_extra_debug_sigprint();

  glue_times::apply_inplace_plus(*this, X, sword(-1));

  return *this;
  }



//! create a matrix from eGlue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>::Mat(const eGlue<T1, T2, eglue_type>& X)
  : n_rows(X.get_n_rows())
  , n_cols(X.get_n_cols())
  , n_elem(X.get_n_elem())
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  init_cold();

  eglue_type::apply(*this, X);
  }



//! create a matrix from eGlue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const bool bad_alias =
    (
    (eGlue<T1, T2, eglue_type>::proxy1_type::has_subview  &&  X.P1.is_alias(*this))
    ||
    (eGlue<T1, T2, eglue_type>::proxy2_type::has_subview  &&  X.P2.is_alias(*this))
    );

  if(bad_alias == false)
    {
    init_warm(X.get_n_rows(), X.get_n_cols());

    eglue_type::apply(*this, X);
    }
  else
    {
    arma_extra_debug_print("bad_alias = true");

    Mat<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



//! in-place matrix addition, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_plus(*this, X);

  return *this;
  }



//! in-place matrix subtraction, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_minus(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  glue_times::apply_inplace(*this, X);
  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_schur(*this, X);
  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_div(*this, X);
  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>::Mat(const mtGlue<eT, T1, T2, glue_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem(0)
  , vec_state(0)
  , mem_state(0)
  , mem()
  {
  arma_extra_debug_sigprint_this(this);

  glue_type::apply(*this, X);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  glue_type::apply(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator+=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator+=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator-=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator-=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator*=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  glue_times::apply_inplace(*this, m);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator%=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator%=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Mat<eT>&
Mat<eT>::operator/=(const mtGlue<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> m(X);

  return (*this).operator/=(m);
  }



//! linear element accessor (treats the matrix as a vector); no bounds check; assumes memory is aligned
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::at_alt(const uword ii) const
  {
  const eT* mem_aligned = mem;
  memory::mark_as_aligned(mem_aligned);

  return mem_aligned[ii];
  }



//! linear element accessor (treats the matrix as a vector); bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
eT&
Mat<eT>::operator() (const uword ii)
  {
  arma_debug_check( (ii >= n_elem), "Mat::operator(): index out of bounds");
  return access::rw(mem[ii]);
  }



//! linear element accessor (treats the matrix as a vector); bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::operator() (const uword ii) const
  {
  arma_debug_check( (ii >= n_elem), "Mat::operator(): index out of bounds");
  return mem[ii];
  }


//! linear element accessor (treats the matrix as a vector); no bounds check.
template<typename eT>
arma_inline
arma_warn_unused
eT&
Mat<eT>::operator[] (const uword ii)
  {
  return access::rw(mem[ii]);
  }



//! linear element accessor (treats the matrix as a vector); no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::operator[] (const uword ii) const
  {
  return mem[ii];
  }



//! linear element accessor (treats the matrix as a vector); no bounds check.
template<typename eT>
arma_inline
arma_warn_unused
eT&
Mat<eT>::at(const uword ii)
  {
  return access::rw(mem[ii]);
  }



//! linear element accessor (treats the matrix as a vector); no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::at(const uword ii) const
  {
  return mem[ii];
  }



//! element accessor; bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
eT&
Mat<eT>::operator() (const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row >= n_rows) || (in_col >= n_cols)), "Mat::operator(): index out of bounds");
  return access::rw(mem[in_row + in_col*n_rows]);
  }



//! element accessor; bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::operator() (const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row >= n_rows) || (in_col >= n_cols)), "Mat::operator(): index out of bounds");
  return mem[in_row + in_col*n_rows];
  }



//! element accessor; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
eT&
Mat<eT>::at(const uword in_row, const uword in_col)
  {
  return access::rw( mem[in_row + in_col*n_rows] );
  }



//! element accessor; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::at(const uword in_row, const uword in_col) const
  {
  return mem[in_row + in_col*n_rows];
  }



//! prefix ++
template<typename eT>
arma_inline
const Mat<eT>&
Mat<eT>::operator++()
  {
  Mat_aux::prefix_pp(*this);
  return *this;
  }



//! postfix ++  (must not return the object by reference)
template<typename eT>
arma_inline
void
Mat<eT>::operator++(int)
  {
  Mat_aux::postfix_pp(*this);
  }



//! prefix --
template<typename eT>
arma_inline
const Mat<eT>&
Mat<eT>::operator--()
  {
  Mat_aux::prefix_mm(*this);
  return *this;
  }



//! postfix --  (must not return the object by reference)
template<typename eT>
arma_inline
void
Mat<eT>::operator--(int)
  {
  Mat_aux::postfix_mm(*this);
  }



//! returns true if the matrix has no elements
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::is_empty() const
  {
  return (n_elem == 0);
  }



//! returns true if the object can be interpreted as a column or row vector
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::is_vec() const
  {
  return ( (n_rows == 1) || (n_cols == 1) );
  }



//! returns true if the object can be interpreted as a row vector
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::is_rowvec() const
  {
  return (n_rows == 1);
  }



//! returns true if the object can be interpreted as a column vector
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::is_colvec() const
  {
  return (n_cols == 1);
  }



//! returns true if the object has the same number of non-zero rows and columnns
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::is_square() const
  {
  return (n_rows == n_cols);
  }



//! returns true if all of the elements are finite
template<typename eT>
inline
arma_warn_unused
bool
Mat<eT>::is_finite() const
  {
  return arrayops::is_finite( memptr(), n_elem );
  }



template<typename eT>
inline
arma_warn_unused
bool
Mat<eT>::has_inf() const
  {
  arma_extra_debug_sigprint();

  return arrayops::has_inf(memptr(), n_elem);
  }



template<typename eT>
inline
arma_warn_unused
bool
Mat<eT>::has_nan() const
  {
  arma_extra_debug_sigprint();

  return arrayops::has_nan(memptr(), n_elem);
  }



template<typename eT>
inline
arma_warn_unused
bool
Mat<eT>::is_sorted(const char* direction) const
  {
  arma_extra_debug_sigprint();

  return (*this).is_sorted(direction, (((vec_state == 2) || (n_rows == 1)) ? uword(1) : uword(0)));
  }



template<typename eT>
inline
arma_warn_unused
bool
Mat<eT>::is_sorted(const char* direction, const uword dim) const
  {
  arma_extra_debug_sigprint();

  const char sig = (direction != NULL) ? direction[0] : char(0);

  arma_debug_check( ((sig != 'a') && (sig != 'd')), "Mat::is_sorted(): unknown sort direction" );

  arma_debug_check( (dim > 1), "Mat::is_sorted(): parameter 'dim' must be 0 or 1" );

  if(n_elem <= 1)  { return true; }

  const uword local_n_cols = n_cols;
  const uword local_n_rows = n_rows;

  if(sig == 'a')
    {
    // deliberately using the opposite direction comparator,
    // as we need to handle the case of two elements being equal

    arma_descend_sort_helper<eT> comparator;

    if(dim == 0)
      {
      if(local_n_rows <= 1u)  { return true; }

      const uword local_n_rows_m1 = local_n_rows - 1;

      for(uword c=0; c < local_n_cols; ++c)
        {
        const eT* coldata = colptr(c);

        for(uword r=0; r < local_n_rows_m1; ++r)
          {
          const eT val1 = (*coldata); coldata++;
          const eT val2 = (*coldata);

          if(comparator(val1,val2))  { return false; }
          }
        }
      }
    else  // dim == 1
      {
      if(local_n_cols <= 1u)  { return true; }

      const uword local_n_cols_m1 = local_n_cols - 1;

      if(local_n_rows == 1)
        {
        const eT* rowdata = memptr();

        for(uword c=0; c < local_n_cols_m1; ++c)
          {
          const eT val1 = (*rowdata);  rowdata++;
          const eT val2 = (*rowdata);

          if(comparator(val1,val2))  { return false; }
          }
        }
      else
        {
        for(uword r=0; r < local_n_rows;    ++r)
        for(uword c=0; c < local_n_cols_m1; ++c)
          {
          const eT val1 = at(r,c  );
          const eT val2 = at(r,c+1);

          if(comparator(val1,val2))  { return false; }
          }
        }
      }
    }
  else
  if(sig == 'd')
    {
    // deliberately using the opposite direction comparator,
    // as we need to handle the case of two elements being equal

    arma_ascend_sort_helper<eT> comparator;

    if(dim == 0)
      {
      if(local_n_rows <= 1u)  { return true; }

      const uword local_n_rows_m1 = local_n_rows - 1;

      for(uword c=0; c < local_n_cols; ++c)
        {
        const eT* coldata = colptr(c);

        for(uword r=0; r < local_n_rows_m1; ++r)
          {
          const eT val1 = (*coldata); coldata++;
          const eT val2 = (*coldata);

          if(comparator(val1,val2))  { return false; }
          }
        }
      }
    else  // dim == 1
      {
      if(local_n_cols <= 1u)  { return true; }

      const uword local_n_cols_m1 = local_n_cols - 1;

      if(local_n_rows == 1)
        {
        const eT* rowdata = memptr();

        for(uword c=0; c < local_n_cols_m1; ++c)
          {
          const eT val1 = (*rowdata);  rowdata++;
          const eT val2 = (*rowdata);

          if(comparator(val1,val2))  { return false; }
          }
        }
      else
        {
        for(uword r=0; r < local_n_rows;    ++r)
        for(uword c=0; c < local_n_cols_m1; ++c)
          {
          const eT val1 = at(r,c  );
          const eT val2 = at(r,c+1);

          if(comparator(val1,val2))  { return false; }
          }
        }
      }
    }

  return true;
  }



//! returns true if the given index is currently in range
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const uword ii) const
  {
  return (ii < n_elem);
  }



//! returns true if the given start and end indices are currently in range
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const span& x) const
  {
  arma_extra_debug_sigprint();

  if(x.whole == true)
    {
    return true;
    }
  else
    {
    const uword a = x.a;
    const uword b = x.b;

    return ( (a <= b) && (b < n_elem) );
    }
  }



//! returns true if the given location is currently in range
template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const uword in_row, const uword in_col) const
  {
  return ( (in_row < n_rows) && (in_col < n_cols) );
  }



template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const span& row_span, const uword in_col) const
  {
  arma_extra_debug_sigprint();

  if(row_span.whole == true)
    {
    return (in_col < n_cols);
    }
  else
    {
    const uword in_row1 = row_span.a;
    const uword in_row2 = row_span.b;

    return ( (in_row1 <= in_row2) && (in_row2 < n_rows) && (in_col < n_cols) );
    }
  }



template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const uword in_row, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  if(col_span.whole == true)
    {
    return (in_row < n_rows);
    }
  else
    {
    const uword in_col1 = col_span.a;
    const uword in_col2 = col_span.b;

    return ( (in_row < n_rows) && (in_col1 <= in_col2) && (in_col2 < n_cols) );
    }
  }



template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const uword in_row1 = row_span.a;
  const uword in_row2 = row_span.b;

  const uword in_col1 = col_span.a;
  const uword in_col2 = col_span.b;

  const bool rows_ok = row_span.whole ? true : ( (in_row1 <= in_row2) && (in_row2 < n_rows) );
  const bool cols_ok = col_span.whole ? true : ( (in_col1 <= in_col2) && (in_col2 < n_cols) );

  return ( (rows_ok == true) && (cols_ok == true) );
  }



template<typename eT>
arma_inline
arma_warn_unused
bool
Mat<eT>::in_range(const uword in_row, const uword in_col, const SizeMat& s) const
  {
  const uword l_n_rows = n_rows;
  const uword l_n_cols = n_cols;

  if( (in_row >= l_n_rows) || (in_col >= l_n_cols) || ((in_row + s.n_rows) > l_n_rows) || ((in_col + s.n_cols) > l_n_cols) )
    {
    return false;
    }
  else
    {
    return true;
    }
  }



//! returns a pointer to array of eTs for a specified column; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
eT*
Mat<eT>::colptr(const uword in_col)
  {
  return & access::rw(mem[in_col*n_rows]);
  }



//! returns a pointer to array of eTs for a specified column; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT*
Mat<eT>::colptr(const uword in_col) const
  {
  return & mem[in_col*n_rows];
  }



//! returns a pointer to array of eTs used by the matrix
template<typename eT>
arma_inline
arma_warn_unused
eT*
Mat<eT>::memptr()
  {
  return const_cast<eT*>(mem);
  }



//! returns a pointer to array of eTs used by the matrix
template<typename eT>
arma_inline
arma_warn_unused
const eT*
Mat<eT>::memptr() const
  {
  return mem;
  }



//! print contents of the matrix (to the cout stream),
//! optionally preceding with a user specified line of text.
//! the precision and cell width are modified.
//! on return, the stream's state are restored to their original values.
template<typename eT>
inline
void
Mat<eT>::impl_print(const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    const std::streamsize orig_width = get_cout_stream().width();

    get_cout_stream() << extra_text << '\n';

    get_cout_stream().width(orig_width);
    }

  arma_ostream::print(get_cout_stream(), *this, true);
  }



//! print contents of the matrix to a user specified stream,
//! optionally preceding with a user specified line of text.
//! the precision and cell width are modified.
//! on return, the stream's state are restored to their original values.
template<typename eT>
inline
void
Mat<eT>::impl_print(std::ostream& user_stream, const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    const std::streamsize orig_width = user_stream.width();

    user_stream << extra_text << '\n';

    user_stream.width(orig_width);
    }

  arma_ostream::print(user_stream, *this, true);
  }



//! print contents of the matrix (to the cout stream),
//! optionally preceding with a user specified line of text.
//! the stream's state are used as is and are not modified
//! (i.e. the precision and cell width are not modified).
template<typename eT>
inline
void
Mat<eT>::impl_raw_print(const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    const std::streamsize orig_width = get_cout_stream().width();

    get_cout_stream() << extra_text << '\n';

    get_cout_stream().width(orig_width);
    }

  arma_ostream::print(get_cout_stream(), *this, false);
  }



//! print contents of the matrix to a user specified stream,
//! optionally preceding with a user specified line of text.
//! the stream's state are used as is and are not modified.
//! (i.e. the precision and cell width are not modified).
template<typename eT>
inline
void
Mat<eT>::impl_raw_print(std::ostream& user_stream, const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    const std::streamsize orig_width = user_stream.width();

    user_stream << extra_text << '\n';

    user_stream.width(orig_width);
    }

  arma_ostream::print(user_stream, *this, false);
  }



//! change the matrix to have user specified dimensions (data is not preserved)
template<typename eT>
inline
void
Mat<eT>::set_size(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  switch(vec_state)
    {
    case 0:
    case 1:
      init_warm(in_elem, 1);
      break;

    case 2:
      init_warm(1, in_elem);
      break;

    default:
      ;
    }
  }



//! change the matrix to have user specified dimensions (data is not preserved)
template<typename eT>
inline
void
Mat<eT>::set_size(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  init_warm(in_rows, in_cols);
  }



template<typename eT>
inline
void
Mat<eT>::set_size(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  init_warm(s.n_rows, s.n_cols);
  }



//! change the matrix to have user specified dimensions (data is preserved)
template<typename eT>
inline
void
Mat<eT>::resize(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  switch(vec_state)
    {
    case 0:
    case 1:
      (*this).resize(in_elem, 1);
      break;

    case 2:
      (*this).resize(1, in_elem);
      break;

    default:
      ;
    }
  }



//! change the matrix to have user specified dimensions (data is preserved)
template<typename eT>
inline
void
Mat<eT>::resize(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  *this = arma::resize(*this, in_rows, in_cols);
  }



template<typename eT>
inline
void
Mat<eT>::resize(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  *this = arma::resize(*this, s.n_rows, s.n_cols);
  }



//! change the matrix to have user specified dimensions (data is preserved)
template<typename eT>
inline
void
Mat<eT>::reshape(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, in_rows, in_cols);
  }



template<typename eT>
inline
void
Mat<eT>::reshape(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, s.n_rows, s.n_cols);
  }



//! NOTE: don't use this form; it's deprecated and will be removed
template<typename eT>
arma_deprecated
inline
void
Mat<eT>::reshape(const uword in_rows, const uword in_cols, const uword dim)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, in_rows, in_cols, dim);
  }



//! change the matrix (without preserving data) to have the same dimensions as the given expression
template<typename eT>
template<typename eT2, typename expr>
inline
void
Mat<eT>::copy_size(const Base<eT2, expr>& X)
  {
  arma_extra_debug_sigprint();

  const Proxy<expr> P(X.get_ref());

  const uword X_n_rows = P.get_n_rows();
  const uword X_n_cols = P.get_n_cols();

  init_warm(X_n_rows, X_n_cols);
  }



//! apply a functor to each element
template<typename eT>
template<typename functor>
inline
const Mat<eT>&
Mat<eT>::for_each(functor F)
  {
  arma_extra_debug_sigprint();

  eT* data = memptr();

  const uword N = n_elem;

  uword ii, jj;

  for(ii=0, jj=1; jj < N; ii+=2, jj+=2)
    {
    F(data[ii]);
    F(data[jj]);
    }

  if(ii < N)
    {
    F(data[ii]);
    }

  return *this;
  }



template<typename eT>
template<typename functor>
inline
const Mat<eT>&
Mat<eT>::for_each(functor F) const
  {
  arma_extra_debug_sigprint();

  const eT* data = memptr();

  const uword N = n_elem;

  uword ii, jj;

  for(ii=0, jj=1; jj < N; ii+=2, jj+=2)
    {
    F(data[ii]);
    F(data[jj]);
    }

  if(ii < N)
    {
    F(data[ii]);
    }

  return *this;
  }



//! transform each element in the matrix using a functor
template<typename eT>
template<typename functor>
inline
const Mat<eT>&
Mat<eT>::transform(functor F)
  {
  arma_extra_debug_sigprint();

  eT* out_mem = memptr();

  const uword N = n_elem;

  uword ii, jj;

  for(ii=0, jj=1; jj < N; ii+=2, jj+=2)
    {
    eT tmp_ii = out_mem[ii];
    eT tmp_jj = out_mem[jj];

    tmp_ii = eT( F(tmp_ii) );
    tmp_jj = eT( F(tmp_jj) );

    out_mem[ii] = tmp_ii;
    out_mem[jj] = tmp_jj;
    }

  if(ii < N)
    {
    out_mem[ii] = eT( F(out_mem[ii]) );
    }

  return *this;
  }



//! imbue (fill) the matrix with values provided by a functor
template<typename eT>
template<typename functor>
inline
const Mat<eT>&
Mat<eT>::imbue(functor F)
  {
  arma_extra_debug_sigprint();

  eT* out_mem = memptr();

  const uword N = n_elem;

  uword ii, jj;

  for(ii=0, jj=1; jj < N; ii+=2, jj+=2)
    {
    const eT tmp_ii = eT( F() );
    const eT tmp_jj = eT( F() );

    out_mem[ii] = tmp_ii;
    out_mem[jj] = tmp_jj;
    }

  if(ii < N)
    {
    out_mem[ii] = eT( F() );
    }

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  arrayops::replace(memptr(), n_elem, old_val, new_val);

  return *this;
  }



//! fill the matrix with the specified value
template<typename eT>
arma_hot
inline
const Mat<eT>&
Mat<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_set( memptr(), val, n_elem );

  return *this;
  }



//! fill the matrix with the specified value
template<typename eT>
template<typename fill_type>
arma_hot
inline
const Mat<eT>&
Mat<eT>::fill(const fill::fill_class<fill_type>&)
  {
  arma_extra_debug_sigprint();

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_eye  >::yes)  (*this).eye();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  arrayops::fill_zeros(memptr(), n_elem);

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::zeros(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  set_size(in_elem);

  return (*this).zeros();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::zeros(const uword in_n_rows, const uword in_n_cols)
  {
  arma_extra_debug_sigprint();

  set_size(in_n_rows, in_n_cols);

  return (*this).zeros();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::zeros(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).zeros(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::ones()
  {
  arma_extra_debug_sigprint();

  return fill(eT(1));
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::ones(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  set_size(in_elem);

  return fill(eT(1));
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::ones(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols);

  return fill(eT(1));
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::ones(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).ones(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randu()
  {
  arma_extra_debug_sigprint();

  arma_rng::randu<eT>::fill( memptr(), n_elem );

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randu(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  set_size(in_elem);

  return (*this).randu();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randu(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols);

  return (*this).randu();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randu(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).randu(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randn()
  {
  arma_extra_debug_sigprint();

  arma_rng::randn<eT>::fill( memptr(), n_elem );

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randn(const uword in_elem)
  {
  arma_extra_debug_sigprint();

  set_size(in_elem);

  return (*this).randn();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randn(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols);

  return (*this).randn();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::randn(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).randn(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::eye()
  {
  arma_extra_debug_sigprint();

  (*this).zeros();

  const uword N = (std::min)(n_rows, n_cols);

  for(uword ii=0; ii<N; ++ii)
    {
    at(ii,ii) = eT(1);
    }

  return *this;
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::eye(const uword in_rows, const uword in_cols)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols);

  return (*this).eye();
  }



template<typename eT>
inline
const Mat<eT>&
Mat<eT>::eye(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  return (*this).eye(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
void
Mat<eT>::reset()
  {
  arma_extra_debug_sigprint();

  switch(vec_state)
    {
    default:
      init_warm(0, 0);
      break;

    case 1:
      init_warm(0, 1);
      break;

    case 2:
      init_warm(1, 0);
      break;
    }
  }



template<typename eT>
inline
void
Mat<eT>::soft_reset()
  {
  arma_extra_debug_sigprint();

  // don't change the size if the matrix has a fixed size or is a cube slice
  if(mem_state <= 1)
    {
    reset();
    }
  else
    {
    fill(Datum<eT>::nan);
    }
  }



template<typename eT>
template<typename T1>
inline
void
Mat<eT>::set_real(const Base<typename Mat<eT>::pod_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat_aux::set_real(*this, X);
  }



template<typename eT>
template<typename T1>
inline
void
Mat<eT>::set_imag(const Base<typename Mat<eT>::pod_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat_aux::set_imag(*this, X);
  }



template<typename eT>
inline
arma_warn_unused
eT
Mat<eT>::min() const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::min(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_min::direct_min(memptr(), n_elem);
  }



template<typename eT>
inline
arma_warn_unused
eT
Mat<eT>::max() const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::max(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_max::direct_max(memptr(), n_elem);
  }



template<typename eT>
inline
eT
Mat<eT>::min(uword& index_of_min_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::min(): object has no elements");

    index_of_min_val = uword(0);

    return Datum<eT>::nan;
    }

  return op_min::direct_min(memptr(), n_elem, index_of_min_val);
  }



template<typename eT>
inline
eT
Mat<eT>::max(uword& index_of_max_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::max(): object has no elements");

    index_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  return op_max::direct_max(memptr(), n_elem, index_of_max_val);
  }



template<typename eT>
inline
eT
Mat<eT>::min(uword& row_of_min_val, uword& col_of_min_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::min(): object has no elements");

    row_of_min_val = uword(0);
    col_of_min_val = uword(0);

    return Datum<eT>::nan;
    }

  uword iq;

  eT val = op_min::direct_min(memptr(), n_elem, iq);

  row_of_min_val = iq % n_rows;
  col_of_min_val = iq / n_rows;

  return val;
  }



template<typename eT>
inline
eT
Mat<eT>::max(uword& row_of_max_val, uword& col_of_max_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Mat::max(): object has no elements");

    row_of_max_val = uword(0);
    col_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  uword iq;

  eT val = op_max::direct_max(memptr(), n_elem, iq);

  row_of_max_val = iq % n_rows;
  col_of_max_val = iq / n_rows;

  return val;
  }



//! save the matrix to a file
template<typename eT>
inline
bool
Mat<eT>::save(const std::string name, const file_type type, const bool print_status) const
  {
  arma_extra_debug_sigprint();

  bool save_okay;

  switch(type)
    {
    case raw_ascii:
      save_okay = diskio::save_raw_ascii(*this, name);
      break;

    case arma_ascii:
      save_okay = diskio::save_arma_ascii(*this, name);
      break;

    case csv_ascii:
      save_okay = diskio::save_csv_ascii(*this, name);
      break;

    case raw_binary:
      save_okay = diskio::save_raw_binary(*this, name);
      break;

    case arma_binary:
      save_okay = diskio::save_arma_binary(*this, name);
      break;

    case pgm_binary:
      save_okay = diskio::save_pgm_binary(*this, name);
      break;

    case hdf5_binary:
      save_okay = diskio::save_hdf5_binary(*this, hdf5_name(name));
      break;

    case hdf5_binary_trans:
      {
      Mat<eT> tmp;

      op_strans::apply_mat_noalias(tmp, *this);

      save_okay = diskio::save_hdf5_binary(tmp, hdf5_name(name));
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Mat::save(): couldn't write to ", name); }

  return save_okay;
  }



template<typename eT>
inline
bool
Mat<eT>::save(const hdf5_name& spec, const file_type type, const bool print_status) const
  {
  arma_extra_debug_sigprint();

  bool save_okay;

  switch(type)
    {
    case hdf5_binary:
      save_okay = diskio::save_hdf5_binary(*this, spec);
      break;

    case hdf5_binary_trans:
      {
      Mat<eT> tmp;

      op_strans::apply_mat_noalias(tmp, *this);

      save_okay = diskio::save_hdf5_binary(tmp, spec);
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Mat::save(): couldn't write to ", spec.filename); }

  return save_okay;
  }



//! save the matrix to a stream
template<typename eT>
inline
bool
Mat<eT>::save(std::ostream& os, const file_type type, const bool print_status) const
  {
  arma_extra_debug_sigprint();

  bool save_okay;

  switch(type)
    {
    case raw_ascii:
      save_okay = diskio::save_raw_ascii(*this, os);
      break;

    case arma_ascii:
      save_okay = diskio::save_arma_ascii(*this, os);
      break;

    case csv_ascii:
      save_okay = diskio::save_csv_ascii(*this, os);
      break;

    case raw_binary:
      save_okay = diskio::save_raw_binary(*this, os);
      break;

    case arma_binary:
      save_okay = diskio::save_arma_binary(*this, os);
      break;

    case pgm_binary:
      save_okay = diskio::save_pgm_binary(*this, os);
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Mat::save(): couldn't write to the given stream"); }

  return save_okay;
  }



//! load a matrix from a file
template<typename eT>
inline
bool
Mat<eT>::load(const std::string name, const file_type type, const bool print_status)
  {
  arma_extra_debug_sigprint();

  bool load_okay;
  std::string err_msg;

  switch(type)
    {
    case auto_detect:
      load_okay = diskio::load_auto_detect(*this, name, err_msg);
      break;

    case raw_ascii:
      load_okay = diskio::load_raw_ascii(*this, name, err_msg);
      break;

    case arma_ascii:
      load_okay = diskio::load_arma_ascii(*this, name, err_msg);
      break;

    case csv_ascii:
      load_okay = diskio::load_csv_ascii(*this, name, err_msg);
      break;

    case raw_binary:
      load_okay = diskio::load_raw_binary(*this, name, err_msg);
      break;

    case arma_binary:
      load_okay = diskio::load_arma_binary(*this, name, err_msg);
      break;

    case pgm_binary:
      load_okay = diskio::load_pgm_binary(*this, name, err_msg);
      break;

    case hdf5_binary:
      load_okay = diskio::load_hdf5_binary(*this, hdf5_name(name), err_msg);
      break;

    case hdf5_binary_trans:
      {
      Mat<eT> tmp;

      load_okay = diskio::load_hdf5_binary(tmp, hdf5_name(name), err_msg);

      if(load_okay)  { op_strans::apply_mat_noalias(*this, tmp); }
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::load(): unsupported file type"); }
      load_okay = false;
    }

  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Mat::load(): ", err_msg, name);
      }
    else
      {
      arma_debug_warn("Mat::load(): couldn't read ", name);
      }
    }

  if(load_okay == false)
    {
    (*this).soft_reset();
    }

  return load_okay;
  }



template<typename eT>
inline
bool
Mat<eT>::load(const hdf5_name& spec, const file_type type, const bool print_status)
  {
  arma_extra_debug_sigprint();

  bool load_okay;
  std::string err_msg;

  switch(type)
    {
    case hdf5_binary:
      load_okay = diskio::load_hdf5_binary(*this, spec, err_msg);
      break;

    case hdf5_binary_trans:
      {
      Mat<eT> tmp;

      load_okay = diskio::load_hdf5_binary(tmp, spec, err_msg);

      if(load_okay)  { op_strans::apply_mat_noalias(*this, tmp); }
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::load(): unsupported file type"); }
      load_okay = false;
    }

  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Mat::load(): ", err_msg, spec.filename);
      }
    else
      {
      arma_debug_warn("Mat::load(): couldn't read ", spec.filename);
      }
    }

  if(load_okay == false)
    {
    (*this).soft_reset();
    }

  return load_okay;
  }



//! load a matrix from a stream
template<typename eT>
inline
bool
Mat<eT>::load(std::istream& is, const file_type type, const bool print_status)
  {
  arma_extra_debug_sigprint();

  bool load_okay;
  std::string err_msg;

  switch(type)
    {
    case auto_detect:
      load_okay = diskio::load_auto_detect(*this, is, err_msg);
      break;

    case raw_ascii:
      load_okay = diskio::load_raw_ascii(*this, is, err_msg);
      break;

    case arma_ascii:
      load_okay = diskio::load_arma_ascii(*this, is, err_msg);
      break;

    case csv_ascii:
      load_okay = diskio::load_csv_ascii(*this, is, err_msg);
      break;

    case raw_binary:
      load_okay = diskio::load_raw_binary(*this, is, err_msg);
      break;

    case arma_binary:
      load_okay = diskio::load_arma_binary(*this, is, err_msg);
      break;

    case pgm_binary:
      load_okay = diskio::load_pgm_binary(*this, is, err_msg);
      break;

    default:
      if(print_status)  { arma_debug_warn("Mat::load(): unsupported file type"); }
      load_okay = false;
    }


  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Mat::load(): ", err_msg, "the given stream");
      }
    else
      {
      arma_debug_warn("Mat::load(): couldn't load from the given stream");
      }
    }

  if(load_okay == false)
    {
    (*this).soft_reset();
    }

  return load_okay;
  }



//! save the matrix to a file, without printing any error messages
template<typename eT>
inline
bool
Mat<eT>::quiet_save(const std::string name, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(name, type, false);
  }



template<typename eT>
inline
bool
Mat<eT>::quiet_save(const hdf5_name& spec, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(spec, type, false);
  }



//! save the matrix to a stream, without printing any error messages
template<typename eT>
inline
bool
Mat<eT>::quiet_save(std::ostream& os, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(os, type, false);
  }



//! load a matrix from a file, without printing any error messages
template<typename eT>
inline
bool
Mat<eT>::quiet_load(const std::string name, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(name, type, false);
  }



template<typename eT>
inline
bool
Mat<eT>::quiet_load(const hdf5_name& spec, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(spec, type, false);
  }



//! load a matrix from a stream, without printing any error messages
template<typename eT>
inline
bool
Mat<eT>::quiet_load(std::istream& is, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(is, type, false);
  }



template<typename eT>
inline
Mat<eT>::row_iterator::row_iterator(Mat<eT>& in_M, const uword in_row)
  : M  (in_M  )
  , row(in_row)
  , col(0     )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
eT&
Mat<eT>::row_iterator::operator*()
  {
  return M.at(row,col);
  }



template<typename eT>
inline
typename Mat<eT>::row_iterator&
Mat<eT>::row_iterator::operator++()
  {
  ++col;

  if(col >= M.n_cols)
    {
    col = 0;
    ++row;
    }

  return *this;
  }



template<typename eT>
inline
void
Mat<eT>::row_iterator::operator++(int)
  {
  operator++();
  }



template<typename eT>
inline
typename Mat<eT>::row_iterator&
Mat<eT>::row_iterator::operator--()
  {
  if(col > 0)
    {
    --col;
    }
  else
    {
    if(row > 0)
      {
      col = M.n_cols - 1;
      --row;
      }
    }

  return *this;
  }



template<typename eT>
inline
void
Mat<eT>::row_iterator::operator--(int)
  {
  operator--();
  }



template<typename eT>
inline
bool
Mat<eT>::row_iterator::operator!=(const typename Mat<eT>::row_iterator& X) const
  {
  return ( (row != X.row) || (col != X.col) ) ? true : false;
  }



template<typename eT>
inline
bool
Mat<eT>::row_iterator::operator==(const typename Mat<eT>::row_iterator& X) const
  {
  return ( (row == X.row) && (col == X.col) ) ? true : false;
  }



template<typename eT>
inline
Mat<eT>::const_row_iterator::const_row_iterator(const Mat<eT>& in_M, const uword in_row)
  : M  (in_M  )
  , row(in_row)
  , col(0     )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Mat<eT>::const_row_iterator::const_row_iterator(const typename Mat<eT>::row_iterator& X)
  : M  (X.M)
  , row(X.row)
  , col(X.col)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
eT
Mat<eT>::const_row_iterator::operator*() const
  {
  return M.at(row,col);
  }



template<typename eT>
inline
typename Mat<eT>::const_row_iterator&
Mat<eT>::const_row_iterator::operator++()
  {
  ++col;

  if(col >= M.n_cols)
    {
    col = 0;
    ++row;
    }

  return *this;
  }



template<typename eT>
inline
void
Mat<eT>::const_row_iterator::operator++(int)
  {
  operator++();
  }



template<typename eT>
inline
typename Mat<eT>::const_row_iterator&
Mat<eT>::const_row_iterator::operator--()
  {
  if(col > 0)
    {
    --col;
    }
  else
    {
    if(row > 0)
      {
      col = M.n_cols - 1;
      --row;
      }
    }

  return *this;
  }



template<typename eT>
inline
void
Mat<eT>::const_row_iterator::operator--(int)
  {
  operator--();
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_iterator::operator!=(const typename Mat<eT>::const_row_iterator& X) const
  {
  return ( (row != X.row) || (col != X.col) ) ? true : false;
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_iterator::operator==(const typename Mat<eT>::const_row_iterator& X) const
  {
  return ( (row == X.row) && (col == X.col) ) ? true : false;
  }



template<typename eT>
inline
Mat<eT>::row_col_iterator::row_col_iterator()
  : M           (NULL)
  , current_pos (NULL)
  , internal_col(0   )
  , internal_row(0   )
  {
  arma_extra_debug_sigprint();
  // Technically this iterator is invalid (it does not point to a real element)
  }



template<typename eT>
inline
Mat<eT>::row_col_iterator::row_col_iterator(const row_col_iterator& in_it)
  : M           (in_it.M           )
  , current_pos (in_it.current_pos )
  , internal_col(in_it.internal_col)
  , internal_row(in_it.internal_row)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Mat<eT>::row_col_iterator::row_col_iterator(Mat<eT>& in_M, const uword in_row, const uword in_col)
  : M           (&in_M                  )
  , current_pos (&in_M.at(in_row,in_col))
  , internal_col(in_col                 )
  , internal_row(in_row                 )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
eT&
Mat<eT>::row_col_iterator::operator*()
  {
  return *current_pos;
  }



template<typename eT>
inline
typename Mat<eT>::row_col_iterator&
Mat<eT>::row_col_iterator::operator++()
  {
  current_pos++;
  internal_row++;

  // Check to see if we moved a column.
  if(internal_row == M->n_rows)
    {
    internal_col++;
    internal_row = 0;
    }

  return *this;
  }



template<typename eT>
inline
typename Mat<eT>::row_col_iterator
Mat<eT>::row_col_iterator::operator++(int)
  {
  typename Mat<eT>::row_col_iterator temp(*this);

  ++(*this);

  return temp;
  }



template<typename eT>
inline typename Mat<eT>::row_col_iterator&
Mat<eT>::row_col_iterator::operator--()
  {
  if(internal_row > 0)
    {
    current_pos--;
    internal_row--;
    }
  else
  if(internal_col > 0)
    {
    current_pos--;
    internal_col--;
    internal_row = M->n_rows - 1;
    }

  return *this;
  }



template<typename eT>
inline
typename Mat<eT>::row_col_iterator
Mat<eT>::row_col_iterator::operator--(int)
  {
  typename Mat<eT>::row_col_iterator temp(*this);

  --(*this);

  return temp;
  }



template<typename eT>
inline
uword
Mat<eT>::row_col_iterator::row() const
  {
  return internal_row;
  }



template<typename eT>
inline
uword
Mat<eT>::row_col_iterator::col() const
  {
  return internal_col;
  }



template<typename eT>
inline
bool
Mat<eT>::row_col_iterator::operator==(const row_col_iterator& rhs) const
  {
  return (current_pos == rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::row_col_iterator::operator!=(const row_col_iterator& rhs) const
  {
  return (current_pos != rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::row_col_iterator::operator==(const const_row_col_iterator& rhs) const
  {
  return (current_pos == rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::row_col_iterator::operator!=(const const_row_col_iterator& rhs) const
  {
  return (current_pos != rhs.current_pos);
  }



template<typename eT>
inline
Mat<eT>::const_row_col_iterator::const_row_col_iterator()
  : M           (NULL)
  , current_pos (NULL)
  , internal_col(0   )
  , internal_row(0   )
  {
  arma_extra_debug_sigprint();
  // Technically this iterator is invalid (it does not point to a real element)
  }



template<typename eT>
inline
Mat<eT>::const_row_col_iterator::const_row_col_iterator(const row_col_iterator& in_it)
  : M           (in_it.M          )
  , current_pos (in_it.current_pos)
  , internal_col(in_it.col()      )
  , internal_row(in_it.row()      )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Mat<eT>::const_row_col_iterator::const_row_col_iterator(const const_row_col_iterator& in_it)
  : M           (in_it.M          )
  , current_pos (in_it.current_pos)
  , internal_col(in_it.col()      )
  , internal_row(in_it.row()      )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Mat<eT>::const_row_col_iterator::const_row_col_iterator(const Mat<eT>& in_M, const uword in_row, const uword in_col)
  : M           (&in_M                  )
  , current_pos (&in_M.at(in_row,in_col))
  , internal_col(in_col                 )
  , internal_row(in_row                 )
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
const eT&
Mat<eT>::const_row_col_iterator::operator*() const
  {
  return *current_pos;
  }



template<typename eT>
inline
typename Mat<eT>::const_row_col_iterator&
Mat<eT>::const_row_col_iterator::operator++()
  {
  current_pos++;
  internal_row++;

  // Check to see if we moved a column.
  if(internal_row == M->n_rows)
    {
    internal_col++;
    internal_row = 0;
    }

  return *this;
  }



template<typename eT>
inline
typename Mat<eT>::const_row_col_iterator
Mat<eT>::const_row_col_iterator::operator++(int)
  {
  typename Mat<eT>::const_row_col_iterator temp(*this);

  ++(*this);

  return temp;
  }



template<typename eT>
inline
typename Mat<eT>::const_row_col_iterator&
Mat<eT>::const_row_col_iterator::operator--()
  {
  if(internal_row > 0)
    {
    current_pos--;
    internal_row--;
    }
  else
  if(internal_col > 0)
    {
    current_pos--;
    internal_col--;
    internal_row = M->n_rows - 1;
    }

  return *this;
  }



template<typename eT>
inline
typename Mat<eT>::const_row_col_iterator
Mat<eT>::const_row_col_iterator::operator--(int)
  {
  typename Mat<eT>::const_row_col_iterator temp(*this);

  --(*this);

  return temp;
  }



template<typename eT>
inline
uword
Mat<eT>::const_row_col_iterator::row() const
  {
  return internal_row;
  }



template<typename eT>
inline
uword
Mat<eT>::const_row_col_iterator::col() const
  {
  return internal_col;
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_col_iterator::operator==(const const_row_col_iterator& rhs) const
  {
  return (current_pos == rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_col_iterator::operator!=(const const_row_col_iterator& rhs) const
  {
  return (current_pos != rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_col_iterator::operator==(const row_col_iterator& rhs) const
  {
  return (current_pos == rhs.current_pos);
  }



template<typename eT>
inline
bool
Mat<eT>::const_row_col_iterator::operator!=(const row_col_iterator& rhs) const
  {
  return (current_pos != rhs.current_pos);
  }



template<typename eT>
inline
typename Mat<eT>::iterator
Mat<eT>::begin()
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Mat<eT>::const_iterator
Mat<eT>::begin() const
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Mat<eT>::const_iterator
Mat<eT>::cbegin() const
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Mat<eT>::iterator
Mat<eT>::end()
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Mat<eT>::const_iterator
Mat<eT>::end() const
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Mat<eT>::const_iterator
Mat<eT>::cend() const
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Mat<eT>::col_iterator
Mat<eT>::begin_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (col_num >= n_cols), "Mat::begin_col(): index out of bounds");

  return colptr(col_num);
  }



template<typename eT>
inline
typename Mat<eT>::const_col_iterator
Mat<eT>::begin_col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (col_num >= n_cols), "Mat::begin_col(): index out of bounds");

  return colptr(col_num);
  }



template<typename eT>
inline
typename Mat<eT>::col_iterator
Mat<eT>::end_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (col_num >= n_cols), "Mat::end_col(): index out of bounds");

  return colptr(col_num) + n_rows;
  }



template<typename eT>
inline
typename Mat<eT>::const_col_iterator
Mat<eT>::end_col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (col_num >= n_cols), "Mat::end_col(): index out of bounds");

  return colptr(col_num) + n_rows;
  }



template<typename eT>
inline
typename Mat<eT>::row_iterator
Mat<eT>::begin_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= n_rows), "Mat::begin_row(): index out of bounds" );

  return typename Mat<eT>::row_iterator(*this, row_num);
  }



template<typename eT>
inline
typename Mat<eT>::const_row_iterator
Mat<eT>::begin_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= n_rows), "Mat::begin_row(): index out of bounds" );

  return typename Mat<eT>::const_row_iterator(*this, row_num);
  }



template<typename eT>
inline
typename Mat<eT>::row_iterator
Mat<eT>::end_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= n_rows), "Mat::end_row(): index out of bounds" );

  return typename Mat<eT>::row_iterator(*this, row_num + 1);
  }



template<typename eT>
inline
typename Mat<eT>::const_row_iterator
Mat<eT>::end_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= n_rows), "Mat::end_row(): index out of bounds" );

  return typename Mat<eT>::const_row_iterator(*this, row_num + 1);
  }



template<typename eT>
inline
typename Mat<eT>::row_col_iterator
Mat<eT>::begin_row_col()
  {
  return row_col_iterator(*this);
  }



template<typename eT>
inline
typename Mat<eT>::const_row_col_iterator
Mat<eT>::begin_row_col() const
  {
  return const_row_col_iterator(*this);
  }



template<typename eT>
inline typename Mat<eT>::row_col_iterator
Mat<eT>::end_row_col()
  {
  return row_col_iterator(*this, 0, n_cols);
  }



template<typename eT>
inline typename Mat<eT>::const_row_col_iterator
Mat<eT>::end_row_col() const
  {
  return const_row_col_iterator(*this, 0, n_cols);
  }



//! resets this matrix to an empty matrix
template<typename eT>
inline
void
Mat<eT>::clear()
  {
  reset();
  }



//! returns true if the matrix has no elements
template<typename eT>
inline
bool
Mat<eT>::empty() const
  {
  return (n_elem == 0);
  }



//! returns the number of elements in this matrix
template<typename eT>
inline
uword
Mat<eT>::size() const
  {
  return n_elem;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed()
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const fixed<fixed_n_rows, fixed_n_cols>& X)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

        eT* dest = (use_extra) ?   mem_local_extra :   mem_local;
  const eT* src  = (use_extra) ? X.mem_local_extra : X.mem_local;

  arrayops::copy( dest, src, fixed_n_elem );
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
template<typename fill_type>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const fill::fill_class<fill_type>&)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_eye  >::yes)  (*this).eye();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
template<typename T1>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const Base<eT,T1>& A)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Mat<eT>::operator=(A.get_ref());
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
template<typename T1, typename T2>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const Base<pod_type,T1>& A, const Base<pod_type,T2>& B)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Mat<eT>::init(A,B);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const eT* aux_mem)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  eT* dest = (use_extra) ? mem_local_extra : mem_local;

  arrayops::copy( dest, aux_mem, fixed_n_elem );
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const char* text)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Mat<eT>::operator=(text);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
inline
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const std::string& text)
  : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Mat<eT>::operator=(text);
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  inline
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const std::initializer_list<eT>& list)
    : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
    {
    arma_extra_debug_sigprint_this(this);

    (*this).operator=(list);
    }



  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  inline
  Mat<eT>&
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator=(const std::initializer_list<eT>& list)
    {
    arma_extra_debug_sigprint();

    const uword N = uword(list.size());

    arma_debug_check( (N > fixed_n_elem), "Mat::fixed: initialiser list is too long" );

    eT* this_mem = (*this).memptr();

    arrayops::copy( this_mem, list.begin(), N );

    for(uword iq=N; iq < fixed_n_elem; ++iq) { this_mem[iq] = eT(0); }

    return *this;
    }



  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  inline
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fixed(const std::initializer_list< std::initializer_list<eT> >& list)
    : Mat<eT>( arma_fixed_indicator(), fixed_n_rows, fixed_n_cols, 0, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
    {
    arma_extra_debug_sigprint_this(this);

    Mat<eT>::init(list);
    }



  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  inline
  Mat<eT>&
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator=(const std::initializer_list< std::initializer_list<eT> >& list)
    {
    arma_extra_debug_sigprint();

    Mat<eT>::init(list);

    return *this;
    }

#endif



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
Mat<eT>&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator=(const fixed<fixed_n_rows, fixed_n_cols>& X)
  {
  arma_extra_debug_sigprint();

  if(this != &X)
    {
          eT* dest = (use_extra) ?   mem_local_extra :   mem_local;
    const eT* src  = (use_extra) ? X.mem_local_extra : X.mem_local;

    arrayops::copy( dest, src, fixed_n_elem );
    }

  return *this;
  }



#if defined(ARMA_GOOD_COMPILER)

  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  template<typename T1, typename eop_type>
  inline
  Mat<eT>&
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator=(const eOp<T1, eop_type>& X)
    {
    arma_extra_debug_sigprint();

    arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

    const bool bad_alias = (eOp<T1, eop_type>::proxy_type::has_subview  &&  X.P.is_alias(*this));

    if(bad_alias == false)
      {
      arma_debug_assert_same_size(fixed_n_rows, fixed_n_cols, X.get_n_rows(), X.get_n_cols(), "Mat::fixed::operator=");

      eop_type::apply(*this, X);
      }
    else
      {
      arma_extra_debug_print("bad_alias = true");

      Mat<eT> tmp(X);

      (*this) = tmp;
      }

    return *this;
    }



  template<typename eT>
  template<uword fixed_n_rows, uword fixed_n_cols>
  template<typename T1, typename T2, typename eglue_type>
  inline
  Mat<eT>&
  Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator=(const eGlue<T1, T2, eglue_type>& X)
    {
    arma_extra_debug_sigprint();

    arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
    arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

    const bool bad_alias =
      (
      (eGlue<T1, T2, eglue_type>::proxy1_type::has_subview  &&  X.P1.is_alias(*this))
      ||
      (eGlue<T1, T2, eglue_type>::proxy2_type::has_subview  &&  X.P2.is_alias(*this))
      );

    if(bad_alias == false)
      {
      arma_debug_assert_same_size(fixed_n_rows, fixed_n_cols, X.get_n_rows(), X.get_n_cols(), "Mat::fixed::operator=");

      eglue_type::apply(*this, X);
      }
    else
      {
      arma_extra_debug_print("bad_alias = true");

      Mat<eT> tmp(X);

      (*this) = tmp;
      }

    return *this;
    }

#endif



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
const Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_htrans >
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::t() const
  {
  return Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_htrans >(*this);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
const Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_htrans >
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::ht() const
  {
  return Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_htrans >(*this);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
const Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_strans >
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::st() const
  {
  return Op< typename Mat<eT>::template fixed<fixed_n_rows, fixed_n_cols>::Mat_fixed_type, op_strans >(*this);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::at_alt(const uword ii) const
  {
  #if defined(ARMA_HAVE_ALIGNED_ATTRIBUTE)

    return (use_extra) ? mem_local_extra[ii] : mem_local[ii];

  #else
    const eT* mem_aligned = (use_extra) ? mem_local_extra : mem_local;

    memory::mark_as_aligned(mem_aligned);

    return mem_aligned[ii];
  #endif
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator[] (const uword ii)
  {
  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator[] (const uword ii) const
  {
  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::at(const uword ii)
  {
  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::at(const uword ii) const
  {
  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator() (const uword ii)
  {
  arma_debug_check( (ii >= fixed_n_elem), "Mat::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator() (const uword ii) const
  {
  arma_debug_check( (ii >= fixed_n_elem), "Mat::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[ii] : mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::at(const uword in_row, const uword in_col)
  {
  const uword iq = in_row + in_col*fixed_n_rows;

  return (use_extra) ? mem_local_extra[iq] : mem_local[iq];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::at(const uword in_row, const uword in_col) const
  {
  const uword iq = in_row + in_col*fixed_n_rows;

  return (use_extra) ? mem_local_extra[iq] : mem_local[iq];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator() (const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row >= fixed_n_rows) || (in_col >= fixed_n_cols)), "Mat::operator(): index out of bounds");

  const uword iq = in_row + in_col*fixed_n_rows;

  return (use_extra) ? mem_local_extra[iq] : mem_local[iq];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::operator() (const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row >= fixed_n_rows) || (in_col >= fixed_n_cols)), "Mat::operator(): index out of bounds");

  const uword iq = in_row + in_col*fixed_n_rows;

  return (use_extra) ? mem_local_extra[iq] : mem_local[iq];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT*
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::colptr(const uword in_col)
  {
  eT* mem_actual = (use_extra) ? mem_local_extra : mem_local;

  return & access::rw(mem_actual[in_col*fixed_n_rows]);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT*
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::colptr(const uword in_col) const
  {
  const eT* mem_actual = (use_extra) ? mem_local_extra : mem_local;

  return & mem_actual[in_col*fixed_n_rows];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
eT*
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::memptr()
  {
  return (use_extra) ? mem_local_extra : mem_local;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
const eT*
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::memptr() const
  {
  return (use_extra) ? mem_local_extra : mem_local;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_inline
arma_warn_unused
bool
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::is_vec() const
  {
  return ( (fixed_n_rows == 1) || (fixed_n_cols == 1) );
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_hot
inline
const Mat<eT>&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, val );

  return *this;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_hot
inline
const Mat<eT>&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::zeros()
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, eT(0) );

  return *this;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols>
arma_hot
inline
const Mat<eT>&
Mat<eT>::fixed<fixed_n_rows, fixed_n_cols>::ones()
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, eT(1) );

  return *this;
  }



//! prefix ++
template<typename eT>
inline
void
Mat_aux::prefix_pp(Mat<eT>& x)
  {
        eT*   memptr = x.memptr();
  const uword n_elem = x.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    ++(memptr[i]);
    ++(memptr[j]);
    }

  if(i < n_elem)
    {
    ++(memptr[i]);
    }
  }



//! prefix ++ for complex numbers (work around for limitations of the std::complex class)
template<typename T>
inline
void
Mat_aux::prefix_pp(Mat< std::complex<T> >& x)
  {
  x += T(1);
  }



//! postfix ++
template<typename eT>
inline
void
Mat_aux::postfix_pp(Mat<eT>& x)
  {
        eT*   memptr = x.memptr();
  const uword n_elem = x.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    (memptr[i])++;
    (memptr[j])++;
    }

  if(i < n_elem)
    {
    (memptr[i])++;
    }
  }



//! postfix ++ for complex numbers (work around for limitations of the std::complex class)
template<typename T>
inline
void
Mat_aux::postfix_pp(Mat< std::complex<T> >& x)
  {
  x += T(1);
  }



//! prefix --
template<typename eT>
inline
void
Mat_aux::prefix_mm(Mat<eT>& x)
  {
        eT*   memptr = x.memptr();
  const uword n_elem = x.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    --(memptr[i]);
    --(memptr[j]);
    }

  if(i < n_elem)
    {
    --(memptr[i]);
    }
  }



//! prefix -- for complex numbers (work around for limitations of the std::complex class)
template<typename T>
inline
void
Mat_aux::prefix_mm(Mat< std::complex<T> >& x)
  {
  x -= T(1);
  }



//! postfix --
template<typename eT>
inline
void
Mat_aux::postfix_mm(Mat<eT>& x)
  {
        eT*   memptr = x.memptr();
  const uword n_elem = x.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    (memptr[i])--;
    (memptr[j])--;
    }

  if(i < n_elem)
    {
    (memptr[i])--;
    }
  }



//! postfix ++ for complex numbers (work around for limitations of the std::complex class)
template<typename T>
inline
void
Mat_aux::postfix_mm(Mat< std::complex<T> >& x)
  {
  x -= T(1);
  }



template<typename eT, typename T1>
inline
void
Mat_aux::set_real(Mat<eT>& out, const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& A = tmp.M;

  arma_debug_assert_same_size( out, A, "Mat::set_real()" );

  out = A;
  }



template<typename eT, typename T1>
inline
void
Mat_aux::set_imag(Mat<eT>&, const Base<eT,T1>&)
  {
  arma_extra_debug_sigprint();
  }



template<typename T, typename T1>
inline
void
Mat_aux::set_real(Mat< std::complex<T> >& out, const Base<T,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  const Proxy<T1> P(X.get_ref());

  const uword local_n_rows = P.get_n_rows();
  const uword local_n_cols = P.get_n_cols();

  arma_debug_assert_same_size( out.n_rows, out.n_cols, local_n_rows, local_n_cols, "Mat::set_real()" );

  eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    const uword N = out.n_elem;

    for(uword i=0; i<N; ++i)
      {
      out_mem[i] = std::complex<T>( A[i], out_mem[i].imag() );
      }
    }
  else
    {
    for(uword col=0; col < local_n_cols; ++col)
    for(uword row=0; row < local_n_rows; ++row)
      {
      (*out_mem) = std::complex<T>( P.at(row,col), (*out_mem).imag() );
      out_mem++;
      }
    }
  }



template<typename T, typename T1>
inline
void
Mat_aux::set_imag(Mat< std::complex<T> >& out, const Base<T,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  const Proxy<T1> P(X.get_ref());

  const uword local_n_rows = P.get_n_rows();
  const uword local_n_cols = P.get_n_cols();

  arma_debug_assert_same_size( out.n_rows, out.n_cols, local_n_rows, local_n_cols, "Mat::set_imag()" );

  eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    const uword N = out.n_elem;

    for(uword i=0; i<N; ++i)
      {
      out_mem[i] = std::complex<T>( out_mem[i].real(), A[i] );
      }
    }
  else
    {
    for(uword col=0; col < local_n_cols; ++col)
    for(uword row=0; row < local_n_rows; ++row)
      {
      (*out_mem) = std::complex<T>( (*out_mem).real(), P.at(row,col) );
      out_mem++;
      }
    }
  }



#ifdef ARMA_EXTRA_MAT_MEAT
  #include ARMA_INCFILE_WRAP(ARMA_EXTRA_MAT_MEAT)
#endif



//! @}
