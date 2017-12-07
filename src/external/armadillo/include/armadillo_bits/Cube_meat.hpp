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


//! \addtogroup Cube
//! @{


template<typename eT>
inline
Cube<eT>::~Cube()
  {
  arma_extra_debug_sigprint_this(this);

  delete_mat();

  if( (mem_state == 0) && (n_elem > Cube_prealloc::mem_n_elem) )
    {
    memory::release( access::rw(mem) );
    }

  // try to expose buggy user code that accesses deleted objects
  if(arma_config::debug)
    {
    access::rw(mem)      = 0;
    access::rw(mat_ptrs) = 0;
    }

  arma_type_check(( is_supported_elem_type<eT>::value == false ));
  }



template<typename eT>
inline
Cube<eT>::Cube()
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);
  }



//! construct the cube to have user specified dimensions
template<typename eT>
inline
Cube<eT>::Cube(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem_slice(in_n_rows*in_n_cols)
  , n_slices(in_n_slices)
  , n_elem(in_n_rows*in_n_cols*in_n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();
  }



template<typename eT>
inline
Cube<eT>::Cube(const SizeCube& s)
  : n_rows(s.n_rows)
  , n_cols(s.n_cols)
  , n_elem_slice(s.n_rows*s.n_cols)
  , n_slices(s.n_slices)
  , n_elem(s.n_rows*s.n_cols*s.n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();
  }



//! construct the cube to have user specified dimensions and fill with specified pattern
template<typename eT>
template<typename fill_type>
inline
Cube<eT>::Cube(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices, const fill::fill_class<fill_type>&)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem_slice(in_n_rows*in_n_cols)
  , n_slices(in_n_slices)
  , n_elem(in_n_rows*in_n_cols*in_n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();

  if(is_same_type<fill_type, fill::fill_eye  >::yes)  { arma_debug_check(true, "Cube::Cube(): unsupported fill type"); }
  }



template<typename eT>
template<typename fill_type>
inline
Cube<eT>::Cube(const SizeCube& s, const fill::fill_class<fill_type>&)
  : n_rows(s.n_rows)
  , n_cols(s.n_cols)
  , n_elem_slice(s.n_rows*s.n_cols)
  , n_slices(s.n_slices)
  , n_elem(s.n_rows*s.n_cols*s.n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();

  if(is_same_type<fill_type, fill::fill_eye  >::yes)  { arma_debug_check(true, "Cube::Cube(): unsupported fill type"); }
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  inline
  Cube<eT>::Cube(Cube<eT>&& in_cube)
    : n_rows(0)
    , n_cols(0)
    , n_elem_slice(0)
    , n_slices(0)
    , n_elem(0)
    , mem_state(0)
    , mem()
    , mat_ptrs(0)
    {
    arma_extra_debug_sigprint_this(this);
    arma_extra_debug_sigprint(arma_str::format("this = %x   in_cube = %x") % this % &in_cube);

    (*this).steal_mem(in_cube);
    }



  template<typename eT>
  inline
  Cube<eT>&
  Cube<eT>::operator=(Cube<eT>&& in_cube)
    {
    arma_extra_debug_sigprint(arma_str::format("this = %x   in_cube = %x") % this % &in_cube);

    (*this).steal_mem(in_cube);

    return *this;
    }

#endif



template<typename eT>
inline
void
Cube<eT>::init_cold()
  {
  arma_extra_debug_sigprint( arma_str::format("n_rows = %d, n_cols = %d, n_slices = %d") % n_rows % n_cols % n_slices );

  #if (defined(ARMA_USE_CXX11) || defined(ARMA_64BIT_WORD))
    const char* error_message = "Cube::init(): requested size is too large";
  #else
    const char* error_message = "Cube::init(): requested size is too large; suggest to compile in C++11 mode or enable ARMA_64BIT_WORD";
  #endif

  arma_debug_check
    (
      (
      ( (n_rows > 0x0FFF) || (n_cols > 0x0FFF) || (n_slices > 0xFF) )
        ? ( (double(n_rows) * double(n_cols) * double(n_slices)) > double(ARMA_MAX_UWORD) )
        : false
      ),
    error_message
    );


  if(n_elem <= Cube_prealloc::mem_n_elem)
    {
    if(n_elem == 0)
      {
      access::rw(mem) = NULL;
      }
    else
      {
      arma_extra_debug_print("Cube::init(): using local memory");
      access::rw(mem) = mem_local;
      }
    }
  else
    {
    arma_extra_debug_print("Cube::init(): acquiring memory");
    access::rw(mem) = memory::acquire<eT>(n_elem);
    }

  create_mat();
  }



template<typename eT>
inline
void
Cube<eT>::init_warm(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices)
  {
  arma_extra_debug_sigprint( arma_str::format("in_n_rows = %d, in_n_cols = %d, in_n_slices = %d") % in_n_rows % in_n_cols % in_n_slices );

  if( (n_rows == in_n_rows) && (n_cols == in_n_cols) && (n_slices == in_n_slices) )  { return; }

  const uword t_mem_state = mem_state;

  bool  err_state = false;
  char* err_msg   = 0;

  arma_debug_set_error( err_state, err_msg, (t_mem_state == 3), "Cube::init(): size is fixed and hence cannot be changed" );

  #if (defined(ARMA_USE_CXX11) || defined(ARMA_64BIT_WORD))
    const char* error_message = "Cube::init(): requested size is too large";
  #else
    const char* error_message = "Cube::init(): requested size is too large; suggest to compile in C++11 mode or enable ARMA_64BIT_WORD";
  #endif

  arma_debug_set_error
    (
    err_state,
    err_msg,
      (
      ( (in_n_rows > 0x0FFF) || (in_n_cols > 0x0FFF) || (in_n_slices > 0xFF) )
        ? ( (double(in_n_rows) * double(in_n_cols) * double(in_n_slices)) > double(ARMA_MAX_UWORD) )
        : false
      ),
    error_message
    );

  arma_debug_check(err_state, err_msg);

  const uword old_n_elem = n_elem;
  const uword new_n_elem = in_n_rows * in_n_cols * in_n_slices;

  if(old_n_elem == new_n_elem)
    {
    arma_extra_debug_print("Cube::init(): reusing memory");

    delete_mat();

    access::rw(n_rows)       = in_n_rows;
    access::rw(n_cols)       = in_n_cols;
    access::rw(n_elem_slice) = in_n_rows*in_n_cols;
    access::rw(n_slices)     = in_n_slices;

    create_mat();
    }
  else  // condition: old_n_elem != new_n_elem
    {
    arma_debug_check( (t_mem_state == 2), "Cube::init(): requested size is not compatible with the size of auxiliary memory" );

    delete_mat();

    if(new_n_elem < old_n_elem)  // reuse existing memory if possible
      {
      if( (t_mem_state == 0) && (new_n_elem <= Cube_prealloc::mem_n_elem) )
        {
        if(old_n_elem > Cube_prealloc::mem_n_elem)
          {
          arma_extra_debug_print("Cube::init(): releasing memory");
          memory::release( access::rw(mem) );
          }

        if(new_n_elem == 0)
          {
          access::rw(mem) = NULL;
          }
        else
          {
          arma_extra_debug_print("Cube::init(): using local memory");
          access::rw(mem) = mem_local;
          }
        }
      else
        {
        arma_extra_debug_print("Cube::init(): reusing memory");
        }
      }
    else  // condition: new_n_elem > old_n_elem
      {
      if( (t_mem_state == 0) && (old_n_elem > Cube_prealloc::mem_n_elem) )
        {
        arma_extra_debug_print("Cube::init(): releasing memory");
        memory::release( access::rw(mem) );
        }

      if(new_n_elem <= Cube_prealloc::mem_n_elem)
        {
        arma_extra_debug_print("Cube::init(): using local memory");
        access::rw(mem) = mem_local;
        }
      else
        {
        arma_extra_debug_print("Cube::init(): acquiring memory");
        access::rw(mem) = memory::acquire<eT>(new_n_elem);
        }

      access::rw(mem_state) = 0;
      }

    access::rw(n_rows)       = in_n_rows;
    access::rw(n_cols)       = in_n_cols;
    access::rw(n_elem_slice) = in_n_rows*in_n_cols;
    access::rw(n_slices)     = in_n_slices;
    access::rw(n_elem)       = new_n_elem;

    create_mat();
    }
  }



//! for constructing a complex cube out of two non-complex cubes
template<typename eT>
template<typename T1, typename T2>
inline
void
Cube<eT>::init
  (
  const BaseCube<typename Cube<eT>::pod_type,T1>& X,
  const BaseCube<typename Cube<eT>::pod_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type T;

  arma_type_check(( is_complex<eT>::value == false ));   //!< compile-time abort if eT is not std::complex
  arma_type_check(( is_complex< T>::value == true  ));   //!< compile-time abort if  T is     std::complex

  arma_type_check(( is_same_type< std::complex<T>, eT >::no ));   //!< compile-time abort if types are not compatible

  const ProxyCube<T1> PX(X.get_ref());
  const ProxyCube<T2> PY(Y.get_ref());

  arma_debug_assert_same_size(PX, PY, "Cube()");

  const uword local_n_rows   = PX.get_n_rows();
  const uword local_n_cols   = PX.get_n_cols();
  const uword local_n_slices = PX.get_n_slices();

  init_warm(local_n_rows, local_n_cols, local_n_slices);

  eT* out_mem = (*this).memptr();

  const bool use_at = ( ProxyCube<T1>::use_at || ProxyCube<T2>::use_at );

  if(use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type1;
    typedef typename ProxyCube<T2>::ea_type ea_type2;

    const uword N = n_elem;

    ea_type1 A = PX.get_ea();
    ea_type2 B = PY.get_ea();

    for(uword i=0; i<N; ++i)
      {
      out_mem[i] = std::complex<T>(A[i], B[i]);
      }
    }
  else
    {
    for(uword uslice = 0; uslice < local_n_slices; ++uslice)
    for(uword ucol   = 0;   ucol < local_n_cols;   ++ucol  )
    for(uword urow   = 0;   urow < local_n_rows;   ++urow  )
      {
      *out_mem = std::complex<T>( PX.at(urow,ucol,uslice), PY.at(urow,ucol,uslice) );
      out_mem++;
      }
    }
  }



template<typename eT>
inline
void
Cube<eT>::delete_mat()
  {
  arma_extra_debug_sigprint();

  if((n_slices > 0) && (mat_ptrs != NULL))
    {
    for(uword uslice = 0; uslice < n_slices; ++uslice)
      {
      if(mat_ptrs[uslice] != NULL)  { delete access::rw(mat_ptrs[uslice]); }
      }

    if( (mem_state <= 2) && (n_slices > Cube_prealloc::mat_ptrs_size) )
      {
      delete [] mat_ptrs;
      }
    }
  }



template<typename eT>
inline
void
Cube<eT>::create_mat()
  {
  arma_extra_debug_sigprint();

  if(n_slices == 0)
    {
    access::rw(mat_ptrs) = NULL;
    }
  else
    {
    if(mem_state <= 2)
      {
      if(n_slices <= Cube_prealloc::mat_ptrs_size)
        {
        access::rw(mat_ptrs) = const_cast< const Mat<eT>** >(mat_ptrs_local);
        }
      else
        {
        access::rw(mat_ptrs) = new(std::nothrow) const Mat<eT>*[n_slices];

        arma_check_bad_alloc( (mat_ptrs == 0), "Cube::create_mat(): out of memory" );
        }
      }

    for(uword uslice = 0; uslice < n_slices; ++uslice)
      {
      mat_ptrs[uslice] = NULL;
      }
    }
  }



//! Set the cube to be equal to the specified scalar.
//! NOTE: the size of the cube will be 1x1x1
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  init_warm(1,1,1);
  access::rw(mem[0]) = val;
  return *this;
  }



//! In-place addition of a scalar to all elements of the cube
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator+=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_plus( memptr(), val, n_elem );

  return *this;
  }



