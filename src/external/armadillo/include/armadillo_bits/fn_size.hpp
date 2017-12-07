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


//! \addtogroup fn_size
//! @{



arma_warn_unused
inline
const SizeMat
size(const uword n_rows, const uword n_cols)
  {
  arma_extra_debug_sigprint();

  return SizeMat(n_rows, n_cols);
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_type<T1>::value, const SizeMat >::result
size(const T1& X)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(X);

  return SizeMat( P.get_n_rows(), P.get_n_cols() );
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_type<T1>::value, uword >::result
size(const T1& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(X);

  return SizeMat( P.get_n_rows(), P.get_n_cols() )( dim );
  }



arma_warn_unused
inline
const SizeCube
size(const uword n_rows, const uword n_cols, const uword n_slices)
  {
  arma_extra_debug_sigprint();

  return SizeCube(n_rows, n_cols, n_slices);
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_cube_type<T1>::value, const SizeCube >::result
size(const T1& X)
  {
  arma_extra_debug_sigprint();

  const ProxyCube<T1> P(X);

  return SizeCube( P.get_n_rows(), P.get_n_cols(), P.get_n_slices() );
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_cube_type<T1>::value, uword >::result
size(const T1& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  const ProxyCube<T1> P(X);

  return SizeCube( P.get_n_rows(), P.get_n_cols(), P.get_n_slices() )( dim );
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_sparse_type<T1>::value, const SizeMat >::result
size(const T1& X)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> P(X);

  return SizeMat( P.get_n_rows(), P.get_n_cols() );
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_sparse_type<T1>::value, uword >::result
size(const T1& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> P(X);

  return SizeMat( P.get_n_rows(), P.get_n_cols() )( dim );
  }




template<typename oT>
arma_warn_unused
inline
const SizeCube
size(const field<oT>& X)
  {
  arma_extra_debug_sigprint();

  return SizeCube( X.n_rows, X.n_cols, X.n_slices );
  }



template<typename oT>
arma_warn_unused
inline
uword
size(const field<oT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  return SizeCube( X.n_rows, X.n_cols, X.n_slices )( dim );
  }



template<typename oT>
arma_warn_unused
inline
const SizeCube
size(const subview_field<oT>& X)
  {
  arma_extra_debug_sigprint();

  return SizeCube( X.n_rows, X.n_cols, X.n_slices );
  }



template<typename oT>
arma_warn_unused
inline
uword
size(const subview_field<oT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  return SizeCube( X.n_rows, X.n_cols, X.n_slices )( dim );
  }



//! @}
