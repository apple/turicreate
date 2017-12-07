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


//! \addtogroup op_median
//! @{


template<typename T>
struct arma_cx_median_packet
  {
  T     val;
  uword index;
  };



template<typename T>
arma_inline
bool
operator< (const arma_cx_median_packet<T>& A, const arma_cx_median_packet<T>& B)
  {
  return A.val < B.val;
  }



//! Class for finding median values of a matrix
class op_median
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_median>& in);

  template<typename T, typename T1>
  inline static void apply(Mat< std::complex<T> >& out, const Op<T1,op_median>& in);

  //
  //

  template<typename T1>
  inline static typename T1::elem_type median_vec(const T1& X, const typename arma_not_cx<typename T1::elem_type>::result* junk = 0);

  template<typename T1>
  inline static typename T1::elem_type median_vec(const T1& X, const typename arma_cx_only<typename T1::elem_type>::result* junk = 0);

  //
  //

  template<typename eT>
  inline static eT direct_median(std::vector<eT>& X);

  template<typename T>
  inline static void direct_cx_median_index(uword& out_index1, uword& out_index2, std::vector< arma_cx_median_packet<T> >& X);
  };



//! @}
