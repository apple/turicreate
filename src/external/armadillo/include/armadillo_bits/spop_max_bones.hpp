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


//! \addtogroup spop_max
//! @{


class spop_max
  {
  public:

  template<typename T1>
  inline static void apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_max>& in);

  //

  template<typename T1>
  inline static void apply_proxy(SpMat<typename T1::elem_type>& out, const SpProxy<T1>& p, const uword dim, const typename arma_not_cx<typename T1::elem_type>::result* junk = 0);

  template<typename T1>
  inline static typename T1::elem_type vector_max(const T1& X, const typename arma_not_cx<typename T1::elem_type>::result* junk = 0);

  template<typename T1>
  inline static typename arma_not_cx<typename T1::elem_type>::result max(const SpBase<typename T1::elem_type, T1>& X);

  template<typename T1>
  inline static typename arma_not_cx<typename T1::elem_type>::result max_with_index(const SpProxy<T1>& P, uword& index_of_max_val);

  //

  template<typename T1>
  inline static void apply_proxy(SpMat<typename T1::elem_type>& out, const SpProxy<T1>& p, const uword dim, const typename arma_cx_only<typename T1::elem_type>::result* junk = 0);

  template<typename T1>
  inline static typename T1::elem_type vector_max(const T1& X, const typename arma_cx_only<typename T1::elem_type>::result* junk = 0);

  template<typename T1>
  inline static typename arma_cx_only<typename T1::elem_type>::result max(const SpBase<typename T1::elem_type, T1>& X);

  template<typename T1>
  inline static typename arma_cx_only<typename T1::elem_type>::result max_with_index(const SpProxy<T1>& P, uword& index_of_max_val);
  };


//! @}
