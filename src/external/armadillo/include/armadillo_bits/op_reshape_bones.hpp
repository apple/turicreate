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



//! \addtogroup op_reshape
//! @{



class op_reshape
  {
  public:

  template<typename eT> inline static void apply_unwrap(Mat<eT>&                     out, const Mat<eT>&   A, const uword in_n_rows, const uword in_n_cols, const uword in_dim);

  template<typename T1> inline static void apply_proxy (Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword in_n_rows, const uword in_n_cols);

  template<typename T1> inline static void apply       (Mat<typename T1::elem_type>& out, const Op<T1,op_reshape>& in);
  };



class op_reshape_ext
  {
  public:

  template<typename T1> inline static void apply( Mat<typename T1::elem_type>& out, const     Op<T1,op_reshape_ext>& in);
  template<typename T1> inline static void apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_reshape_ext>& in);
  };



//! @}