//! In-place subtraction of a scalar from all elements of the cube
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator-=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_minus( memptr(), val, n_elem );

  return *this;
  }



//! In-place multiplication of all elements of the cube with a scalar
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator*=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_mul( memptr(), val, n_elem );

  return *this;
  }



//! In-place division of all elements of the cube with a scalar
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator/=(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_div( memptr(), val, n_elem );

  return *this;
  }



//! construct a cube from a given cube
template<typename eT>
inline
Cube<eT>::Cube(const Cube<eT>& x)
  : n_rows(x.n_rows)
  , n_cols(x.n_cols)
  , n_elem_slice(x.n_elem_slice)
  , n_slices(x.n_slices)
  , n_elem(x.n_elem)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);
  arma_extra_debug_sigprint(arma_str::format("this = %x   in_cube = %x") % this % &x);

  init_cold();

  arrayops::copy( memptr(), x.mem, n_elem );
  }



//! construct a cube from a given cube
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator=(const Cube<eT>& x)
  {
  arma_extra_debug_sigprint(arma_str::format("this = %x   in_cube = %x") % this % &x);

  if(this != &x)
    {
    init_warm(x.n_rows, x.n_cols, x.n_slices);

    arrayops::copy( memptr(), x.mem, n_elem );
    }

  return *this;
  }



//! construct a cube from a given auxiliary array of eTs.
//! if copy_aux_mem is true, new memory is allocated and the array is copied.
//! if copy_aux_mem is false, the auxiliary array is used directly (without allocating memory and copying).
template<typename eT>
inline
Cube<eT>::Cube(eT* aux_mem, const uword aux_n_rows, const uword aux_n_cols, const uword aux_n_slices, const bool copy_aux_mem, const bool strict, const bool prealloc_mat)
  : n_rows      ( aux_n_rows                          )
  , n_cols      ( aux_n_cols                          )
  , n_elem_slice( aux_n_rows*aux_n_cols               )
  , n_slices    ( aux_n_slices                        )
  , n_elem      ( aux_n_rows*aux_n_cols*aux_n_slices  )
  , mem_state   ( copy_aux_mem ? 0 : (strict ? 2 : 1) )
  , mem         ( copy_aux_mem ? 0 : aux_mem          )
  , mat_ptrs    ( 0                                   )
  {
  arma_extra_debug_sigprint_this(this);

  if(prealloc_mat == true)  { arma_debug_warn("Cube::Cube(): parameter 'prealloc_mat' ignored as it's no longer used"); }

  if(copy_aux_mem == true)
    {
    init_cold();

    arrayops::copy( memptr(), aux_mem, n_elem );
    }
  else
    {
    create_mat();
    }
  }



//! construct a cube from a given auxiliary read-only array of eTs.
//! the array is copied.
template<typename eT>
inline
Cube<eT>::Cube(const eT* aux_mem, const uword aux_n_rows, const uword aux_n_cols, const uword aux_n_slices)
  : n_rows(aux_n_rows)
  , n_cols(aux_n_cols)
  , n_elem_slice(aux_n_rows*aux_n_cols)
  , n_slices(aux_n_slices)
  , n_elem(aux_n_rows*aux_n_cols*aux_n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  arrayops::copy( memptr(), aux_mem, n_elem );
  }



//! in-place cube addition
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator+=(const Cube<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "addition");

  arrayops::inplace_plus( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place cube subtraction
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator-=(const Cube<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "subtraction");

  arrayops::inplace_minus( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place element-wise cube multiplication
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator%=(const Cube<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "element-wise multiplication");

  arrayops::inplace_mul( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! in-place element-wise cube division
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator/=(const Cube<eT>& m)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(*this, m, "element-wise division");

  arrayops::inplace_div( memptr(), m.memptr(), n_elem );

  return *this;
  }



//! for constructing a complex cube out of two non-complex cubes
template<typename eT>
template<typename T1, typename T2>
inline
Cube<eT>::Cube
  (
  const BaseCube<typename Cube<eT>::pod_type,T1>& A,
  const BaseCube<typename Cube<eT>::pod_type,T2>& B
  )
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init(A,B);
  }



//! construct a cube from a subview_cube instance (e.g. construct a cube from a delayed subcube operation)
template<typename eT>
inline
Cube<eT>::Cube(const subview_cube<eT>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem_slice(X.n_elem_slice)
  , n_slices(X.n_slices)
  , n_elem(X.n_elem)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  subview_cube<eT>::extract(*this, X);
  }



//! construct a cube from a subview_cube instance (e.g. construct a cube from a delayed subcube operation)
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  const bool alias = (this == &(X.m));

  if(alias == false)
    {
    init_warm(X.n_rows, X.n_cols, X.n_slices);

    subview_cube<eT>::extract(*this, X);
    }
  else
    {
    Cube<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



//! in-place cube addition (using a subcube on the right-hand-side)
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator+=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::plus_inplace(*this, X);

  return *this;
  }



//! in-place cube subtraction (using a subcube on the right-hand-side)
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator-=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::minus_inplace(*this, X);

  return *this;
  }



//! in-place element-wise cube mutiplication (using a subcube on the right-hand-side)
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator%=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::schur_inplace(*this, X);

  return *this;
  }



//! in-place element-wise cube division (using a subcube on the right-hand-side)
template<typename eT>
inline
Cube<eT>&
Cube<eT>::operator/=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview_cube<eT>::div_inplace(*this, X);

  return *this;
  }



//! provide the reference to the matrix representing a single slice
template<typename eT>
inline
Mat<eT>&
Cube<eT>::slice(const uword in_slice)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (in_slice >= n_slices), "Cube::slice(): index out of bounds" );

  if(mat_ptrs[in_slice] == NULL)
    {
    const eT* ptr = (n_elem_slice > 0) ? slice_memptr(in_slice) : NULL;

    mat_ptrs[in_slice] = new Mat<eT>('j', ptr, n_rows, n_cols);
    }

  return const_cast< Mat<eT>& >( *(mat_ptrs[in_slice]) );
  }



//! provide the reference to the matrix representing a single slice
template<typename eT>
inline
const Mat<eT>&
Cube<eT>::slice(const uword in_slice) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (in_slice >= n_slices), "Cube::slice(): index out of bounds" );

  if(mat_ptrs[in_slice] == NULL)
    {
    const eT* ptr = (n_elem_slice > 0) ? slice_memptr(in_slice) : NULL;

    mat_ptrs[in_slice] = new Mat<eT>('j', ptr, n_rows, n_cols);
    }

  return *(mat_ptrs[in_slice]);
  }



//! creation of subview_cube (subcube comprised of specified slices)
template<typename eT>
arma_inline
subview_cube<eT>
Cube<eT>::slices(const uword in_slice1, const uword in_slice2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_slice1 > in_slice2) || (in_slice2 >= n_slices),
    "Cube::slices(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_slices = in_slice2 - in_slice1 + 1;

  return subview_cube<eT>(*this, 0, 0, in_slice1, n_rows, n_cols, subcube_n_slices);
  }



