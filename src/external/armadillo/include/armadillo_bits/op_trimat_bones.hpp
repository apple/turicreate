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


//! \addtogroup op_trimat
//! @{



class op_trimat
  {
  public:

  template<typename eT>
  inline static void fill_zeros(Mat<eT>& A, const bool upper);

  //

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_trimat>& in);

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<Op<T1,op_htrans>, op_trimat>& in);

  //

  template<typename eT>
  inline static void apply_htrans(Mat<eT>& out, const Mat<eT>& A, const bool upper, const typename arma_not_cx<eT>::result* junk = 0);

  template<typename eT>
  inline static void apply_htrans(Mat<eT>& out, const Mat<eT>& A, const bool upper, const typename arma_cx_only<eT>::result* junk = 0);
  };



class op_trimatu_ext
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_trimatu_ext>& in);

  template<typename eT>
  inline static void fill_zeros(Mat<eT>& A, const uword row_offset, const uword col_offset);
  };



class op_trimatl_ext
  {
  public:

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_trimatl_ext>& in);

  template<typename eT>
  inline static void fill_zeros(Mat<eT>& A, const uword row_offset, const uword col_offset);
  };



//! @}
