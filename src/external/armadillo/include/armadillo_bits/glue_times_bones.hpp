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


//! \addtogroup glue_times
//! @{



//! \brief
//! Template metaprogram depth_lhs
//! calculates the number of Glue<Tx,Ty, glue_type> instances on the left hand side argument of Glue<Tx,Ty, glue_type>
//! i.e. it recursively expands each Tx, until the type of Tx is not "Glue<..,.., glue_type>"  (i.e the "glue_type" changes)

template<typename glue_type, typename T1>
struct depth_lhs
  {
  static const uword num = 0;
  };

template<typename glue_type, typename T1, typename T2>
struct depth_lhs< glue_type, Glue<T1,T2,glue_type> >
  {
  static const uword num = 1 + depth_lhs<glue_type, T1>::num;
  };



template<bool do_inv_detect>
struct glue_times_redirect2_helper
  {
  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_times>& X);
  };


template<>
struct glue_times_redirect2_helper<true>
  {
  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_times>& X);
  };



template<bool do_inv_detect>
struct glue_times_redirect3_helper
  {
  template<typename T1, typename T2, typename T3>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue< Glue<T1,T2,glue_times>,T3,glue_times>& X);
  };


template<>
struct glue_times_redirect3_helper<true>
  {
  template<typename T1, typename T2, typename T3>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue< Glue<T1,T2,glue_times>,T3,glue_times>& X);
  };



template<uword N>
struct glue_times_redirect
  {
  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_times>& X);
  };


template<>
struct glue_times_redirect<2>
  {
  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_times>& X);
  };


template<>
struct glue_times_redirect<3>
  {
  template<typename T1, typename T2, typename T3>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue< Glue<T1,T2,glue_times>,T3,glue_times>& X);
  };


template<>
struct glue_times_redirect<4>
  {
  template<typename T1, typename T2, typename T3, typename T4>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue< Glue< Glue<T1,T2,glue_times>, T3, glue_times>, T4, glue_times>& X);
  };



//! Class which implements the immediate multiplication of two or more matrices
class glue_times
  {
  public:


  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_times>& X);


  template<typename T1>
  arma_hot inline static void apply_inplace(Mat<typename T1::elem_type>& out, const T1& X);

  template<typename T1, typename T2>
  arma_hot inline static void apply_inplace_plus(Mat<typename T1::elem_type>& out, const Glue<T1, T2, glue_times>& X, const sword sign);

  //

  template<typename eT, const bool do_trans_A, const bool do_trans_B, typename TA, typename TB>
  arma_inline static uword mul_storage_cost(const TA& A, const TB& B);

  template<typename eT, const bool do_trans_A, const bool do_trans_B, const bool do_scalar_times, typename TA, typename TB>
  arma_hot inline static void apply(Mat<eT>& out, const TA& A, const TB& B, const eT val);

  template<typename eT, const bool do_trans_A, const bool do_trans_B, const bool do_trans_C, const bool do_scalar_times, typename TA, typename TB, typename TC>
  arma_hot inline static void apply(Mat<eT>& out, const TA& A, const TB& B, const TC& C, const eT val);

  template<typename eT, const bool do_trans_A, const bool do_trans_B, const bool do_trans_C, const bool do_trans_D, const bool do_scalar_times, typename TA, typename TB, typename TC, typename TD>
  arma_hot inline static void apply(Mat<eT>& out, const TA& A, const TB& B, const TC& C, const TD& D, const eT val);
  };



class glue_times_diag
  {
  public:

  template<typename T1, typename T2>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1, T2, glue_times_diag>& X);

  };



//! @}