//! creation of subview_cube (subcube comprised of specified slices)
template<typename eT>
arma_inline
const subview_cube<eT>
Cube<eT>::slices(const uword in_slice1, const uword in_slice2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_slice1 > in_slice2) || (in_slice2 >= n_slices),
    "Cube::rows(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_slices = in_slice2 - in_slice1 + 1;

  return subview_cube<eT>(*this, 0, 0, in_slice1, n_rows, n_cols, subcube_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
arma_inline
subview_cube<eT>
Cube<eT>::subcube(const uword in_row1, const uword in_col1, const uword in_slice1, const uword in_row2, const uword in_col2, const uword in_slice2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 >  in_row2) || (in_col1 >  in_col2) || (in_slice1 >  in_slice2) ||
    (in_row2 >= n_rows)  || (in_col2 >= n_cols)  || (in_slice2 >= n_slices),
    "Cube::subcube(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_rows   = in_row2   - in_row1   + 1;
  const uword subcube_n_cols   = in_col2   - in_col1   + 1;
  const uword subcube_n_slices = in_slice2 - in_slice1 + 1;

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, subcube_n_rows, subcube_n_cols, subcube_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
arma_inline
const subview_cube<eT>
Cube<eT>::subcube(const uword in_row1, const uword in_col1, const uword in_slice1, const uword in_row2, const uword in_col2, const uword in_slice2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 >  in_row2) || (in_col1 >  in_col2) || (in_slice1 >  in_slice2) ||
    (in_row2 >= n_rows)  || (in_col2 >= n_cols)  || (in_slice2 >= n_slices),
    "Cube::subcube(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_rows   = in_row2   - in_row1   + 1;
  const uword subcube_n_cols   = in_col2   - in_col1   + 1;
  const uword subcube_n_slices = in_slice2 - in_slice1 + 1;

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, subcube_n_rows, subcube_n_cols, subcube_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
inline
subview_cube<eT>
Cube<eT>::subcube(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows   = n_rows;
  const uword l_n_cols   = n_cols;
  const uword l_n_slices = n_slices;

  const uword s_n_rows   = s.n_rows;
  const uword s_n_cols   = s.n_cols;
  const uword s_n_slices = s.n_slices;

  arma_debug_check
    (
       ( in_row1             >= l_n_rows) || ( in_col1             >= l_n_cols) || ( in_slice1               >= l_n_slices)
    || ((in_row1 + s_n_rows) >  l_n_rows) || ((in_col1 + s_n_cols) >  l_n_cols) || ((in_slice1 + s_n_slices) >  l_n_slices),
    "Cube::subcube(): indices or size out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, s_n_rows, s_n_cols, s_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::subcube(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s) const
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows   = n_rows;
  const uword l_n_cols   = n_cols;
  const uword l_n_slices = n_slices;

  const uword s_n_rows   = s.n_rows;
  const uword s_n_cols   = s.n_cols;
  const uword s_n_slices = s.n_slices;

  arma_debug_check
    (
       ( in_row1             >= l_n_rows) || ( in_col1             >= l_n_cols) || ( in_slice1               >= l_n_slices)
    || ((in_row1 + s_n_rows) >  l_n_rows) || ((in_col1 + s_n_cols) >  l_n_cols) || ((in_slice1 + s_n_slices) >  l_n_slices),
    "Cube::subcube(): indices or size out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, s_n_rows, s_n_cols, s_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
inline
subview_cube<eT>
Cube<eT>::subcube(const span& row_span, const span& col_span, const span& slice_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all   = row_span.whole;
  const bool col_all   = col_span.whole;
  const bool slice_all = slice_span.whole;

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  const uword in_row1          = row_all   ? 0              : row_span.a;
  const uword in_row2          =                              row_span.b;
  const uword subcube_n_rows   = row_all   ? local_n_rows   : in_row2 - in_row1 + 1;

  const uword in_col1          = col_all   ? 0              : col_span.a;
  const uword in_col2          =                              col_span.b;
  const uword subcube_n_cols   = col_all   ? local_n_cols   : in_col2 - in_col1 + 1;

  const uword in_slice1        = slice_all ? 0              : slice_span.a;
  const uword in_slice2        =                              slice_span.b;
  const uword subcube_n_slices = slice_all ? local_n_slices : in_slice2 - in_slice1 + 1;

  arma_debug_check
    (
    ( row_all   ? false : ((in_row1   >  in_row2)   || (in_row2   >= local_n_rows))   )
    ||
    ( col_all   ? false : ((in_col1   >  in_col2)   || (in_col2   >= local_n_cols))   )
    ||
    ( slice_all ? false : ((in_slice1 >  in_slice2) || (in_slice2 >= local_n_slices)) )
    ,
    "Cube::subcube(): indices out of bounds or incorrectly used"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, subcube_n_rows, subcube_n_cols, subcube_n_slices);
  }



//! creation of subview_cube (generic subcube)
template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::subcube(const span& row_span, const span& col_span, const span& slice_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all   = row_span.whole;
  const bool col_all   = col_span.whole;
  const bool slice_all = slice_span.whole;

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  const uword in_row1          = row_all   ? 0              : row_span.a;
  const uword in_row2          =                              row_span.b;
  const uword subcube_n_rows   = row_all   ? local_n_rows   : in_row2 - in_row1 + 1;

  const uword in_col1          = col_all   ? 0              : col_span.a;
  const uword in_col2          =                              col_span.b;
  const uword subcube_n_cols   = col_all   ? local_n_cols   : in_col2 - in_col1 + 1;

  const uword in_slice1        = slice_all ? 0              : slice_span.a;
  const uword in_slice2        =                              slice_span.b;
  const uword subcube_n_slices = slice_all ? local_n_slices : in_slice2 - in_slice1 + 1;

  arma_debug_check
    (
    ( row_all   ? false : ((in_row1   >  in_row2)   || (in_row2   >= local_n_rows))   )
    ||
    ( col_all   ? false : ((in_col1   >  in_col2)   || (in_col2   >= local_n_cols))   )
    ||
    ( slice_all ? false : ((in_slice1 >  in_slice2) || (in_slice2 >= local_n_slices)) )
    ,
    "Cube::subcube(): indices out of bounds or incorrectly used"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, in_slice1, subcube_n_rows, subcube_n_cols, subcube_n_slices);
  }



template<typename eT>
inline
subview_cube<eT>
Cube<eT>::operator()(const span& row_span, const span& col_span, const span& slice_span)
  {
  arma_extra_debug_sigprint();

  return (*this).subcube(row_span, col_span, slice_span);
  }



template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::operator()(const span& row_span, const span& col_span, const span& slice_span) const
  {
  arma_extra_debug_sigprint();

  return (*this).subcube(row_span, col_span, slice_span);
  }



template<typename eT>
inline
subview_cube<eT>
Cube<eT>::operator()(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  return (*this).subcube(in_row1, in_col1, in_slice1, s);
  }



template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::operator()(const uword in_row1, const uword in_col1, const uword in_slice1, const SizeCube& s) const
  {
  arma_extra_debug_sigprint();

  return (*this).subcube(in_row1, in_col1, in_slice1, s);
  }



template<typename eT>
arma_inline
subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    ((in_row1 >= n_rows) || (in_col1 >= n_cols)),
    "Cube::tube(): indices out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, 1, 1, n_slices);
  }



template<typename eT>
arma_inline
const subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    ((in_row1 >= n_rows) || (in_col1 >= n_cols)),
    "Cube::tube(): indices out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, 1, 1, n_slices);
  }



template<typename eT>
arma_inline
subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 >  in_row2) || (in_col1 >  in_col2) ||
    (in_row2 >= n_rows)  || (in_col2 >= n_cols),
    "Cube::tube(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_rows = in_row2 - in_row1 + 1;
  const uword subcube_n_cols = in_col2 - in_col1 + 1;

  return subview_cube<eT>(*this, in_row1, in_col1, 0, subcube_n_rows, subcube_n_cols, n_slices);
  }



template<typename eT>
arma_inline
const subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 >  in_row2) || (in_col1 >  in_col2) ||
    (in_row2 >= n_rows)  || (in_col2 >= n_cols),
    "Cube::tube(): indices out of bounds or incorrectly used"
    );

  const uword subcube_n_rows = in_row2 - in_row1 + 1;
  const uword subcube_n_cols = in_col2 - in_col1 + 1;

  return subview_cube<eT>(*this, in_row1, in_col1, 0, subcube_n_rows, subcube_n_cols, n_slices);
  }



template<typename eT>
arma_inline
subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows = n_rows;
  const uword l_n_cols = n_cols;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  arma_debug_check
    (
    ((in_row1 >= l_n_rows) || (in_col1 >= l_n_cols) || ((in_row1 + s_n_rows) > l_n_rows) || ((in_col1 + s_n_cols) > l_n_cols)),
    "Cube::tube(): indices or size out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, s_n_rows, s_n_cols, n_slices);
  }



template<typename eT>
arma_inline
const subview_cube<eT>
Cube<eT>::tube(const uword in_row1, const uword in_col1, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  const uword l_n_rows = n_rows;
  const uword l_n_cols = n_cols;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  arma_debug_check
    (
    ((in_row1 >= l_n_rows) || (in_col1 >= l_n_cols) || ((in_row1 + s_n_rows) > l_n_rows) || ((in_col1 + s_n_cols) > l_n_cols)),
    "Cube::tube(): indices or size out of bounds"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, s_n_rows, s_n_cols, n_slices);
  }



template<typename eT>
inline
subview_cube<eT>
Cube<eT>::tube(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;

  const uword in_row1        = row_all   ? 0            : row_span.a;
  const uword in_row2        =                            row_span.b;
  const uword subcube_n_rows = row_all   ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1        = col_all   ? 0            : col_span.a;
  const uword in_col2        =                            col_span.b;
  const uword subcube_n_cols = col_all   ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Cube::tube(): indices out of bounds or incorrectly used"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, subcube_n_rows, subcube_n_cols, n_slices);
  }



template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::tube(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;

  const uword in_row1        = row_all   ? 0            : row_span.a;
  const uword in_row2        =                            row_span.b;
  const uword subcube_n_rows = row_all   ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1        = col_all   ? 0            : col_span.a;
  const uword in_col2        =                            col_span.b;
  const uword subcube_n_cols = col_all   ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "Cube::tube(): indices out of bounds or incorrectly used"
    );

  return subview_cube<eT>(*this, in_row1, in_col1, 0, subcube_n_rows, subcube_n_cols, n_slices);
  }



template<typename eT>
inline
subview_cube<eT>
Cube<eT>::head_slices(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_slices), "Cube::head_slices(): size out of bounds" );

  return subview_cube<eT>(*this, 0, 0, 0, n_rows, n_cols, N);
  }



template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::head_slices(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_slices), "Cube::head_slices(): size out of bounds" );

  return subview_cube<eT>(*this, 0, 0, 0, n_rows, n_cols, N);
  }



template<typename eT>
inline
subview_cube<eT>
Cube<eT>::tail_slices(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_slices), "Cube::tail_slices(): size out of bounds" );

  const uword start_slice = n_slices - N;

  return subview_cube<eT>(*this, 0, 0, start_slice, n_rows, n_cols, N);
  }



template<typename eT>
inline
const subview_cube<eT>
Cube<eT>::tail_slices(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > n_slices), "Cube::tail_slices(): size out of bounds" );

  const uword start_slice = n_slices - N;

  return subview_cube<eT>(*this, 0, 0, start_slice, n_rows, n_cols, N);
  }



