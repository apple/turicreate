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


//! \addtogroup glue_relational
//! @{



#undef operator_rel
#undef operator_str

#undef arma_applier_mat
#undef arma_applier_cube


#define arma_applier_mat(operator_rel, operator_str) \
  {\
  const Proxy<T1> P1(X.A);\
  const Proxy<T2> P2(X.B);\
  \
  arma_debug_assert_same_size(P1, P2, operator_str);\
  \
  const bool bad_alias = (Proxy<T1>::has_subview && P1.is_alias(out)) || (Proxy<T2>::has_subview && P2.is_alias(out));\
  \
  if(bad_alias == false)\
    {\
    \
    const uword n_rows = P1.get_n_rows();\
    const uword n_cols = P1.get_n_cols();\
    \
    out.set_size(n_rows, n_cols);\
    \
    uword* out_mem = out.memptr();\
    \
    const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);\
    \
    if(use_at == false)\
      {\
      typename Proxy<T1>::ea_type A = P1.get_ea();\
      typename Proxy<T2>::ea_type B = P2.get_ea();\
      \
      const uword n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (A[i] operator_rel B[i]) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      if(n_rows == 1)\
        {\
        for(uword count=0; count < n_cols; ++count)\
          {\
          out_mem[count] = (P1.at(0,count) operator_rel P2.at(0,count)) ? uword(1) : uword(0);\
          }\
        }\
      else\
        {\
        for(uword col=0; col<n_cols; ++col)\
        for(uword row=0; row<n_rows; ++row)\
          {\
          *out_mem = (P1.at(row,col) operator_rel P2.at(row,col)) ? uword(1) : uword(0);\
          out_mem++;\
          }\
        }\
      }\
    }\
  else\
    {\
    const unwrap_check<typename Proxy<T1>::stored_type> tmp1(P1.Q, P1.is_alias(out));\
    const unwrap_check<typename Proxy<T2>::stored_type> tmp2(P2.Q, P2.is_alias(out));\
    \
    out = (tmp1.M) operator_rel (tmp2.M);\
    }\
  }




#define arma_applier_cube(operator_rel, operator_str) \
  {\
  const ProxyCube<T1> P1(X.A);\
  const ProxyCube<T2> P2(X.B);\
  \
  arma_debug_assert_same_size(P1, P2, operator_str);\
  \
  const bool bad_alias = (ProxyCube<T1>::has_subview && P1.is_alias(out)) || (ProxyCube<T2>::has_subview && P2.is_alias(out));\
  \
  if(bad_alias == false)\
    {\
    \
    const uword n_rows   = P1.get_n_rows();\
    const uword n_cols   = P1.get_n_cols();\
    const uword n_slices = P1.get_n_slices();\
    \
    out.set_size(n_rows, n_cols, n_slices);\
    \
    uword* out_mem = out.memptr();\
    \
    const bool use_at = (ProxyCube<T1>::use_at || ProxyCube<T2>::use_at);\
    \
    if(use_at == false)\
      {\
      typename ProxyCube<T1>::ea_type A = P1.get_ea();\
      typename ProxyCube<T2>::ea_type B = P2.get_ea();\
      \
      const uword n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (A[i] operator_rel B[i]) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      for(uword slice = 0; slice < n_slices; ++slice)\
      for(uword col   = 0; col   < n_cols;   ++col  )\
      for(uword row   = 0; row   < n_rows;   ++row  )\
        {\
        *out_mem = (P1.at(row,col,slice) operator_rel P2.at(row,col,slice)) ? uword(1) : uword(0);\
        out_mem++;\
        }\
      }\
    }\
  else\
    {\
    const unwrap_cube<typename ProxyCube<T1>::stored_type> tmp1(P1.Q);\
    const unwrap_cube<typename ProxyCube<T2>::stored_type> tmp2(P2.Q);\
    \
    out = (tmp1.M) operator_rel (tmp2.M);\
    }\
  }



template<typename T1, typename T2>
inline
void
glue_rel_lt::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_lt>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(<, "operator<");
  }



template<typename T1, typename T2>
inline
void
glue_rel_gt::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_gt>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(>, "operator>");
  }



template<typename T1, typename T2>
inline
void
glue_rel_lteq::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_lteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(<=, "operator<=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_gteq::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_gteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(>=, "operator>=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_eq::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_eq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(==, "operator==");
  }



template<typename T1, typename T2>
inline
void
glue_rel_noteq::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_noteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(!=, "operator!=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_and::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_and>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(&&, "operator&&");
  }



template<typename T1, typename T2>
inline
void
glue_rel_or::apply
  (
        Mat   <uword>& out,
  const mtGlue<uword, T1, T2, glue_rel_or>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_mat(||, "operator||");
  }



//
//
//



template<typename T1, typename T2>
inline
void
glue_rel_lt::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_lt>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(<, "operator<");
  }



template<typename T1, typename T2>
inline
void
glue_rel_gt::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_gt>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(>, "operator>");
  }



template<typename T1, typename T2>
inline
void
glue_rel_lteq::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_lteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(<=, "operator<=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_gteq::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_gteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(>=, "operator>=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_eq::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_eq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(==, "operator==");
  }



template<typename T1, typename T2>
inline
void
glue_rel_noteq::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_noteq>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(!=, "operator!=");
  }



template<typename T1, typename T2>
inline
void
glue_rel_and::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_and>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(&&, "operator&&");
  }



template<typename T1, typename T2>
inline
void
glue_rel_or::apply
  (
        Cube      <uword>& out,
  const mtGlueCube<uword, T1, T2, glue_rel_or>& X
  )
  {
  arma_extra_debug_sigprint();

  arma_applier_cube(||, "operator||");
  }



#undef arma_applier_mat
#undef arma_applier_cube



//! @}
