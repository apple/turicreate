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


//! \addtogroup fn_vectorise
//! @{



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const Op<T1, op_vectorise_col>
  >::result
vectorise(const T1& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_vectorise_col>(X);
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const Op<T1, op_vectorise_all>
  >::result
vectorise(const T1& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (dim > 1), "vectorise(): parameter 'dim' must be 0 or 1" );

  return Op<T1, op_vectorise_all>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
Col<typename T1::elem_type>
vectorise(const BaseCube<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  Col<typename T1::elem_type> out;

  op_vectorise_cube_col::apply(out, X);

  return out;
  }



//! @}