template<typename eT>
template<typename T1>
arma_inline
subview_elem1<eT,T1>
Cube<eT>::elem(const Base<uword,T1>& a)
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
const subview_elem1<eT,T1>
Cube<eT>::elem(const Base<uword,T1>& a) const
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
subview_elem1<eT,T1>
Cube<eT>::operator()(const Base<uword,T1>& a)
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
template<typename T1>
arma_inline
const subview_elem1<eT,T1>
Cube<eT>::operator()(const Base<uword,T1>& a) const
  {
  arma_extra_debug_sigprint();

  return subview_elem1<eT,T1>(*this, a);
  }



template<typename eT>
arma_inline
subview_cube_each1<eT>
Cube<eT>::each_slice()
  {
  arma_extra_debug_sigprint();

  return subview_cube_each1<eT>(*this);
  }



template<typename eT>
arma_inline
const subview_cube_each1<eT>
Cube<eT>::each_slice() const
  {
  arma_extra_debug_sigprint();

  return subview_cube_each1<eT>(*this);
  }



template<typename eT>
template<typename T1>
inline
subview_cube_each2<eT, T1>
Cube<eT>::each_slice(const Base<uword, T1>& indices)
  {
  arma_extra_debug_sigprint();

  return subview_cube_each2<eT, T1>(*this, indices);
  }



template<typename eT>
template<typename T1>
inline
const subview_cube_each2<eT, T1>
Cube<eT>::each_slice(const Base<uword, T1>& indices) const
  {
  arma_extra_debug_sigprint();

  return subview_cube_each2<eT, T1>(*this, indices);
  }



#if defined(ARMA_USE_CXX11)

  //! apply a lambda function to each slice, where each slice is interpreted as a matrix
  template<typename eT>
  inline
  const Cube<eT>&
  Cube<eT>::each_slice(const std::function< void(Mat<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    for(uword slice_id=0; slice_id < n_slices; ++slice_id)
      {
      Mat<eT> tmp('j', slice_memptr(slice_id), n_rows, n_cols);

      F(tmp);
      }

    return *this;
    }



  template<typename eT>
  inline
  const Cube<eT>&
  Cube<eT>::each_slice(const std::function< void(const Mat<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

    for(uword slice_id=0; slice_id < n_slices; ++slice_id)
      {
      const Mat<eT> tmp('j', slice_memptr(slice_id), n_rows, n_cols);

      F(tmp);
      }

    return *this;
    }



  template<typename eT>
  inline
  const Cube<eT>&
  Cube<eT>::each_slice(const std::function< void(Mat<eT>&) >& F, const bool use_mp)
    {
    arma_extra_debug_sigprint();

    if((use_mp == false) || (arma_config::openmp == false))
      {
      return (*this).each_slice(F);
      }

    #if defined(ARMA_USE_OPENMP)
      {
      const uword local_n_slices = n_slices;
      const int   n_threads      = mp_thread_limit::get();

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword slice_id=0; slice_id < local_n_slices; ++slice_id)
        {
        Mat<eT> tmp('j', slice_memptr(slice_id), n_rows, n_cols);

        F(tmp);
        }
      }
    #endif

    return *this;
    }



  template<typename eT>
  inline
  const Cube<eT>&
  Cube<eT>::each_slice(const std::function< void(const Mat<eT>&) >& F, const bool use_mp) const
    {
    arma_extra_debug_sigprint();

    if((use_mp == false) || (arma_config::openmp == false))
      {
      return (*this).each_slice(F);
      }

    #if defined(ARMA_USE_OPENMP)
      {
      const uword local_n_slices = n_slices;
      const int   n_threads      = mp_thread_limit::get();

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword slice_id=0; slice_id < local_n_slices; ++slice_id)
        {
        Mat<eT> tmp('j', slice_memptr(slice_id), n_rows, n_cols);

        F(tmp);
        }
      }
    #endif

    return *this;
    }
#endif



//! remove specified slice
template<typename eT>
inline
void
Cube<eT>::shed_slice(const uword slice_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( slice_num >= n_slices, "Cube::shed_slice(): index out of bounds");

  shed_slices(slice_num, slice_num);
  }



//! remove specified slices
template<typename eT>
inline
void
Cube<eT>::shed_slices(const uword in_slice1, const uword in_slice2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_slice1 > in_slice2) || (in_slice2 >= n_slices),
    "Cube::shed_slices(): indices out of bounds or incorrectly used"
    );

  const uword n_keep_front = in_slice1;
  const uword n_keep_back  = n_slices - (in_slice2 + 1);

  Cube<eT> X(n_rows, n_cols, n_keep_front + n_keep_back);

  if(n_keep_front > 0)
    {
    X.slices( 0, (n_keep_front-1) ) = slices( 0, (in_slice1-1) );
    }

  if(n_keep_back > 0)
    {
    X.slices( n_keep_front,  (n_keep_front+n_keep_back-1) ) = slices( (in_slice2+1), (n_slices-1) );
    }

  steal_mem(X);
  }



//! insert N slices at the specified slice position,
//! optionally setting the elements of the inserted slices to zero
template<typename eT>
inline
void
Cube<eT>::insert_slices(const uword slice_num, const uword N, const bool set_to_zero)
  {
  arma_extra_debug_sigprint();

  const uword t_n_slices = n_slices;

  const uword A_n_slices = slice_num;
  const uword B_n_slices = t_n_slices - slice_num;

  // insertion at slice_num == n_slices is in effect an append operation
  arma_debug_check( (slice_num > t_n_slices), "Cube::insert_slices(): index out of bounds");

  if(N > 0)
    {
    Cube<eT> out(n_rows, n_cols, t_n_slices + N);

    if(A_n_slices > 0)
      {
      out.slices(0, A_n_slices-1) = slices(0, A_n_slices-1);
      }

    if(B_n_slices > 0)
      {
      out.slices(slice_num + N, t_n_slices + N - 1) = slices(slice_num, t_n_slices-1);
      }

    if(set_to_zero == true)
      {
      //out.slices(slice_num, slice_num + N - 1).zeros();

      for(uword i=slice_num; i < (slice_num + N); ++i)
        {
        arrayops::fill_zeros(out.slice_memptr(i), out.n_elem_slice);
        }
      }

    steal_mem(out);
    }
  }



//! insert the given object at the specified slice position;
//! the given object must have the same number of rows and columns as the cube
template<typename eT>
template<typename T1>
inline
void
Cube<eT>::insert_slices(const uword slice_num, const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& C   = tmp.M;

  const uword N = C.n_slices;

  const uword t_n_slices = n_slices;

  const uword A_n_slices = slice_num;
  const uword B_n_slices = t_n_slices - slice_num;

  // insertion at slice_num == n_slices is in effect an append operation
  arma_debug_check( (slice_num  >  t_n_slices), "Cube::insert_slices(): index out of bounds");

  arma_debug_check
    (
    ( (C.n_rows != n_rows) || (C.n_cols != n_cols) ),
    "Cube::insert_slices(): given object has incompatible dimensions"
    );

  if(N > 0)
    {
    Cube<eT> out(n_rows, n_cols, t_n_slices + N);

    if(A_n_slices > 0)
      {
      out.slices(0, A_n_slices-1) = slices(0, A_n_slices-1);
      }

    if(B_n_slices > 0)
      {
      out.slices(slice_num + N, t_n_slices + N - 1) = slices(slice_num, t_n_slices - 1);
      }

    out.slices(slice_num, slice_num + N - 1) = C;

    steal_mem(out);
    }
  }



//! create a cube from OpCube, i.e. run the previously delayed unary operations
template<typename eT>
template<typename gen_type>
inline
Cube<eT>::Cube(const GenCube<eT, gen_type>& X)
  : n_rows(X.n_rows)
  , n_cols(X.n_cols)
  , n_elem_slice(X.n_rows*X.n_cols)
  , n_slices(X.n_slices)
  , n_elem(X.n_rows*X.n_cols*X.n_slices)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold();

  X.apply(*this);
  }



template<typename eT>
template<typename gen_type>
inline
Cube<eT>&
Cube<eT>::operator=(const GenCube<eT, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  init_warm(X.n_rows, X.n_cols, X.n_slices);

  X.apply(*this);

  return *this;
  }



template<typename eT>
template<typename gen_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const GenCube<eT, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  X.apply_inplace_plus(*this);

  return *this;
  }



template<typename eT>
template<typename gen_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const GenCube<eT, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  X.apply_inplace_minus(*this);

  return *this;
  }



template<typename eT>
template<typename gen_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const GenCube<eT, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  X.apply_inplace_schur(*this);

  return *this;
  }



template<typename eT>
template<typename gen_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const GenCube<eT, gen_type>& X)
  {
  arma_extra_debug_sigprint();

  X.apply_inplace_div(*this);

  return *this;
  }



//! create a cube from OpCube, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>::Cube(const OpCube<T1, op_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  op_type::apply(*this, X);
  }



//! create a cube from OpCube, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator=(const OpCube<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  op_type::apply(*this, X);

  return *this;
  }



//! in-place cube addition, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const OpCube<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator+=(m);
  }



//! in-place cube subtraction, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const OpCube<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator-=(m);
  }



//! in-place cube element-wise multiplication, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const OpCube<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator%=(m);
  }



//! in-place cube element-wise division, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const OpCube<T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator/=(m);
  }



//! create a cube from eOpCube, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>::Cube(const eOpCube<T1, eop_type>& X)
  : n_rows(X.get_n_rows())
  , n_cols(X.get_n_cols())
  , n_elem_slice(X.get_n_elem_slice())
  , n_slices(X.get_n_slices())
  , n_elem(X.get_n_elem())
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  init_cold();

  eop_type::apply(*this, X);
  }



//! create a cube from eOpCube, i.e. run the previously delayed unary operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>&
Cube<eT>::operator=(const eOpCube<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  const bool bad_alias = ( X.P.has_subview  &&  X.P.is_alias(*this) );

  if(bad_alias == false)
    {
    init_warm(X.get_n_rows(), X.get_n_cols(), X.get_n_slices());

    eop_type::apply(*this, X);
    }
  else
    {
    Cube<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



//! in-place cube addition, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const eOpCube<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_plus(*this, X);

  return *this;
  }



//! in-place cube subtraction, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const eOpCube<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_minus(*this, X);

  return *this;
  }



