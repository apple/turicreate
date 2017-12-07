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


//! \addtogroup cond_rel
//! @{



template<>
template<typename eT>
arma_inline
bool
cond_rel<true>::lt(const eT A, const eT B)
  {
  return (A < B);
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<false>::lt(const eT, const eT)
  {
  return false;
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<true>::gt(const eT A, const eT B)
  {
  return (A > B);
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<false>::gt(const eT, const eT)
  {
  return false;
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<true>::leq(const eT A, const eT B)
  {
  return (A <= B);
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<false>::leq(const eT, const eT)
  {
  return false;
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<true>::geq(const eT A, const eT B)
  {
  return (A >= B);
  }



template<>
template<typename eT>
arma_inline
bool
cond_rel<false>::geq(const eT, const eT)
  {
  return false;
  }



template<>
template<typename eT>
arma_inline
eT
cond_rel<true>::make_neg(const eT val)
  {
  return -val;
  }



template<>
template<typename eT>
arma_inline
eT
cond_rel<false>::make_neg(const eT)
  {
  return eT(0);
  }



//! @}
