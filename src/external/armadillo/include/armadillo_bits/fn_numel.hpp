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


//! \addtogroup fn_numel
//! @{



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_type<T1>::value, uword >::result
numel(const T1& X)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(X);

  return P.get_n_elem();
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_cube_type<T1>::value, uword >::result
numel(const T1& X)
  {
  arma_extra_debug_sigprint();

  const ProxyCube<T1> P(X);

  return P.get_n_elem();
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_arma_sparse_type<T1>::value, uword >::result
numel(const T1& X)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> P(X);

  return P.get_n_elem();
  }



template<typename oT>
arma_warn_unused
inline
uword
numel(const field<oT>& X)
  {
  arma_extra_debug_sigprint();

  return X.n_elem;
  }



template<typename oT>
arma_warn_unused
inline
uword
numel(const subview_field<oT>& X)
  {
  arma_extra_debug_sigprint();

  return X.n_elem;
  }



//! @}
