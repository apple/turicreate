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



//! \addtogroup op_any
//! @{



class op_any
  {
  public:


  template<typename T1>
  static inline bool
  any_vec_helper(const Base<typename T1::elem_type, T1>& X);


  template<typename eT>
  static inline bool
  any_vec_helper(const subview<eT>& X);


  template<typename T1>
  static inline bool
  any_vec_helper(const Op<T1, op_vectorise_col>& X);


  template<typename T1, typename op_type>
  static inline bool
  any_vec_helper
    (
    const mtOp<uword, T1, op_type>& X,
    const typename arma_op_rel_only<op_type>::result           junk1 = 0,
    const typename arma_not_cx<typename T1::elem_type>::result junk2 = 0
    );


  template<typename T1, typename T2, typename glue_type>
  static inline bool
  any_vec_helper
    (
    const mtGlue<uword, T1, T2, glue_type>& X,
    const typename arma_glue_rel_only<glue_type>::result       junk1 = 0,
    const typename arma_not_cx<typename T1::elem_type>::result junk2 = 0,
    const typename arma_not_cx<typename T2::elem_type>::result junk3 = 0
    );


  template<typename T1>
  static inline bool any_vec(T1& X);


  template<typename T1>
  static inline void apply_helper(Mat<uword>& out, const Proxy<T1>& P, const uword dim);


  template<typename T1>
  static inline void apply(Mat<uword>& out, const mtOp<uword, T1, op_any>& X);
  };



//! @}
