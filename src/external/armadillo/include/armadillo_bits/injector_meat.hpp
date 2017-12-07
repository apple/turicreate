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


//! \addtogroup injector
//! @{



template<typename eT>
inline
mat_injector_row<eT>::mat_injector_row()
  : n_cols(0)
  {
  arma_extra_debug_sigprint();

  A.set_size( podarray_prealloc_n_elem::val );
  }



template<typename eT>
inline
void
mat_injector_row<eT>::insert(const eT val) const
  {
  arma_extra_debug_sigprint();

  if(n_cols < A.n_elem)
    {
    A[n_cols] = val;
    ++n_cols;
    }
  else
    {
    B.set_size(2 * A.n_elem);

    arrayops::copy(B.memptr(), A.memptr(), n_cols);

    B[n_cols] = val;
    ++n_cols;

    std::swap( access::rw(A.mem),    access::rw(B.mem)    );
    std::swap( access::rw(A.n_elem), access::rw(B.n_elem) );
    }
  }



//
//
//



template<typename T1>
inline
mat_injector<T1>::mat_injector(T1& in_X, const typename mat_injector<T1>::elem_type val)
  : X(in_X)
  , n_rows(1)
  {
  arma_extra_debug_sigprint();

  typedef typename mat_injector<T1>::elem_type eT;

  AA = new podarray< mat_injector_row<eT>* >;
  BB = new podarray< mat_injector_row<eT>* >;

  podarray< mat_injector_row<eT>* >& A = *AA;

  A.set_size(n_rows);

  for(uword row=0; row<n_rows; ++row)
    {
    A[row] = new mat_injector_row<eT>;
    }

  (*(A[0])).insert(val);
  }



template<typename T1>
inline
mat_injector<T1>::mat_injector(T1& in_X, const injector_end_of_row<>& x)
  : X(in_X)
  , n_rows(1)
  {
  arma_extra_debug_sigprint();
  arma_ignore(x);

  typedef typename mat_injector<T1>::elem_type eT;

  AA = new podarray< mat_injector_row<eT>* >;
  BB = new podarray< mat_injector_row<eT>* >;

  podarray< mat_injector_row<eT>* >& A = *AA;

  A.set_size(n_rows);

  for(uword row=0; row<n_rows; ++row)
    {
    A[row] = new mat_injector_row<eT>;
    }

  (*this).end_of_row();
  }



template<typename T1>
inline
mat_injector<T1>::~mat_injector()
  {
  arma_extra_debug_sigprint();

  typedef typename mat_injector<T1>::elem_type eT;

  podarray< mat_injector_row<eT>* >& A = *AA;

  if(n_rows > 0)
    {
    uword max_n_cols = (*(A[0])).n_cols;

    for(uword row=1; row<n_rows; ++row)
      {
      const uword n_cols = (*(A[row])).n_cols;

      if(max_n_cols < n_cols)
        {
        max_n_cols = n_cols;
        }
      }

    const uword max_n_rows = ((*(A[n_rows-1])).n_cols == 0) ? n_rows-1 : n_rows;

    if(is_Mat_only<T1>::value == true)
      {
      X.set_size(max_n_rows, max_n_cols);

      for(uword row=0; row<max_n_rows; ++row)
        {
        const uword n_cols = (*(A[row])).n_cols;

        for(uword col=0; col<n_cols; ++col)
          {
          X.at(row,col) = (*(A[row])).A[col];
          }

        for(uword col=n_cols; col<max_n_cols; ++col)
          {
          X.at(row,col) = eT(0);
          }
        }
      }
    else
    if(is_Row<T1>::value == true)
      {
      arma_debug_check( (max_n_rows > 1), "matrix initialisation: incompatible dimensions" );

      const uword n_cols = (*(A[0])).n_cols;

      X.set_size(1, n_cols);

      arrayops::copy( X.memptr(), (*(A[0])).A.memptr(), n_cols );
      }
    else
    if(is_Col<T1>::value == true)
      {
      const bool is_vec = ( (max_n_rows == 1) || (max_n_cols == 1) );

      arma_debug_check( (is_vec == false), "matrix initialisation: incompatible dimensions" );

      const uword n_elem = (std::max)(max_n_rows, max_n_cols);

      X.set_size(n_elem, 1);

      uword i = 0;
      for(uword row=0; row<max_n_rows; ++row)
        {
        const uword n_cols = (*(A[0])).n_cols;

        for(uword col=0; col<n_cols; ++col)
          {
          X[i] = (*(A[row])).A[col];
          ++i;
          }

        for(uword col=n_cols; col<max_n_cols; ++col)
          {
          X[i] = eT(0);
          ++i;
          }
        }
      }
    }

  for(uword row=0; row<n_rows; ++row)
    {
    delete A[row];
    }

  delete AA;
  delete BB;
  }