//! in-place cube element-wise multiplication, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const eOpCube<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_schur(*this, X);

  return *this;
  }



//! in-place cube element-wise division, with the right-hand-side operand having delayed operations
template<typename eT>
template<typename T1, typename eop_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const eOpCube<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

  eop_type::apply_inplace_div(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>::Cube(const mtOpCube<eT, T1, op_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  op_type::apply(*this, X);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator=(const mtOpCube<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  op_type::apply(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const mtOpCube<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator+=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const mtOpCube<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator-=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const mtOpCube<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator%=(m);
  }



template<typename eT>
template<typename T1, typename op_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const mtOpCube<eT, T1, op_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator/=(m);
  }



//! create a cube from Glue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>::Cube(const GlueCube<T1, T2, glue_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);
  this->operator=(X);
  }



//! create a cube from Glue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator=(const GlueCube<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  glue_type::apply(*this, X);

  return *this;
  }


//! in-place cube addition, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const GlueCube<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator+=(m);
  }



//! in-place cube subtraction, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const GlueCube<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator-=(m);
  }



//! in-place cube element-wise multiplication, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const GlueCube<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator%=(m);
  }



//! in-place cube element-wise division, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const GlueCube<T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const Cube<eT> m(X);

  return (*this).operator/=(m);
  }



//! create a cube from eGlue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>::Cube(const eGlueCube<T1, T2, eglue_type>& X)
  : n_rows(X.get_n_rows())
  , n_cols(X.get_n_cols())
  , n_elem_slice(X.get_n_elem_slice())
  , n_slices(X.get_n_slices())
  , n_elem(X.get_n_elem())
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  init_cold();

  eglue_type::apply(*this, X);
  }



//! create a cube from Glue, i.e. run the previously delayed binary operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>&
Cube<eT>::operator=(const eGlueCube<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  const bool bad_alias = ( (X.P1.has_subview  &&  X.P1.is_alias(*this))  ||  (X.P2.has_subview  &&  X.P2.is_alias(*this)) );

  if(bad_alias == false)
    {
    init_warm(X.get_n_rows(), X.get_n_cols(), X.get_n_slices());

    eglue_type::apply(*this, X);
    }
  else
    {
    Cube<eT> tmp(X);

    steal_mem(tmp);
    }

  return *this;
  }



//! in-place cube addition, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const eGlueCube<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_plus(*this, X);

  return *this;
  }



//! in-place cube subtraction, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const eGlueCube<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_minus(*this, X);

  return *this;
  }



//! in-place cube element-wise multiplication, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const eGlueCube<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
  arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

  eglue_type::apply_inplace_schur(*this, X);

  return *this;
  }



//! in-place cube element-wise division, with the right-hand-side operands having delayed operations
template<typename eT>
template<typename T1, typename T2, typename eglue_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const eGlueCube<T1, T2, eglue_type>& X)
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
Cube<eT>::Cube(const mtGlueCube<eT, T1, T2, glue_type>& X)
  : n_rows(0)
  , n_cols(0)
  , n_elem_slice(0)
  , n_slices(0)
  , n_elem(0)
  , mem_state(0)
  , mem()
  , mat_ptrs(0)
  {
  arma_extra_debug_sigprint_this(this);

  glue_type::apply(*this, X);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator=(const mtGlueCube<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  glue_type::apply(*this, X);

  return *this;
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator+=(const mtGlueCube<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator+=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator-=(const mtGlueCube<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator-=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator%=(const mtGlueCube<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator%=(m);
  }



template<typename eT>
template<typename T1, typename T2, typename glue_type>
inline
Cube<eT>&
Cube<eT>::operator/=(const mtGlueCube<eT, T1, T2, glue_type>& X)
  {
  arma_extra_debug_sigprint();

  const Cube<eT> m(X);

  return (*this).operator/=(m);
  }



//! linear element accessor (treats the cube as a vector); no bounds check; assumes memory is aligned
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::at_alt(const uword i) const
  {
  const eT* mem_aligned = mem;
  memory::mark_as_aligned(mem_aligned);

  return mem_aligned[i];
  }



//! linear element accessor (treats the cube as a vector); bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
eT&
Cube<eT>::operator() (const uword i)
  {
  arma_debug_check( (i >= n_elem), "Cube::operator(): index out of bounds");
  return access::rw(mem[i]);
  }



//! linear element accessor (treats the cube as a vector); bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::operator() (const uword i) const
  {
  arma_debug_check( (i >= n_elem), "Cube::operator(): index out of bounds");
  return mem[i];
  }


//! linear element accessor (treats the cube as a vector); no bounds check.
template<typename eT>
arma_inline
arma_warn_unused
eT&
Cube<eT>::operator[] (const uword i)
  {
  return access::rw(mem[i]);
  }



//! linear element accessor (treats the cube as a vector); no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::operator[] (const uword i) const
  {
  return mem[i];
  }



//! linear element accessor (treats the cube as a vector); no bounds check.
template<typename eT>
arma_inline
arma_warn_unused
eT&
Cube<eT>::at(const uword i)
  {
  return access::rw(mem[i]);
  }



//! linear element accessor (treats the cube as a vector); no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::at(const uword i) const
  {
  return mem[i];
  }



//! element accessor; bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
eT&
Cube<eT>::operator() (const uword in_row, const uword in_col, const uword in_slice)
  {
  arma_debug_check
    (
    (in_row >= n_rows) ||
    (in_col >= n_cols) ||
    (in_slice >= n_slices)
    ,
    "Cube::operator(): index out of bounds"
    );

  return access::rw(mem[in_slice*n_elem_slice + in_col*n_rows + in_row]);
  }



//! element accessor; bounds checking not done when ARMA_NO_DEBUG is defined
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::operator() (const uword in_row, const uword in_col, const uword in_slice) const
  {
  arma_debug_check
    (
    (in_row >= n_rows) ||
    (in_col >= n_cols) ||
    (in_slice >= n_slices)
    ,
    "Cube::operator(): index out of bounds"
    );

  return mem[in_slice*n_elem_slice + in_col*n_rows + in_row];
  }



//! element accessor; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
eT&
Cube<eT>::at(const uword in_row, const uword in_col, const uword in_slice)
  {
  return access::rw( mem[in_slice*n_elem_slice + in_col*n_rows + in_row] );
  }



//! element accessor; no bounds check
template<typename eT>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::at(const uword in_row, const uword in_col, const uword in_slice) const
  {
  return mem[in_slice*n_elem_slice + in_col*n_rows + in_row];
  }



//! prefix ++
template<typename eT>
arma_inline
const Cube<eT>&
Cube<eT>::operator++()
  {
  Cube_aux::prefix_pp(*this);
  return *this;
  }



//! postfix ++  (must not return the object by reference)
template<typename eT>
arma_inline
void
Cube<eT>::operator++(int)
  {
  Cube_aux::postfix_pp(*this);
  }



//! prefix --
template<typename eT>
arma_inline
const Cube<eT>&
Cube<eT>::operator--()
  {
  Cube_aux::prefix_mm(*this);
  return *this;
  }



//! postfix --  (must not return the object by reference)
template<typename eT>
arma_inline
void
Cube<eT>::operator--(int)
  {
  Cube_aux::postfix_mm(*this);
  }



//! returns true if all of the elements are finite
template<typename eT>
arma_inline
arma_warn_unused
bool
Cube<eT>::is_finite() const
  {
  return arrayops::is_finite( memptr(), n_elem );
  }



//! returns true if the cube has no elements
template<typename eT>
arma_inline
arma_warn_unused
bool
Cube<eT>::is_empty() const
  {
  return (n_elem == 0);
  }



template<typename eT>
inline
arma_warn_unused
bool
Cube<eT>::has_inf() const
  {
  arma_extra_debug_sigprint();

  return arrayops::has_inf( memptr(), n_elem );
  }



template<typename eT>
inline
arma_warn_unused
bool
Cube<eT>::has_nan() const
  {
  arma_extra_debug_sigprint();

  return arrayops::has_nan( memptr(), n_elem );
  }



//! returns true if the given index is currently in range
template<typename eT>
arma_inline
arma_warn_unused
bool
Cube<eT>::in_range(const uword i) const
  {
  return (i < n_elem);
  }



//! returns true if the given start and end indices are currently in range
template<typename eT>
arma_inline
arma_warn_unused
bool
Cube<eT>::in_range(const span& x) const
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
Cube<eT>::in_range(const uword in_row, const uword in_col, const uword in_slice) const
  {
  return ( (in_row < n_rows) && (in_col < n_cols) && (in_slice < n_slices) );
  }



template<typename eT>
inline
arma_warn_unused
bool
Cube<eT>::in_range(const span& row_span, const span& col_span, const span& slice_span) const
  {
  arma_extra_debug_sigprint();

  const uword in_row1   = row_span.a;
  const uword in_row2   = row_span.b;

  const uword in_col1   = col_span.a;
  const uword in_col2   = col_span.b;

  const uword in_slice1 = slice_span.a;
  const uword in_slice2 = slice_span.b;


  const bool rows_ok   = row_span.whole   ? true : ( (in_row1   <= in_row2)   && (in_row2   < n_rows)   );
  const bool cols_ok   = col_span.whole   ? true : ( (in_col1   <= in_col2)   && (in_col2   < n_cols)   );
  const bool slices_ok = slice_span.whole ? true : ( (in_slice1 <= in_slice2) && (in_slice2 < n_slices) );


  return ( (rows_ok == true) && (cols_ok == true) && (slices_ok == true) );
  }



template<typename eT>
inline
arma_warn_unused
bool
Cube<eT>::in_range(const uword in_row, const uword in_col, const uword in_slice, const SizeCube& s) const
  {
  const uword l_n_rows   = n_rows;
  const uword l_n_cols   = n_cols;
  const uword l_n_slices = n_slices;

  if(
       ( in_row             >= l_n_rows) || ( in_col             >= l_n_cols) || ( in_slice               >= l_n_slices)
    || ((in_row + s.n_rows) >  l_n_rows) || ((in_col + s.n_cols) >  l_n_cols) || ((in_slice + s.n_slices) >  l_n_slices)
    )
    {
    return false;
    }
  else
    {
    return true;
    }
  }



//! returns a pointer to array of eTs used by the cube
template<typename eT>
arma_inline
arma_warn_unused
eT*
Cube<eT>::memptr()
  {
  return const_cast<eT*>(mem);
  }



//! returns a pointer to array of eTs used by the cube
template<typename eT>
arma_inline
arma_warn_unused
const eT*
Cube<eT>::memptr() const
  {
  return mem;
  }



//! returns a pointer to array of eTs used by the specified slice in the cube
template<typename eT>
arma_inline
arma_warn_unused
eT*
Cube<eT>::slice_memptr(const uword uslice)
  {
  return const_cast<eT*>( &mem[ uslice*n_elem_slice ] );
  }



//! returns a pointer to array of eTs used by the specified slice in the cube
template<typename eT>
arma_inline
arma_warn_unused
const eT*
Cube<eT>::slice_memptr(const uword uslice) const
  {
  return &mem[ uslice*n_elem_slice ];
  }



//! returns a pointer to array of eTs used by the specified slice in the cube
template<typename eT>
arma_inline
arma_warn_unused
eT*
Cube<eT>::slice_colptr(const uword uslice, const uword col)
  {
  return const_cast<eT*>( &mem[ uslice*n_elem_slice + col*n_rows] );
  }



//! returns a pointer to array of eTs used by the specified slice in the cube
template<typename eT>
arma_inline
arma_warn_unused
const eT*
Cube<eT>::slice_colptr(const uword uslice, const uword col) const
  {
  return &mem[ uslice*n_elem_slice + col*n_rows ];
  }



//! print contents of the cube (to the cout stream),
//! optionally preceding with a user specified line of text.
//! the precision and cell width are modified.
//! on return, the stream's state are restored to their original values.
template<typename eT>
inline
void
Cube<eT>::impl_print(const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    get_cout_stream() << extra_text << '\n';
    }

  arma_ostream::print(get_cout_stream(), *this, true);
  }


//! print contents of the cube to a user specified stream,
//! optionally preceding with a user specified line of text.
//! the precision and cell width are modified.
//! on return, the stream's state are restored to their original values.
template<typename eT>
inline
void
Cube<eT>::impl_print(std::ostream& user_stream, const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    user_stream << extra_text << '\n';
    }

  arma_ostream::print(user_stream, *this, true);
  }



//! print contents of the cube (to the cout stream),
//! optionally preceding with a user specified line of text.
//! the stream's state are used as is and are not modified
//! (i.e. the precision and cell width are not modified).
template<typename eT>
inline
void
Cube<eT>::impl_raw_print(const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    get_cout_stream() << extra_text << '\n';
    }

  arma_ostream::print(get_cout_stream(), *this, false);
  }



//! print contents of the cube to a user specified stream,
//! optionally preceding with a user specified line of text.
//! the stream's state are used as is and are not modified.
//! (i.e. the precision and cell width are not modified).
template<typename eT>
inline
void
Cube<eT>::impl_raw_print(std::ostream& user_stream, const std::string& extra_text) const
  {
  arma_extra_debug_sigprint();

  if(extra_text.length() != 0)
    {
    user_stream << extra_text << '\n';
    }

  arma_ostream::print(user_stream, *this, false);
  }



//! change the cube to have user specified dimensions (data is not preserved)
template<typename eT>
inline
void
Cube<eT>::set_size(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices)
  {
  arma_extra_debug_sigprint();

  init_warm(in_n_rows, in_n_cols, in_n_slices);
  }



//! change the cube to have user specified dimensions (data is preserved)
template<typename eT>
inline
void
Cube<eT>::reshape(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, in_rows, in_cols, in_slices);
  }



