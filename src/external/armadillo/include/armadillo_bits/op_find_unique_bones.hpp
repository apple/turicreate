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


//! \addtogroup op_find_unique
//! @{



class op_find_unique
  {
  public:

  template<typename T1>
  static inline bool apply_helper(Mat<uword>& out, const Proxy<T1>& P, const bool ascending_indices);

  template<typename T1>
  static inline void apply(Mat<uword>& out, const mtOp<uword,T1,op_find_unique>& in);
  };



template<typename eT>
struct arma_find_unique_packet
  {
  eT    val;
  uword index;
  };



template<typename eT>
struct arma_find_unique_comparator
  {
  arma_inline
  bool
  operator() (const arma_find_unique_packet<eT>& A, const arma_find_unique_packet<eT>& B) const
    {
    return (A.val < B.val);
    }
  };



template<typename T>
struct arma_find_unique_comparator< std::complex<T> >
  {
  arma_inline
  bool
  operator() (const arma_find_unique_packet< std::complex<T> >& A, const arma_find_unique_packet< std::complex<T> >& B) const
    {
    const T A_real = A.val.real();
    const T B_real = B.val.real();

    return (  (A_real < B_real) ? true : ((A_real == B_real) ? (A.val.imag() < B.val.imag()) : false)  );
    }
  };



//! @}
