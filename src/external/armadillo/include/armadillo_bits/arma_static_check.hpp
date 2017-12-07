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


//! \addtogroup arma_static_check
//! @{



template<bool ERROR___INCORRECT_OR_UNSUPPORTED_TYPE>
struct arma_type_check_cxx1998
  {
  arma_inline
  static
  void
  apply()
    {
    static const char
    junk[ ERROR___INCORRECT_OR_UNSUPPORTED_TYPE ? -1 : +1 ];
    }
  };



template<>
struct arma_type_check_cxx1998<false>
  {
  arma_inline
  static
  void
  apply()
    {
    }
  };



#if defined(ARMA_USE_CXX11)

  #define arma_static_check(condition, message)  static_assert( !(condition), #message )

  #define arma_type_check(condition)  static_assert( !(condition), "error: incorrect or unsupported type" )

#else

  #define arma_static_check(condition, message)  static const char message[ (condition) ? -1 : +1 ]

  #define arma_type_check(condition)  arma_type_check_cxx1998<condition>::apply()

#endif



//! @}
