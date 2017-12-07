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


//! \addtogroup op_sort
//! @{



class op_sort
  {
  public:

  template<typename eT>
  inline static void copy_row(eT* X, const Mat<eT>& A, const uword row);

  template<typename eT>
  inline static void copy_row(Mat<eT>& A, const eT* X, const uword row);

  template<typename eT>
  inline static void direct_sort(eT* X, const uword N, const uword sort_type = 0);

  template<typename eT>
  inline static void direct_sort_ascending(eT* X, const uword N);

  template<typename eT>
  inline static void apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword sort_type, const uword dim);

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_sort>& in);
  };



class op_sort_default
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_sort_default>& in);
  };



template<typename eT>
struct arma_ascend_sort_helper
  {
  arma_inline bool operator() (const eT a, const eT b) const { return (a < b); }
  };



template<typename eT>
struct arma_descend_sort_helper
  {
  arma_inline bool operator() (const eT a, const eT b) const { return (a > b); }
  };



template<typename T>
struct arma_ascend_sort_helper< std::complex<T> >
  {
  typedef typename std::complex<T> eT;

  inline bool operator() (const eT& a, const eT& b) const { return (std::abs(a) < std::abs(b)); }

  // inline
  // bool
  // operator() (const eT& a, const eT& b) const
  //   {
  //   const T abs_a = std::abs(a);
  //   const T abs_b = std::abs(b);
  //
  //   return ( (abs_a != abs_b) ? (abs_a < abs_b) : (std::arg(a) < std::arg(b)) );
  //   }
  };



template<typename T>
struct arma_descend_sort_helper< std::complex<T> >
  {
  typedef typename std::complex<T> eT;

  inline bool operator() (const eT& a, const eT& b) const { return (std::abs(a) > std::abs(b)); }

  // inline
  // bool
  // operator() (const eT& a, const eT& b) const
  //   {
  //   const T abs_a = std::abs(a);
  //   const T abs_b = std::abs(b);
  //
  //   return ( (abs_a != abs_b) ? (abs_a > abs_b) : (std::arg(a) > std::arg(b)) );
  //   }
  };



//! @}
