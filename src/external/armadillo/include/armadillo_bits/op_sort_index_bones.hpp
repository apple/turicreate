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


//! \addtogroup op_sort_index
//! @{



class op_sort_index
  {
  public:

  template<typename T1>
  static inline bool apply_noalias(Mat<uword>& out, const Proxy<T1>& P, const uword sort_type);

  template<typename T1>
  static inline void apply(Mat<uword>& out, const mtOp<uword,T1,op_sort_index>& in);
  };



class op_stable_sort_index
  {
  public:

  template<typename T1>
  static inline bool apply_noalias(Mat<uword>& out, const Proxy<T1>& P, const uword sort_type);

  template<typename T1>
  static inline void apply(Mat<uword>& out, const mtOp<uword,T1,op_stable_sort_index>& in);
  };



template<typename eT>
struct arma_sort_index_packet
  {
  eT    val;
  uword index;
  };



template<typename eT>
struct arma_sort_index_helper_ascend
  {
  arma_inline
  bool
  operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
    {
    return (A.val < B.val);
    }
  };



template<typename eT>
struct arma_sort_index_helper_descend
  {
  arma_inline
  bool
  operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
    {
    return (A.val > B.val);
    }
  };



template<typename T>
struct arma_sort_index_helper_ascend< std::complex<T> >
  {
  typedef typename std::complex<T> eT;

  inline
  bool
  operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
    {
    return (std::abs(A.val) < std::abs(B.val));
    }

  // inline
  // bool
  // operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
  //   {
  //   const T abs_A_val = std::abs(A.val);
  //   const T abs_B_val = std::abs(B.val);
  //
  //   return ( (abs_A_val != abs_B_val) ? (abs_A_val < abs_B_val) : (std::arg(A.val) < std::arg(B.val)) );
  //   }
  };



template<typename T>
struct arma_sort_index_helper_descend< std::complex<T> >
  {
  typedef typename std::complex<T> eT;

  inline
  bool
  operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
    {
    return (std::abs(A.val) > std::abs(B.val));
    }

  // inline
  // bool
  // operator() (const arma_sort_index_packet<eT>& A, const arma_sort_index_packet<eT>& B) const
  //   {
  //   const T abs_A_val = std::abs(A.val);
  //   const T abs_B_val = std::abs(B.val);
  //
  //   return ( (abs_A_val != abs_B_val) ? (abs_A_val > abs_B_val) : (std::arg(A.val) > std::arg(B.val)) );
  //   }
  };



//! @}
