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


//! \addtogroup fn_trapz
//! @{



template<typename T1, typename T2>
arma_warn_unused
inline
const Glue<T1, T2, glue_trapz>
trapz
  (
  const Base<typename T1::elem_type,T1>& X,
  const Base<typename T1::elem_type,T2>& Y,
  const uword                            dim = 0
  )
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_trapz>(X.get_ref(), Y.get_ref(), dim);
  }



template<typename T1>
arma_warn_unused
inline
const Op<T1, op_trapz>
trapz
  (
  const Base<typename T1::elem_type,T1>& Y,
  const uword                            dim = 0
  )
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_trapz>(Y.get_ref(), dim, uword(0));
  }



//! @}
