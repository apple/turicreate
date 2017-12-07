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


//! \addtogroup spop_var
//! @{



//! Class for finding variance values of a sparse matrix
class spop_var
  {
  public:

  template<typename T1>
  inline static void apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_var>& in);

  template<typename T1>
  inline static void apply_noalias(SpMat<typename T1::pod_type>& out, const SpProxy<T1>& p, const uword norm_type, const uword dim);

  // Calculate variance of a sparse vector, where we can directly use the memory.
  template<typename T1>
  inline static typename T1::pod_type var_vec(const T1& X, const uword norm_type = 0);

  // Calculate the variance directly.  Because this is for sparse matrices, we
  // specify both the number of elements in the array (the length of the array)
  // as well as the actual number of elements when zeros are included.
  template<typename eT>
  inline static eT direct_var(const eT* const X, const uword length, const uword N, const uword norm_type = 0);

  // For complex numbers.

  template<typename T>
  inline static T direct_var(const std::complex<T>* const X, const uword length, const uword N, const uword norm_type = 0);

  // Calculate the variance using iterators, for non-complex numbers.
  template<typename T1, typename eT>
  inline static eT iterator_var(T1& it, const T1& end, const uword n_zero, const uword norm_type, const eT junk1, const typename arma_not_cx<eT>::result* junk2 = 0);

  // Calculate the variance using iterators, for complex numbers.
  template<typename T1, typename eT>
  inline static typename get_pod_type<eT>::result iterator_var(T1& it, const T1& end, const uword n_zero, const uword norm_type, const eT junk1, const typename arma_cx_only<eT>::result* junk2 = 0);

  };



//! @}
