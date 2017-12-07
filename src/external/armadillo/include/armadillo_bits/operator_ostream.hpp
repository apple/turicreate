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


//! \addtogroup operator_ostream
//! @{



template<typename eT, typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(X.get_ref());

  arma_ostream::print(o, tmp.M, true);

  return o;
  }



template<typename eT, typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const SpBase<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap_spmat<T1> tmp(X.get_ref());

  arma_ostream::print(o, tmp.M, true);

  return o;
  }



template<typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const SpValProxy<T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  o << eT(X);

  return o;
  }



template<typename eT>
inline
std::ostream&
operator<< (std::ostream& o, const MapMat_val<eT>& X)
  {
  arma_extra_debug_sigprint();

  o << eT(X);

  return o;
  }



template<typename eT>
inline
std::ostream&
operator<< (std::ostream& o, const MapMat_elem<eT>& X)
  {
  arma_extra_debug_sigprint();

  o << eT(X);

  return o;
  }



template<typename eT>
inline
std::ostream&
operator<< (std::ostream& o, const MapMat_svel<eT>& X)
  {
  arma_extra_debug_sigprint();

  o << eT(X);

  return o;
  }



template<typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(X.get_ref());

  arma_ostream::print(o, tmp.M, true);

  return o;
  }



//! Print the contents of a field to the specified stream.
template<typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const field<T1>& X)
  {
  arma_extra_debug_sigprint();

  arma_ostream::print(o, X);

  return o;
  }



//! Print the contents of a subfield to the specified stream
template<typename T1>
inline
std::ostream&
operator<< (std::ostream& o, const subview_field<T1>& X)
  {
  arma_extra_debug_sigprint();

  arma_ostream::print(o, X);

  return o;
  }



inline
std::ostream&
operator<< (std::ostream& o, const SizeMat& S)
  {
  arma_extra_debug_sigprint();

  arma_ostream::print(o, S);

  return o;
  }



inline
std::ostream&
operator<< (std::ostream& o, const SizeCube& S)
  {
  arma_extra_debug_sigprint();

  arma_ostream::print(o, S);

  return o;
  }



//! @}
