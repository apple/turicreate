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


//! \addtogroup op_relational
//! @{


#undef operator_rel

#undef arma_applier_mat_pre
#undef arma_applier_mat_post

#undef arma_applier_cube_pre
#undef arma_applier_cube_post


#define arma_applier_mat_pre(operator_rel) \
  {\
  typedef typename T1::elem_type      eT;\
  typedef typename Proxy<T1>::ea_type ea_type;\
  \
  const eT val = X.aux;\
  \
  const Proxy<T1> P(X.m);\
  \
  const uword n_rows = P.get_n_rows();\
  const uword n_cols = P.get_n_cols();\
  \
  const bool bad_alias = ( Proxy<T1>::has_subview && P.is_alias(out) );\
  \
  if(bad_alias == false)\
    {\
    out.set_size(n_rows, n_cols);\
    \
    uword* out_mem = out.memptr();\
    \
    if(Proxy<T1>::use_at == false)\
      {\
            ea_type PA     = P.get_ea();\
      const uword   n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (val operator_rel PA[i]) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      if(n_rows == 1)\
        {\
        for(uword count=0; count < n_cols; ++count)\
          {\
          out_mem[count] = (val operator_rel P.at(0,count)) ? uword(1) : uword(0);\
          }\
        }\
      else\
        {\
        for(uword col=0; col < n_cols; ++col)\
        for(uword row=0; row < n_rows; ++row)\
          {\
          *out_mem = (val operator_rel P.at(row,col)) ? uword(1) : uword(0);\
          out_mem++;\
          }\
        }\
      }\
    }\
  else\
    {\
    const Mat<eT> tmp(P.Q);\
    \
    out = (val) operator_rel (tmp);\
    }\
  }



#define arma_applier_mat_post(operator_rel) \
  {\
  typedef typename T1::elem_type      eT;\
  typedef typename Proxy<T1>::ea_type ea_type;\
  \
  const eT val = X.aux;\
  \
  const Proxy<T1> P(X.m);\
  \
  const uword n_rows = P.get_n_rows();\
  const uword n_cols = P.get_n_cols();\
  \
  const bool bad_alias = ( Proxy<T1>::has_subview && P.is_alias(out) );\
  \
  if(bad_alias == false)\
    {\
    out.set_size(n_rows, n_cols);\
    \
    uword* out_mem = out.memptr();\
    \
    if(Proxy<T1>::use_at == false)\
      {\
            ea_type PA     = P.get_ea();\
      const uword   n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (PA[i] operator_rel val) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      if(n_rows == 1)\
        {\
        for(uword count=0; count < n_cols; ++count)\
          {\
          out_mem[count] = (P.at(0,count) operator_rel val) ? uword(1) : uword(0);\
          }\
        }\
      else\
        {\
        for(uword col=0; col < n_cols; ++col)\
        for(uword row=0; row < n_rows; ++row)\
          {\
          *out_mem = (P.at(row,col) operator_rel val) ? uword(1) : uword(0);\
          out_mem++;\
          }\
        }\
      }\
    }\
  else\
    {\
    const Mat<eT> tmp(P.Q);\
    \
    out = (tmp) operator_rel (val);\
    }\
  }



#define arma_applier_cube_pre(operator_rel) \
  {\
  typedef typename T1::elem_type          eT;\
  typedef typename ProxyCube<T1>::ea_type ea_type;\
  \
  const eT val = X.aux;\
  \
  const ProxyCube<T1> P(X.m);\
  \
  const uword n_rows   = P.get_n_rows();\
  const uword n_cols   = P.get_n_cols();\
  const uword n_slices = P.get_n_slices();\
  \
  const bool bad_alias = ( ProxyCube<T1>::has_subview && P.is_alias(out) );\
  \
  if(bad_alias == false)\
    {\
    out.set_size(n_rows, n_cols, n_slices);\
    \
    uword* out_mem = out.memptr();\
    \
    if(ProxyCube<T1>::use_at == false)\
      {\
            ea_type PA     = P.get_ea();\
      const uword   n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (val operator_rel PA[i]) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      for(uword slice=0; slice < n_slices; ++slice)\
      for(uword col=0;   col   < n_cols;   ++col  )\
      for(uword row=0;   row   < n_rows;   ++row  )\
        {\
        *out_mem = (val operator_rel P.at(row,col,slice)) ? uword(1) : uword(0);\
        out_mem++;\
        }\
      }\
    }\
  else\
    {\
    const unwrap_cube<typename ProxyCube<T1>::stored_type> tmp(P.Q);\
    \
    out = (val) operator_rel (tmp.M);\
    }\
  }



