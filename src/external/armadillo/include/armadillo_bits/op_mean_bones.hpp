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


//! \addtogroup op_mean
//! @{


//! Class for finding mean values of a matrix
class op_mean
  {
  public:

  // dense matrices

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_mean>& in);

  template<typename T1>
  inline static void apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim);

  template<typename T1>
  inline static void apply_noalias_unwrap(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim);

  template<typename T1>
  inline static void apply_noalias_proxy(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim);


  // cubes

  template<typename T1>
  inline static void apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_mean>& in);

  template<typename T1>
  inline static void apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim);

  template<typename T1>
  inline static void apply_noalias_unwrap(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim);

  template<typename T1>
  inline static void apply_noalias_proxy(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim);


  //

  template<typename eT>
  inline static eT direct_mean(const eT* const X, const uword N);

  template<typename eT>
  inline static eT direct_mean_robust(const eT* const X, const uword N);


  //

  template<typename eT>
  inline static eT direct_mean(const Mat<eT>& X, const uword row);

  template<typename eT>
  inline static eT direct_mean_robust(const Mat<eT>& X, const uword row);


  //

  template<typename eT>
  inline static eT mean_all(const subview<eT>& X);

  template<typename eT>
  inline static eT mean_all_robust(const subview<eT>& X);


  //

  template<typename eT>
  inline static eT mean_all(const diagview<eT>& X);

  template<typename eT>
  inline static eT mean_all_robust(const diagview<eT>& X);


  //

  template<typename T1>
  inline static typename T1::elem_type mean_all(const Op<T1,op_vectorise_col>& X);

  template<typename T1>
  inline static typename T1::elem_type mean_all(const Base<typename T1::elem_type, T1>& X);


  //

  template<typename eT>
  arma_inline static eT robust_mean(const eT A, const eT B);

  template<typename T>
  arma_inline static std::complex<T> robust_mean(const std::complex<T>& A, const std::complex<T>& B);
  };



//! @}