//! NOTE: don't use this form; it's deprecated and will be removed
template<typename eT>
arma_deprecated
inline
void
Cube<eT>::reshape(const uword in_rows, const uword in_cols, const uword in_slices, const uword dim)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, in_rows, in_cols, in_slices, dim);
  }



//! change the cube to have user specified dimensions (data is preserved)
template<typename eT>
inline
void
Cube<eT>::resize(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  *this = arma::resize(*this, in_rows, in_cols, in_slices);
  }



template<typename eT>
inline
void
Cube<eT>::set_size(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  init_warm(s.n_rows, s.n_cols, s.n_slices);
  }



template<typename eT>
inline
void
Cube<eT>::reshape(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  *this = arma::reshape(*this, s.n_rows, s.n_cols, s.n_slices, 0);
  }



template<typename eT>
inline
void
Cube<eT>::resize(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  *this = arma::resize(*this, s.n_rows, s.n_cols, s.n_slices);
  }



//! change the cube (without preserving data) to have the same dimensions as the given cube
template<typename eT>
template<typename eT2>
inline
void
Cube<eT>::copy_size(const Cube<eT2>& m)
  {
  arma_extra_debug_sigprint();

  init_warm(m.n_rows, m.n_cols, m.n_slices);
  }



//! apply a functor to each element
template<typename eT>
template<typename functor>
inline
const Cube<eT>&
Cube<eT>::for_each(functor F)
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
const Cube<eT>&
Cube<eT>::for_each(functor F) const
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



//! transform each element in the cube using a functor
template<typename eT>
template<typename functor>
inline
const Cube<eT>&
Cube<eT>::transform(functor F)
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



//! imbue (fill) the cube with values provided by a functor
template<typename eT>
template<typename functor>
inline
const Cube<eT>&
Cube<eT>::imbue(functor F)
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
const Cube<eT>&
Cube<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  arrayops::replace(memptr(), n_elem, old_val, new_val);

  return *this;
  }



//! fill the cube with the specified value
template<typename eT>
inline
const Cube<eT>&
Cube<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_set( memptr(), val, n_elem );

  return *this;
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  arrayops::fill_zeros(memptr(), n_elem);

  return *this;
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::zeros(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols, in_slices);

  return (*this).zeros();
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::zeros(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  return (*this).zeros(s.n_rows, s.n_cols, s.n_slices);
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::ones()
  {
  arma_extra_debug_sigprint();

  return (*this).fill(eT(1));
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::ones(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols, in_slices);

  return (*this).fill(eT(1));
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::ones(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  return (*this).ones(s.n_rows, s.n_cols, s.n_slices);
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randu()
  {
  arma_extra_debug_sigprint();

  arma_rng::randu<eT>::fill( memptr(), n_elem );

  return *this;
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randu(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols, in_slices);

  return (*this).randu();
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randu(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  return (*this).randu(s.n_rows, s.n_cols, s.n_slices);
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randn()
  {
  arma_extra_debug_sigprint();

  arma_rng::randn<eT>::fill( memptr(), n_elem );

  return *this;
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randn(const uword in_rows, const uword in_cols, const uword in_slices)
  {
  arma_extra_debug_sigprint();

  set_size(in_rows, in_cols, in_slices);

  return (*this).randn();
  }



template<typename eT>
inline
const Cube<eT>&
Cube<eT>::randn(const SizeCube& s)
  {
  arma_extra_debug_sigprint();

  return (*this).randn(s.n_rows, s.n_cols, s.n_slices);
  }



template<typename eT>
inline
void
Cube<eT>::reset()
  {
  arma_extra_debug_sigprint();

  init_warm(0,0,0);
  }



template<typename eT>
inline
void
Cube<eT>::soft_reset()
  {
  arma_extra_debug_sigprint();

  // don't change the size if the cube has a fixed size
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
Cube<eT>::set_real(const BaseCube<typename Cube<eT>::pod_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  Cube_aux::set_real(*this, X);
  }



template<typename eT>
template<typename T1>
inline
void
Cube<eT>::set_imag(const BaseCube<typename Cube<eT>::pod_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  Cube_aux::set_imag(*this, X);
  }



template<typename eT>
inline
arma_warn_unused
eT
Cube<eT>::min() const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::min(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_min::direct_min(memptr(), n_elem);
  }



template<typename eT>
inline
arma_warn_unused
eT
Cube<eT>::max() const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::max(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_max::direct_max(memptr(), n_elem);
  }



template<typename eT>
inline
eT
Cube<eT>::min(uword& index_of_min_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::min(): object has no elements");

    index_of_min_val = uword(0);

    return Datum<eT>::nan;
    }

  return op_min::direct_min(memptr(), n_elem, index_of_min_val);
  }



template<typename eT>
inline
eT
Cube<eT>::max(uword& index_of_max_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::max(): object has no elements");

    index_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  return op_max::direct_max(memptr(), n_elem, index_of_max_val);
  }



template<typename eT>
inline
eT
Cube<eT>::min(uword& row_of_min_val, uword& col_of_min_val, uword& slice_of_min_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::min(): object has no elements");

    row_of_min_val   = uword(0);
    col_of_min_val   = uword(0);
    slice_of_min_val = uword(0);

    return Datum<eT>::nan;
    }

  uword i;

  eT val = op_min::direct_min(memptr(), n_elem, i);

  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

    row_of_min_val = j % n_rows;
    col_of_min_val = j / n_rows;
  slice_of_min_val = in_slice;

  return val;
  }



template<typename eT>
inline
eT
Cube<eT>::max(uword& row_of_max_val, uword& col_of_max_val, uword& slice_of_max_val) const
  {
  arma_extra_debug_sigprint();

  if(n_elem == 0)
    {
    arma_debug_check(true, "Cube::max(): object has no elements");

    row_of_max_val   = uword(0);
    col_of_max_val   = uword(0);
    slice_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  uword i;

  eT val = op_max::direct_max(memptr(), n_elem, i);

  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

    row_of_max_val = j % n_rows;
    col_of_max_val = j / n_rows;
  slice_of_max_val = in_slice;

  return val;
  }



//! save the cube to a file
template<typename eT>
inline
bool
Cube<eT>::save(const std::string name, const file_type type, const bool print_status) const
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

    case raw_binary:
      save_okay = diskio::save_raw_binary(*this, name);
      break;

    case arma_binary:
      save_okay = diskio::save_arma_binary(*this, name);
      break;

    case ppm_binary:
      save_okay = diskio::save_ppm_binary(*this, name);
      break;

    case hdf5_binary:
      save_okay = diskio::save_hdf5_binary(*this, hdf5_name(name));
      break;

    case hdf5_binary_trans:
      {
      Cube<eT> tmp;

      op_strans_cube::apply_noalias(tmp, (*this));

      save_okay = diskio::save_hdf5_binary(tmp, hdf5_name(name));
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Cube::save(): couldn't write to ", name); }

  return save_okay;
  }



template<typename eT>
inline
bool
Cube<eT>::save(const hdf5_name& spec, const file_type type, const bool print_status) const
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
      Cube<eT> tmp;

      op_strans_cube::apply_noalias(tmp, (*this));

      save_okay = diskio::save_hdf5_binary(tmp, spec);
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Cube::save(): couldn't write to ", spec.filename); }

  return save_okay;
  }



//! save the cube to a stream
template<typename eT>
inline
bool
Cube<eT>::save(std::ostream& os, const file_type type, const bool print_status) const
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

    case raw_binary:
      save_okay = diskio::save_raw_binary(*this, os);
      break;

    case arma_binary:
      save_okay = diskio::save_arma_binary(*this, os);
      break;

    case ppm_binary:
      save_okay = diskio::save_ppm_binary(*this, os);
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::save(): unsupported file type"); }
      save_okay = false;
    }

  if(print_status && (save_okay == false))  { arma_debug_warn("Cube::save(): couldn't write to given stream"); }

  return save_okay;
  }



//! load a cube from a file
template<typename eT>
inline
bool
Cube<eT>::load(const std::string name, const file_type type, const bool print_status)
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

    case raw_binary:
      load_okay = diskio::load_raw_binary(*this, name, err_msg);
      break;

    case arma_binary:
      load_okay = diskio::load_arma_binary(*this, name, err_msg);
      break;

    case ppm_binary:
      load_okay = diskio::load_ppm_binary(*this, name, err_msg);
      break;

    case hdf5_binary:
      load_okay = diskio::load_hdf5_binary(*this, hdf5_name(name), err_msg);
      break;

    case hdf5_binary_trans:
      {
      Cube<eT> tmp;

      load_okay = diskio::load_hdf5_binary(tmp, hdf5_name(name), err_msg);

      if(load_okay)  { op_strans_cube::apply_noalias((*this), tmp); }
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::load(): unsupported file type"); }
      load_okay = false;
    }

  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Cube::load(): ", err_msg, name);
      }
    else
      {
      arma_debug_warn("Cube::load(): couldn't read ", name);
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
Cube<eT>::load(const hdf5_name& spec, const file_type type, const bool print_status)
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
      Cube<eT> tmp;

      load_okay = diskio::load_hdf5_binary(tmp, spec, err_msg);

      if(load_okay)  { op_strans_cube::apply_noalias((*this), tmp); }
      }
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::load(): unsupported file type"); }
      load_okay = false;
    }

  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Cube::load(): ", err_msg, spec.filename);
      }
    else
      {
      arma_debug_warn("Cube::load(): couldn't read ", spec.filename);
      }
    }

  if(load_okay == false)
    {
    (*this).soft_reset();
    }

  return load_okay;
  }



