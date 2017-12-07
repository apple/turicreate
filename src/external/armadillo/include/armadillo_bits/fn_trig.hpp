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


//! \addtogroup fn_trig
//! @{

//
// single argument trigonometric functions:
// cos family: cos, acos, cosh, acosh
// sin family: sin, asin, sinh, asinh
// tan family: tan, atan, tanh, atanh
//
// dual argument trigonometric functions:
// atan2
// hypot


//
// cos

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_cos> >::result
cos(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_cos>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_cos>
cos(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_cos>(A.get_ref());
  }



//
// acos

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_acos> >::result
acos(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_acos>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_acos>
acos(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_acos>(A.get_ref());
  }



//
// cosh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_cosh> >::result
cosh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_cosh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_cosh>
cosh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_cosh>(A.get_ref());
  }



//
// acosh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_acosh> >::result
acosh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_acosh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_acosh>
acosh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_acosh>(A.get_ref());
  }



//
// sin

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_sin> >::result
sin(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_sin>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_sin>
sin(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_sin>(A.get_ref());
  }



//
// asin

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_asin> >::result
asin(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_asin>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_asin>
asin(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_asin>(A.get_ref());
  }



//
// sinh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_sinh> >::result
sinh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_sinh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_sinh>
sinh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_sinh>(A.get_ref());
  }



//
// asinh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_asinh> >::result
asinh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_asinh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_asinh>
asinh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_asinh>(A.get_ref());
  }



//
// tan

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_tan> >::result
tan(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_tan>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_tan>
tan(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_tan>(A.get_ref());
  }



//
// atan

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_atan> >::result
atan(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_atan>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_atan>
atan(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_atan>(A.get_ref());
  }



//
// tanh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_tanh> >::result
tanh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_tanh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_tanh>
tanh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_tanh>(A.get_ref());
  }



//
// atanh

template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_atanh> >::result
atanh(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_atanh>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_atanh>
atanh(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_atanh>(A.get_ref());
  }



//
// atan2

template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_real<typename T1::elem_type>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_atan2>
  >::result
atan2(const T1& Y, const T2& X)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_atan2>(Y, X);
  }



template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename enable_if2< is_real<typename T1::elem_type>::value, const GlueCube<T1, T2, glue_atan2> >::result
atan2(const BaseCube<typename T1::elem_type,T1>& Y, const BaseCube<typename T1::elem_type,T2>& X)
  {
  arma_extra_debug_sigprint();

  return GlueCube<T1, T2, glue_atan2>(Y.get_ref(), X.get_ref());
  }



//
// hypot

template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_real<typename T1::elem_type>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_hypot>
  >::result
hypot(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_hypot>(X, Y);
  }



template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename enable_if2< is_real<typename T1::elem_type>::value, const GlueCube<T1, T2, glue_hypot> >::result
hypot(const BaseCube<typename T1::elem_type,T1>& X, const BaseCube<typename T1::elem_type,T2>& Y)
  {
  arma_extra_debug_sigprint();

  return GlueCube<T1, T2, glue_hypot>(X.get_ref(), Y.get_ref());
  }



//! @}
