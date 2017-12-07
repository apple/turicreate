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


//! \addtogroup fn_trunc_log
//! @{



template<typename eT>
arma_warn_unused
inline
static
typename arma_real_only<eT>::result
trunc_log(const eT x)
  {
  if(std::numeric_limits<eT>::is_iec559)
    {
    if(x == std::numeric_limits<eT>::infinity())
      {
      return Datum<eT>::log_max;
      }
    else
      {
      return (x <= eT(0)) ? Datum<eT>::log_min : std::log(x);
      }
    }
  else
    {
    return std::log(x);
    }
  }



template<typename eT>
arma_warn_unused
inline
static
typename arma_integral_only<eT>::result
trunc_log(const eT x)
  {
  return eT( trunc_log( double(x) ) );
  }



template<typename T>
arma_warn_unused
inline
static
std::complex<T>
trunc_log(const std::complex<T>& x)
  {
  return std::complex<T>( trunc_log( std::abs(x) ), std::arg(x) );
  }



template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_trunc_log> >::result
trunc_log(const T1& A)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_trunc_log>(A);
  }



template<typename T1>
arma_warn_unused
arma_inline
const eOpCube<T1, eop_trunc_log>
trunc_log(const BaseCube<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return eOpCube<T1, eop_trunc_log>(A.get_ref());
  }



//! @}
