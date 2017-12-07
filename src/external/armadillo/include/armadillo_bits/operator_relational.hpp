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


//! \addtogroup operator_relational
//! @{


// <  : lt
// >  : gt
// <= : lteq
// >= : gteq
// == : eq
// != : noteq
// && : and
// || : or


template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_lt>
  >::result
operator<
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_lt>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_gt>
  >::result
operator>
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_gt>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_lteq>
  >::result
operator<=
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_lteq>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_gteq>
  >::result
operator>=
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_gteq>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value),
  const mtGlue<uword, T1, T2, glue_rel_eq>
  >::result
operator==
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_eq>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value),
  const mtGlue<uword, T1, T2, glue_rel_noteq>
  >::result
operator!=
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_noteq>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_and>
  >::result
operator&&
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_and>( X, Y );
  }



template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_cx<typename T1::elem_type>::no) && (is_cx<typename T2::elem_type>::no)),
  const mtGlue<uword, T1, T2, glue_rel_or>
  >::result
operator||
(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return mtGlue<uword, T1, T2, glue_rel_or>( X, Y );
  }



//
//
//



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_lt_pre>
  >::result
operator<
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_lt_pre>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_lt_post>
  >::result
operator<
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_lt_post>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_gt_pre>
  >::result
operator>
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_gt_pre>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_gt_post>
  >::result
operator>
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_gt_post>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_lteq_pre>
  >::result
operator<=
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_lteq_pre>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_lteq_post>
  >::result
operator<=
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_lteq_post>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_gteq_pre>
  >::result
operator>=
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_gteq_pre>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (is_cx<typename T1::elem_type>::no)),
  const mtOp<uword, T1, op_rel_gteq_post>
  >::result
operator>=
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_gteq_post>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_rel_eq>
  >::result
operator==
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_eq>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_rel_eq>
  >::result
operator==
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_eq>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_rel_noteq>
  >::result
operator!=
(const typename T1::elem_type val, const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_noteq>(X, val);
  }



template<typename T1>
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_rel_noteq>
  >::result
operator!=
(const T1& X, const typename T1::elem_type val)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_rel_noteq>(X, val);
  }



//! @}