//! load a cube from a stream
template<typename eT>
inline
bool
Cube<eT>::load(std::istream& is, const file_type type, const bool print_status)
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

    case raw_binary:
      load_okay = diskio::load_raw_binary(*this, is, err_msg);
      break;

    case arma_binary:
      load_okay = diskio::load_arma_binary(*this, is, err_msg);
      break;

    case ppm_binary:
      load_okay = diskio::load_ppm_binary(*this, is, err_msg);
      break;

    default:
      if(print_status)  { arma_debug_warn("Cube::load(): unsupported file type"); }
      load_okay = false;
    }


  if( (print_status == true) && (load_okay == false) )
    {
    if(err_msg.length() > 0)
      {
      arma_debug_warn("Cube::load(): ", err_msg, "the given stream");
      }
    else
      {
      arma_debug_warn("Cube::load(): couldn't load from the given stream");
      }
    }

  if(load_okay == false)
    {
    (*this).soft_reset();
    }

  return load_okay;
  }



//! save the cube to a file, without printing any error messages
template<typename eT>
inline
bool
Cube<eT>::quiet_save(const std::string name, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(name, type, false);
  }



template<typename eT>
inline
bool
Cube<eT>::quiet_save(const hdf5_name& spec, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(spec, type, false);
  }



//! save the cube to a stream, without printing any error messages
template<typename eT>
inline
bool
Cube<eT>::quiet_save(std::ostream& os, const file_type type) const
  {
  arma_extra_debug_sigprint();

  return (*this).save(os, type, false);
  }



//! load a cube from a file, without printing any error messages
template<typename eT>
inline
bool
Cube<eT>::quiet_load(const std::string name, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(name, type, false);
  }



template<typename eT>
inline
bool
Cube<eT>::quiet_load(const hdf5_name& spec, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(spec, type, false);
  }



//! load a cube from a stream, without printing any error messages
template<typename eT>
inline
bool
Cube<eT>::quiet_load(std::istream& is, const file_type type)
  {
  arma_extra_debug_sigprint();

  return (*this).load(is, type, false);
  }



template<typename eT>
inline
typename Cube<eT>::iterator
Cube<eT>::begin()
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Cube<eT>::const_iterator
Cube<eT>::begin() const
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Cube<eT>::const_iterator
Cube<eT>::cbegin() const
  {
  arma_extra_debug_sigprint();

  return memptr();
  }



template<typename eT>
inline
typename Cube<eT>::iterator
Cube<eT>::end()
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Cube<eT>::const_iterator
Cube<eT>::end() const
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Cube<eT>::const_iterator
Cube<eT>::cend() const
  {
  arma_extra_debug_sigprint();

  return memptr() + n_elem;
  }



template<typename eT>
inline
typename Cube<eT>::slice_iterator
Cube<eT>::begin_slice(const uword slice_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (slice_num >= n_slices), "begin_slice(): index out of bounds");

  return slice_memptr(slice_num);
  }



template<typename eT>
inline
typename Cube<eT>::const_slice_iterator
Cube<eT>::begin_slice(const uword slice_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (slice_num >= n_slices), "begin_slice(): index out of bounds");

  return slice_memptr(slice_num);
  }



template<typename eT>
inline
typename Cube<eT>::slice_iterator
Cube<eT>::end_slice(const uword slice_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (slice_num >= n_slices), "end_slice(): index out of bounds");

  return slice_memptr(slice_num) + n_elem_slice;
  }



template<typename eT>
inline
typename Cube<eT>::const_slice_iterator
Cube<eT>::end_slice(const uword slice_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (slice_num >= n_slices), "end_slice(): index out of bounds");

  return slice_memptr(slice_num) + n_elem_slice;
  }



//! resets this cube to an empty matrix
template<typename eT>
inline
void
Cube<eT>::clear()
  {
  reset();
  }



//! returns true if the cube has no elements
template<typename eT>
inline
bool
Cube<eT>::empty() const
  {
  return (n_elem == 0);
  }



//! returns the number of elements in this cube
template<typename eT>
inline
uword
Cube<eT>::size() const
  {
  return n_elem;
  }



template<typename eT>
inline
void
Cube<eT>::swap(Cube<eT>& B)
  {
  Cube<eT>& A = (*this);

  arma_extra_debug_sigprint(arma_str::format("A = %x   B = %x") % &A % &B);

  if( (A.mem_state == 0) && (B.mem_state == 0) && (A.n_elem > Cube_prealloc::mem_n_elem) && (B.n_elem > Cube_prealloc::mem_n_elem) )
    {
    A.delete_mat();
    B.delete_mat();

    std::swap( access::rw(A.n_rows),       access::rw(B.n_rows)       );
    std::swap( access::rw(A.n_cols),       access::rw(B.n_cols)       );
    std::swap( access::rw(A.n_elem_slice), access::rw(B.n_elem_slice) );
    std::swap( access::rw(A.n_slices),     access::rw(B.n_slices)     );
    std::swap( access::rw(A.n_elem),       access::rw(B.n_elem)       );
    std::swap( access::rw(A.mem),          access::rw(B.mem)          );

    A.create_mat();
    B.create_mat();
    }
  else
  if( (A.mem_state == 0) && (B.mem_state == 0) && (A.n_elem <= Cube_prealloc::mem_n_elem) && (B.n_elem <= Cube_prealloc::mem_n_elem) )
    {
    A.delete_mat();
    B.delete_mat();

    std::swap( access::rw(A.n_rows),       access::rw(B.n_rows)       );
    std::swap( access::rw(A.n_cols),       access::rw(B.n_cols)       );
    std::swap( access::rw(A.n_elem_slice), access::rw(B.n_elem_slice) );
    std::swap( access::rw(A.n_slices),     access::rw(B.n_slices)     );
    std::swap( access::rw(A.n_elem),       access::rw(B.n_elem)       );

    const uword N = (std::max)(A.n_elem, B.n_elem);

    eT* A_mem = A.memptr();
    eT* B_mem = B.memptr();

    for(uword i=0; i<N; ++i)  { std::swap( A_mem[i], B_mem[i] ); }

    A.create_mat();
    B.create_mat();
    }
  else
    {
    // generic swap

    if(A.n_elem <= B.n_elem)
      {
      Cube<eT> C = A;

      A.steal_mem(B);
      B.steal_mem(C);
      }
    else
      {
      Cube<eT> C = B;

      B.steal_mem(A);
      A.steal_mem(C);
      }
    }
  }



