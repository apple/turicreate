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


//! \addtogroup podarray
//! @{


template<typename eT>
arma_hot
inline
podarray<eT>::~podarray()
  {
  arma_extra_debug_sigprint_this(this);

  if(n_elem > podarray_prealloc_n_elem::val )
    {
    memory::release( mem );
    }
  }



template<typename eT>
inline
podarray<eT>::podarray()
  : n_elem(0)
  , mem   (0)
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename eT>
inline
podarray<eT>::podarray(const podarray& x)
  : n_elem(x.n_elem)
  {
  arma_extra_debug_sigprint();

  const uword x_n_elem = x.n_elem;

  init_cold(x_n_elem);

  arrayops::copy( memptr(), x.memptr(), x_n_elem );
  }



template<typename eT>
inline
const podarray<eT>&
podarray<eT>::operator=(const podarray& x)
  {
  arma_extra_debug_sigprint();

  if(this != &x)
    {
    const uword x_n_elem = x.n_elem;

    init_warm(x_n_elem);

    arrayops::copy( memptr(), x.memptr(), x_n_elem );
    }

  return *this;
  }



template<typename eT>
arma_hot
arma_inline
podarray<eT>::podarray(const uword new_n_elem)
  : n_elem(new_n_elem)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold(new_n_elem);
  }



template<typename eT>
arma_inline
podarray<eT>::podarray(const eT* X, const uword new_n_elem)
  : n_elem(new_n_elem)
  {
  arma_extra_debug_sigprint_this(this);

  init_cold(new_n_elem);

  arrayops::copy( memptr(), X, new_n_elem );
  }



template<typename eT>
template<typename T1>
inline
podarray<eT>::podarray(const Proxy<T1>& P)
  : n_elem(P.get_n_elem())
  {
  arma_extra_debug_sigprint_this(this);

  const uword P_n_elem = P.get_n_elem();

  init_cold(P_n_elem);

  eT* out_mem = (*this).memptr();

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    uword i,j;
    for(i=0, j=1; j < P_n_elem; i+=2, j+=2)
      {
      const eT val_i = A[i];
      const eT val_j = A[j];

      out_mem[i] = val_i;
      out_mem[j] = val_j;
      }

    if(i < P_n_elem)
      {
      out_mem[i] = A[i];
      }
    }
  else
    {
    const uword P_n_rows = P.get_n_rows();
    const uword P_n_cols = P.get_n_cols();

    if(P_n_rows != 1)
      {
      uword count = 0;

      for(uword col=0; col < P_n_cols; ++col)
      for(uword row=0; row < P_n_rows; ++row, ++count)
        {
        out_mem[count] = P.at(row,col);
        }
      }
    else
      {
      for(uword col=0; col < P_n_cols; ++col)
        {
        out_mem[col] = P.at(0,col);
        }
      }
    }
  }



template<typename eT>
arma_inline
eT
podarray<eT>::operator[] (const uword i) const
  {
  return mem[i];
  }



template<typename eT>
arma_inline
eT&
podarray<eT>::operator[] (const uword i)
  {
  return access::rw(mem[i]);
  }



template<typename eT>
arma_inline
eT
podarray<eT>::operator() (const uword i) const
  {
  arma_debug_check( (i >= n_elem), "podarray::operator(): index out of bounds");

  return mem[i];
  }



template<typename eT>
arma_inline
eT&
podarray<eT>::operator() (const uword i)
  {
  arma_debug_check( (i >= n_elem), "podarray::operator(): index out of bounds");

  return access::rw(mem[i]);
  }



template<typename eT>
inline
void
podarray<eT>::set_min_size(const uword min_n_elem)
  {
  arma_extra_debug_sigprint();

  if(min_n_elem > n_elem)
    {
    init_warm(min_n_elem);
    }
  }



template<typename eT>
inline
void
podarray<eT>::set_size(const uword new_n_elem)
  {
  arma_extra_debug_sigprint();

  init_warm(new_n_elem);
  }



template<typename eT>
inline
void
podarray<eT>::reset()
  {
  arma_extra_debug_sigprint();

  init_warm(0);
  }



template<typename eT>
inline
void
podarray<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_set(memptr(), val, n_elem);
  }



template<typename eT>
inline
void
podarray<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  arrayops::fill_zeros(memptr(), n_elem);
  }



template<typename eT>
inline
void
podarray<eT>::zeros(const uword new_n_elem)
  {
  arma_extra_debug_sigprint();

  init_warm(new_n_elem);

  arrayops::fill_zeros(memptr(), n_elem);
  }



template<typename eT>
arma_inline
eT*
podarray<eT>::memptr()
  {
  return mem;
  }



template<typename eT>
arma_inline
const eT*
podarray<eT>::memptr() const
  {
  return mem;
  }



template<typename eT>
arma_hot
inline
void
podarray<eT>::copy_row(const Mat<eT>& A, const uword row)
  {
  const uword cols = A.n_cols;

  // note: this function assumes that the podarray has been set to the correct size beforehand
  eT* out = memptr();

  switch(cols)
    {
    default:
      {
      uword i,j;
      for(i=0, j=1; j < cols; i+=2, j+=2)
        {
        const eT tmp_i = A.at(row, i);
        const eT tmp_j = A.at(row, j);

        out[i] = tmp_i;
        out[j] = tmp_j;
        }

      if(i < cols)
        {
        out[i] = A.at(row, i);
        }
      }
      break;

    case 8:  out[7] = A.at(row, 7);
    case 7:  out[6] = A.at(row, 6);
    case 6:  out[5] = A.at(row, 5);
    case 5:  out[4] = A.at(row, 4);
    case 4:  out[3] = A.at(row, 3);
    case 3:  out[2] = A.at(row, 2);
    case 2:  out[1] = A.at(row, 1);
    case 1:  out[0] = A.at(row, 0);
    case 0:  ;
    }
  }


template<typename eT>
arma_hot
inline
void
podarray<eT>::init_cold(const uword new_n_elem)
  {
  arma_extra_debug_sigprint();

  if(new_n_elem <= podarray_prealloc_n_elem::val )
    {
    mem = mem_local;
    }
  else
    {
    mem = memory::acquire<eT>(new_n_elem);
    }
  }



template<typename eT>
inline
void
podarray<eT>::init_warm(const uword new_n_elem)
  {
  arma_extra_debug_sigprint();

  if(n_elem == new_n_elem)
    {
    return;
    }

  if(n_elem > podarray_prealloc_n_elem::val )
    {
    memory::release( mem );
    }

  if(new_n_elem <= podarray_prealloc_n_elem::val )
    {
    mem = mem_local;
    }
  else
    {
    mem = memory::acquire<eT>(new_n_elem);
    }

  access::rw(n_elem) = new_n_elem;
  }



//! @}
