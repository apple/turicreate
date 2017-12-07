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


//! \addtogroup op_strans
//! @{


//! 'matrix transpose' operation (simple transpose, ie. without taking the conjugate of the elements)

class op_strans
  {
  public:

  template<const bool do_flip, const uword row, const uword col>
  struct pos
    {
    static const uword n2 = (do_flip == false) ? (row + col*2) : (col + row*2);
    static const uword n3 = (do_flip == false) ? (row + col*3) : (col + row*3);
    static const uword n4 = (do_flip == false) ? (row + col*4) : (col + row*4);
    };

  template<typename eT, typename TA>
  arma_hot inline static void apply_mat_noalias_tinysq(Mat<eT>& out, const TA& A);

  template<typename eT, typename TA>
  arma_hot inline static void apply_mat_noalias(Mat<eT>& out, const TA& A);

  template<typename eT>
  arma_hot inline static void apply_mat_inplace(Mat<eT>& out);

  template<typename eT, typename TA>
  arma_hot inline static void apply_mat(Mat<eT>& out, const TA& A);

  template<typename T1>
  arma_hot inline static void apply_proxy(Mat<typename T1::elem_type>& out, const T1& X);

  template<typename T1>
  arma_hot inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_strans>& in);
  };



class op_strans2
  {
  public:

  template<const bool do_flip, const uword row, const uword col>
  struct pos
    {
    static const uword n2 = (do_flip == false) ? (row + col*2) : (col + row*2);
    static const uword n3 = (do_flip == false) ? (row + col*3) : (col + row*3);
    static const uword n4 = (do_flip == false) ? (row + col*4) : (col + row*4);
    };

  template<typename eT, typename TA>
  arma_hot inline static void apply_noalias_tinysq(Mat<eT>& out, const TA& A, const eT val);

  template<typename eT, typename TA>
  arma_hot inline static void apply_noalias(Mat<eT>& out, const TA& A, const eT val);

  template<typename eT, typename TA>
  arma_hot inline static void apply(Mat<eT>& out, const TA& A, const eT val);

  template<typename T1>
  arma_hot inline static void apply_proxy(Mat<typename T1::elem_type>& out, const T1& X, const typename T1::elem_type val);

  // NOTE: there is no direct handling of Op<T1,op_strans2>, as op_strans2::apply_proxy() is currently only called by op_htrans2 for non-complex numbers
  };



class op_strans_cube
  {
  public:

  template<typename eT>
  inline static void apply_noalias(Cube<eT>& out, const Cube<eT>& X);
  };



//! @}