template<typename T1>
inline
void
mat_injector<T1>::insert(const typename mat_injector<T1>::elem_type val) const
  {
  arma_extra_debug_sigprint();

  typedef typename mat_injector<T1>::elem_type eT;

  podarray< mat_injector_row<eT>* >& A = *AA;

  (*(A[n_rows-1])).insert(val);
  }




template<typename T1>
inline
void
mat_injector<T1>::end_of_row() const
  {
  arma_extra_debug_sigprint();

  typedef typename mat_injector<T1>::elem_type eT;

  podarray< mat_injector_row<eT>* >& A = *AA;
  podarray< mat_injector_row<eT>* >& B = *BB;

  B.set_size( n_rows+1 );

  arrayops::copy(B.memptr(), A.memptr(), n_rows);

  for(uword row=n_rows; row<(n_rows+1); ++row)
    {
    B[row] = new mat_injector_row<eT>;
    }

  std::swap(AA, BB);

  n_rows += 1;
  }



template<typename T1>
arma_inline
const mat_injector<T1>&
operator<<(const mat_injector<T1>& ref, const typename mat_injector<T1>::elem_type val)
  {
  arma_extra_debug_sigprint();

  ref.insert(val);

  return ref;
  }



template<typename T1>
arma_inline
const mat_injector<T1>&
operator<<(const mat_injector<T1>& ref, const injector_end_of_row<>& x)
  {
  arma_extra_debug_sigprint();
  arma_ignore(x);

  ref.end_of_row();

  return ref;
  }



//// using a mixture of operator << and , doesn't work yet
//// e.g. A << 1, 2, 3 << endr
//// in the above "3 << endr" requires special handling.
//// similarly, special handling is necessary for "endr << 3"
////
// template<typename T1>
// arma_inline
// const mat_injector<T1>&
// operator,(const mat_injector<T1>& ref, const typename mat_injector<T1>::elem_type val)
//   {
//   arma_extra_debug_sigprint();
//
//   ref.insert(val);
//
//   return ref;
//   }



// template<typename T1>
// arma_inline
// const mat_injector<T1>&
// operator,(const mat_injector<T1>& ref, const injector_end_of_row<>& x)
//   {
//   arma_extra_debug_sigprint();
//   arma_ignore(x);
//
//   ref.end_of_row();
//
//   return ref;
//   }




//
//
//



template<typename oT>
inline
field_injector_row<oT>::field_injector_row()
  : n_cols(0)
  {
  arma_extra_debug_sigprint();

  AA = new field<oT>;
  BB = new field<oT>;

  field<oT>& A = *AA;

  A.set_size( field_prealloc_n_elem::val );
  }



template<typename oT>
inline
field_injector_row<oT>::~field_injector_row()
  {
  arma_extra_debug_sigprint();

  delete AA;
  delete BB;
  }



template<typename oT>
inline
void
field_injector_row<oT>::insert(const oT& val) const
  {
  arma_extra_debug_sigprint();

  field<oT>& A = *AA;
  field<oT>& B = *BB;

  if(n_cols < A.n_elem)
    {
    A[n_cols] = val;
    ++n_cols;
    }
  else
    {
    B.set_size(2 * A.n_elem);

    for(uword i=0; i<n_cols; ++i)
      {
      B[i] = A[i];
      }

    B[n_cols] = val;
    ++n_cols;

    std::swap(AA, BB);
    }
  }



//
//
//


