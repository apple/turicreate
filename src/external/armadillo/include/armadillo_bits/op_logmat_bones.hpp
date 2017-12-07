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


//! \addtogroup op_logmat
//! @{



class op_logmat
  {
  public:

  template<typename T1>
  inline static void apply(Mat< std::complex<typename T1::elem_type> >& out, const mtOp<std::complex<typename T1::elem_type>,T1,op_logmat>& in);

  template<typename T1>
  inline static bool apply_direct(Mat< std::complex<typename T1::elem_type> >& out, const Op<T1,op_diagmat>& expr, const uword);

  template<typename T1>
  inline static bool apply_direct(Mat< std::complex<typename T1::elem_type> >& out, const Base<typename T1::elem_type,T1>& expr, const uword n_iters);
  };



class op_logmat_cx
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_logmat_cx>& in);

  template<typename T1>
  inline static bool apply_direct(Mat<typename T1::elem_type>& out, const Op<T1,op_diagmat>& expr, const uword);

  template<typename T1>
  inline static bool apply_direct_noalias(Mat<typename T1::elem_type>& out, const diagmat_proxy<T1>& P);

  template<typename T1>
  inline static bool apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& expr, const uword n_iters);

  template<typename T>
  inline static bool apply_common(Mat< std::complex<T> >& out, Mat< std::complex<T> >& S, const uword n_iters);


  template<typename eT>
  inline static bool helper(Mat<eT>& S, const uword m);
  };



class op_logmat_sympd
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_logmat_sympd>& in);

  template<typename T1>
  inline static bool apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& expr);
  };



//! @}