#define arma_applier_cube_post(operator_rel) \
  {\
  typedef typename T1::elem_type          eT;\
  typedef typename ProxyCube<T1>::ea_type ea_type;\
  \
  const eT val = X.aux;\
  \
  const ProxyCube<T1> P(X.m);\
  \
  const uword n_rows   = P.get_n_rows();\
  const uword n_cols   = P.get_n_cols();\
  const uword n_slices = P.get_n_slices();\
  \
  const bool bad_alias = ( ProxyCube<T1>::has_subview && P.is_alias(out) );\
  \
  if(bad_alias == false)\
    {\
    out.set_size(n_rows, n_cols, n_slices);\
    \
    uword* out_mem = out.memptr();\
    \
    if(ProxyCube<T1>::use_at == false)\
      {\
            ea_type PA     = P.get_ea();\
      const uword   n_elem = out.n_elem;\
      \
      for(uword i=0; i<n_elem; ++i)\
        {\
        out_mem[i] = (PA[i] operator_rel val) ? uword(1) : uword(0);\
        }\
      }\
    else\
      {\
      for(uword slice=0; slice < n_slices; ++slice)\
      for(uword col=0;   col   < n_cols;   ++col  )\
      for(uword row=0;   row   < n_rows;   ++row  )\
        {\
        *out_mem = (P.at(row,col,slice) operator_rel val) ? uword(1) : uword(0);\
        out_mem++;\
        }\
      }\
    }\
  else\
    {\
    const unwrap_cube<typename ProxyCube<T1>::stored_type> tmp(P.Q);\
    \
    out = (tmp.M) operator_rel (val);\
    }\
  }



template<typename T1>
inline
void
op_rel_lt_pre::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lt_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_pre( < );
  }



template<typename T1>
inline
void
op_rel_gt_pre::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gt_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_pre( > );
  }



template<typename T1>
inline
void
op_rel_lteq_pre::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lteq_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_pre( <= );
  }



template<typename T1>
inline
void
op_rel_gteq_pre::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gteq_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_pre( >= );
  }



template<typename T1>
inline
void
op_rel_lt_post::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lt_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( < );
  }



template<typename T1>
inline
void
op_rel_gt_post::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gt_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( > );
  }



template<typename T1>
inline
void
op_rel_lteq_post::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lteq_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( <= );
  }



template<typename T1>
inline
void
op_rel_gteq_post::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gteq_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( >= );
  }



template<typename T1>
inline
void
op_rel_eq::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_eq>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( == );
  }



template<typename T1>
inline
void
op_rel_noteq::apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_noteq>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_mat_post( != );
  }



//
//
//



template<typename T1>
inline
void
op_rel_lt_pre::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lt_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_pre( < );
  }



template<typename T1>
inline
void
op_rel_gt_pre::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gt_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_pre( > );
  }



template<typename T1>
inline
void
op_rel_lteq_pre::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lteq_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_pre( <= );
  }



template<typename T1>
inline
void
op_rel_gteq_pre::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gteq_pre>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_pre( >= );
  }



template<typename T1>
inline
void
op_rel_lt_post::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lt_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( < );
  }



template<typename T1>
inline
void
op_rel_gt_post::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gt_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( > );
  }



template<typename T1>
inline
void
op_rel_lteq_post::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lteq_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( <= );
  }



template<typename T1>
inline
void
op_rel_gteq_post::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gteq_post>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( >= );
  }



template<typename T1>
inline
void
op_rel_eq::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_eq>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( == );
  }



template<typename T1>
inline
void
op_rel_noteq::apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_noteq>& X)
  {
  arma_extra_debug_sigprint();

  arma_applier_cube_post( != );
  }



#undef arma_applier_mat_pre
#undef arma_applier_mat_post

#undef arma_applier_cube_pre
#undef arma_applier_cube_post



//! @}