template<typename T1>
inline
field_injector<T1>::field_injector(T1& in_X, const typename field_injector<T1>::object_type& val)
  : X(in_X)
  , n_rows(1)
  {
  arma_extra_debug_sigprint();

  typedef typename field_injector<T1>::object_type oT;

  AA = new podarray< field_injector_row<oT>* >;
  BB = new podarray< field_injector_row<oT>* >;

  podarray< field_injector_row<oT>* >& A = *AA;

  A.set_size(n_rows);

  for(uword row=0; row<n_rows; ++row)
    {
    A[row] = new field_injector_row<oT>;
    }

  (*(A[0])).insert(val);
  }



template<typename T1>
inline
field_injector<T1>::field_injector(T1& in_X, const injector_end_of_row<>& x)
  : X(in_X)
  , n_rows(1)
  {
  arma_extra_debug_sigprint();
  arma_ignore(x);

  typedef typename field_injector<T1>::object_type oT;

  AA = new podarray< field_injector_row<oT>* >;
  BB = new podarray< field_injector_row<oT>* >;

  podarray< field_injector_row<oT>* >& A = *AA;

  A.set_size(n_rows);

  for(uword row=0; row<n_rows; ++row)
    {
    A[row] = new field_injector_row<oT>;
    }

  (*this).end_of_row();
  }



template<typename T1>
inline
field_injector<T1>::~field_injector()
  {
  arma_extra_debug_sigprint();

  typedef typename field_injector<T1>::object_type oT;

  podarray< field_injector_row<oT>* >& A = *AA;

  if(n_rows > 0)
    {
    uword max_n_cols = (*(A[0])).n_cols;

    for(uword row=1; row<n_rows; ++row)
      {
      const uword n_cols = (*(A[row])).n_cols;

      if(max_n_cols < n_cols)
        {
        max_n_cols = n_cols;
        }
      }

    const uword max_n_rows = ((*(A[n_rows-1])).n_cols == 0) ? n_rows-1 : n_rows;

    X.set_size(max_n_rows, max_n_cols);

    for(uword row=0; row<max_n_rows; ++row)
      {
      const uword n_cols = (*(A[row])).n_cols;

      for(uword col=0; col<n_cols; ++col)
        {
        const field<oT>& tmp = *((*(A[row])).AA);
        X.at(row,col) = tmp[col];
        }

      for(uword col=n_cols; col<max_n_cols; ++col)
        {
        X.at(row,col) = oT();
        }
      }
    }


  for(uword row=0; row<n_rows; ++row)
    {
    delete A[row];
    }

  delete AA;
  delete BB;
  }



template<typename T1>
inline
void
field_injector<T1>::insert(const typename field_injector<T1>::object_type& val) const
  {
  arma_extra_debug_sigprint();

  typedef typename field_injector<T1>::object_type oT;

  podarray< field_injector_row<oT>* >& A = *AA;

  (*(A[n_rows-1])).insert(val);
  }




template<typename T1>
inline
void
field_injector<T1>::end_of_row() const
  {
  arma_extra_debug_sigprint();

  typedef typename field_injector<T1>::object_type oT;

  podarray< field_injector_row<oT>* >& A = *AA;
  podarray< field_injector_row<oT>* >& B = *BB;

  B.set_size( n_rows+1 );

  for(uword row=0; row<n_rows; ++row)
    {
    B[row] = A[row];
    }

  for(uword row=n_rows; row<(n_rows+1); ++row)
    {
    B[row] = new field_injector_row<oT>;
    }

  std::swap(AA, BB);

  n_rows += 1;
  }



template<typename T1>
arma_inline
const field_injector<T1>&
operator<<(const field_injector<T1>& ref, const typename field_injector<T1>::object_type& val)
  {
  arma_extra_debug_sigprint();

  ref.insert(val);

  return ref;
  }



template<typename T1>
arma_inline
const field_injector<T1>&
operator<<(const field_injector<T1>& ref, const injector_end_of_row<>& x)
  {
  arma_extra_debug_sigprint();
  arma_ignore(x);

  ref.end_of_row();

  return ref;
  }



//! @}
