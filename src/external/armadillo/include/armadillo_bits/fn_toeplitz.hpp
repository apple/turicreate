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


//! \addtogroup fn_toeplitz
//! @{



template<typename T1>
arma_warn_unused
inline
const Op<T1, op_toeplitz>
toeplitz(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_toeplitz>( X.get_ref() );
  }



template<typename T1>
arma_warn_unused
inline
const Op<T1, op_toeplitz_c>
circ_toeplitz(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_toeplitz_c>( X.get_ref() );
  }



template<typename T1, typename T2>
arma_warn_unused
inline
const Glue<T1, T2, glue_toeplitz>
toeplitz(const Base<typename T1::elem_type,T1>& X, const Base<typename T1::elem_type,T2>& Y)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_toeplitz>( X.get_ref(), Y.get_ref() );
  }



//! @}
