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


//! \addtogroup access
//! @{


class access
  {
  public:

  //! internal function to allow modification of data declared as read-only (use with caution)
  template<typename T1> arma_inline static T1&  rw (const T1& x)        { return const_cast<T1& >(x); }
  template<typename T1> arma_inline static T1*& rwp(const T1* const& x) { return const_cast<T1*&>(x); }

  //! internal function to obtain the real part of either a plain number or a complex number
  template<typename eT> arma_inline static const eT& tmp_real(const eT&              X) { return X;        }
  template<typename  T> arma_inline static const   T tmp_real(const std::complex<T>& X) { return X.real(); }

  //! internal function to work around braindead compilers
  template<typename eT> arma_inline static const typename enable_if2<is_not_complex<eT>::value, const eT&>::result alt_conj(const eT& X) { return X;            }
  template<typename eT> arma_inline static const typename enable_if2<    is_complex<eT>::value, const eT >::result alt_conj(const eT& X) { return std::conj(X); }
  };


//! @}