//! try to steal the memory from a given cube;
//! if memory can't be stolen, copy the given cube
template<typename eT>
inline
void
Cube<eT>::steal_mem(Cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(this == &x)  { return; }

  if( (mem_state <= 1) && ( ((x.mem_state == 0) && (x.n_elem > Cube_prealloc::mem_n_elem)) || (x.mem_state == 1) ) )
    {
    reset();

    const uword x_n_slices = x.n_slices;

    access::rw(n_rows)       = x.n_rows;
    access::rw(n_cols)       = x.n_cols;
    access::rw(n_elem_slice) = x.n_elem_slice;
    access::rw(n_slices)     = x_n_slices;
    access::rw(n_elem)       = x.n_elem;
    access::rw(mem_state)    = x.mem_state;
    access::rw(mem)          = x.mem;

    if(x_n_slices > Cube_prealloc::mat_ptrs_size)
      {
      access::rw(  mat_ptrs) = x.mat_ptrs;
      access::rw(x.mat_ptrs) = 0;
      }
    else
      {
      access::rw(mat_ptrs) = const_cast< const Mat<eT>** >(mat_ptrs_local);

      for(uword i=0; i < x_n_slices; ++i)
        {
          mat_ptrs[i] = x.mat_ptrs[i];
        x.mat_ptrs[i] = 0;
        }
      }

    access::rw(x.n_rows)       = 0;
    access::rw(x.n_cols)       = 0;
    access::rw(x.n_elem_slice) = 0;
    access::rw(x.n_slices)     = 0;
    access::rw(x.n_elem)       = 0;
    access::rw(x.mem_state)    = 0;
    access::rw(x.mem)          = 0;
    }
  else
    {
    (*this).operator=(x);
    }
  }



//
//  Cube::fixed



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
void
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::mem_setup()
  {
  arma_extra_debug_sigprint();

  if(fixed_n_elem > 0)
    {
    access::rw(Cube<eT>::n_rows)       = fixed_n_rows;
    access::rw(Cube<eT>::n_cols)       = fixed_n_cols;
    access::rw(Cube<eT>::n_elem_slice) = fixed_n_rows * fixed_n_cols;
    access::rw(Cube<eT>::n_slices)     = fixed_n_slices;
    access::rw(Cube<eT>::n_elem)       = fixed_n_elem;
    access::rw(Cube<eT>::mem_state)    = 3;
    access::rw(Cube<eT>::mem)          = (fixed_n_elem   > Cube_prealloc::mem_n_elem)    ? mem_local_extra      : mem_local;
    access::rw(Cube<eT>::mat_ptrs)     = const_cast< const Mat<eT>** >( \
                                         (fixed_n_slices > Cube_prealloc::mat_ptrs_size) ? mat_ptrs_local_extra : mat_ptrs_local );

    create_mat();
    }
  else
    {
    access::rw(Cube<eT>::n_rows)       = 0;
    access::rw(Cube<eT>::n_cols)       = 0;
    access::rw(Cube<eT>::n_elem_slice) = 0;
    access::rw(Cube<eT>::n_slices)     = 0;
    access::rw(Cube<eT>::n_elem)       = 0;
    access::rw(Cube<eT>::mem_state)    = 3;
    access::rw(Cube<eT>::mem)          = 0;
    access::rw(Cube<eT>::mat_ptrs)     = 0;
    }
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
inline
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::fixed()
  {
  arma_extra_debug_sigprint_this(this);

  mem_setup();
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
inline
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::fixed(const fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>& X)
  {
  arma_extra_debug_sigprint_this(this);

  mem_setup();

        eT* dest = (use_extra) ?   mem_local_extra :   mem_local;
  const eT* src  = (use_extra) ? X.mem_local_extra : X.mem_local;

  arrayops::copy( dest, src, fixed_n_elem );
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
template<typename fill_type>
inline
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::fixed(const fill::fill_class<fill_type>&)
  {
  arma_extra_debug_sigprint_this(this);

  mem_setup();

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();

  if(is_same_type<fill_type, fill::fill_eye  >::yes)  { arma_debug_check(true, "Cube::fixed::fixed(): unsupported fill type"); }
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
template<typename T1>
inline
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::fixed(const BaseCube<eT,T1>& A)
  {
  arma_extra_debug_sigprint_this(this);

  mem_setup();

  Cube<eT>::operator=(A.get_ref());
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
template<typename T1, typename T2>
inline
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::fixed(const BaseCube<pod_type,T1>& A, const BaseCube<pod_type,T2>& B)
  {
  arma_extra_debug_sigprint_this(this);

  mem_setup();

  Cube<eT>::init(A,B);
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
inline
Cube<eT>&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator=(const fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>& X)
  {
  arma_extra_debug_sigprint();

        eT* dest = (use_extra) ?   mem_local_extra :   mem_local;
  const eT* src  = (use_extra) ? X.mem_local_extra : X.mem_local;

  arrayops::copy( dest, src, fixed_n_elem );

  return *this;
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator[] (const uword i)
  {
  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator[] (const uword i) const
  {
  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::at(const uword i)
  {
  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::at(const uword i) const
  {
  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator() (const uword i)
  {
  arma_debug_check( (i >= fixed_n_elem), "Cube::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator() (const uword i) const
  {
  arma_debug_check( (i >= fixed_n_elem), "Cube::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::at(const uword in_row, const uword in_col, const uword in_slice)
  {
  const uword i = in_slice*fixed_n_elem_slice + in_col*fixed_n_rows + in_row;

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::at(const uword in_row, const uword in_col, const uword in_slice) const
  {
  const uword i = in_slice*fixed_n_elem_slice + in_col*fixed_n_rows + in_row;

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator() (const uword in_row, const uword in_col, const uword in_slice)
  {
  arma_debug_check
    (
    (in_row   >= fixed_n_rows  ) ||
    (in_col   >= fixed_n_cols  ) ||
    (in_slice >= fixed_n_slices)
    ,
    "operator(): index out of bounds"
    );

  const uword i = in_slice*fixed_n_elem_slice + in_col*fixed_n_rows + in_row;

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



template<typename eT>
template<uword fixed_n_rows, uword fixed_n_cols, uword fixed_n_slices>
arma_inline
arma_warn_unused
const eT&
Cube<eT>::fixed<fixed_n_rows, fixed_n_cols, fixed_n_slices>::operator() (const uword in_row, const uword in_col, const uword in_slice) const
  {
  arma_debug_check
    (
    (in_row   >= fixed_n_rows  ) ||
    (in_col   >= fixed_n_cols  ) ||
    (in_slice >= fixed_n_slices)
    ,
    "Cube::operator(): index out of bounds"
    );

  const uword i = in_slice*fixed_n_elem_slice + in_col*fixed_n_rows + in_row;

  return (use_extra) ? mem_local_extra[i] : mem_local[i];
  }



//
// Cube_aux



//! prefix ++
template<typename eT>
arma_inline
void
Cube_aux::prefix_pp(Cube<eT>& x)
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
arma_inline
void
Cube_aux::prefix_pp(Cube< std::complex<T> >& x)
  {
  x += T(1);
  }



//! postfix ++
template<typename eT>
arma_inline
void
Cube_aux::postfix_pp(Cube<eT>& x)
  {
        eT* memptr = x.memptr();
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
arma_inline
void
Cube_aux::postfix_pp(Cube< std::complex<T> >& x)
  {
  x += T(1);
  }



//! prefix --
template<typename eT>
arma_inline
void
Cube_aux::prefix_mm(Cube<eT>& x)
  {
        eT* memptr = x.memptr();
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
arma_inline
void
Cube_aux::prefix_mm(Cube< std::complex<T> >& x)
  {
  x -= T(1);
  }



//! postfix --
template<typename eT>
arma_inline
void
Cube_aux::postfix_mm(Cube<eT>& x)
  {
        eT* memptr = x.memptr();
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
arma_inline
void
Cube_aux::postfix_mm(Cube< std::complex<T> >& x)
  {
  x -= T(1);
  }



template<typename eT, typename T1>
inline
void
Cube_aux::set_real(Cube<eT>& out, const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(X.get_ref());
  const Cube<eT>& A   = tmp.M;

  arma_debug_assert_same_size( out, A, "Cube::set_real()" );

  out = A;
  }



template<typename eT, typename T1>
inline
void
Cube_aux::set_imag(Cube<eT>&, const BaseCube<eT,T1>&)
  {
  arma_extra_debug_sigprint();
  }



template<typename T, typename T1>
inline
void
Cube_aux::set_real(Cube< std::complex<T> >& out, const BaseCube<T,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  const ProxyCube<T1> P(X.get_ref());

  const uword local_n_rows   = P.get_n_rows();
  const uword local_n_cols   = P.get_n_cols();
  const uword local_n_slices = P.get_n_slices();

  arma_debug_assert_same_size
    (
    out.n_rows,   out.n_cols,   out.n_slices,
    local_n_rows, local_n_cols, local_n_slices,
    "Cube::set_real()"
    );

  eT* out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    const uword N = out.n_elem;

    for(uword i=0; i<N; ++i)
      {
      //out_mem[i].real() = PA[i];
      out_mem[i] = std::complex<T>( A[i], out_mem[i].imag() );
      }
    }
  else
    {
    for(uword slice = 0; slice < local_n_slices; ++slice)
    for(uword col   = 0; col   < local_n_cols;   ++col  )
    for(uword row   = 0; row   < local_n_rows;   ++row  )
      {
      (*out_mem) = std::complex<T>( P.at(row,col,slice), (*out_mem).imag() );
      out_mem++;
      }
    }
  }



template<typename T, typename T1>
inline
void
Cube_aux::set_imag(Cube< std::complex<T> >& out, const BaseCube<T,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  const ProxyCube<T1> P(X.get_ref());

  const uword local_n_rows   = P.get_n_rows();
  const uword local_n_cols   = P.get_n_cols();
  const uword local_n_slices = P.get_n_slices();

  arma_debug_assert_same_size
    (
    out.n_rows,   out.n_cols,   out.n_slices,
    local_n_rows, local_n_cols, local_n_slices,
    "Cube::set_imag()"
    );

  eT* out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    const uword N = out.n_elem;

    for(uword i=0; i<N; ++i)
      {
      //out_mem[i].imag() = PA[i];
      out_mem[i] = std::complex<T>( out_mem[i].real(), A[i] );
      }
    }
  else
    {
    for(uword slice = 0; slice < local_n_slices; ++slice)
    for(uword col   = 0; col   < local_n_cols;   ++col  )
    for(uword row   = 0; row   < local_n_rows;   ++row  )
      {
      (*out_mem) = std::complex<T>( (*out_mem).real(), P.at(row,col,slice) );
      out_mem++;
      }
    }
  }



#ifdef ARMA_EXTRA_CUBE_MEAT
  #include ARMA_INCFILE_WRAP(ARMA_EXTRA_CUBE_MEAT)
#endif



//! @}
