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



#if (__cplusplus >= 201103L)
  #undef  ARMA_USE_CXX11
  #define ARMA_USE_CXX11
#endif


// MS really can't get its proverbial shit together
#if (defined(_MSVC_LANG) && (_MSVC_LANG >= 201402L))
  #undef  ARMA_USE_CXX11
  #define ARMA_USE_CXX11
  #undef  ARMA_DONT_PRINT_CXX11_WARNING
  #define ARMA_DONT_PRINT_CXX11_WARNING
#endif


#if (defined(_OPENMP) && (_OPENMP >= 200805))
  #undef  ARMA_USE_OPENMP
  #define ARMA_USE_OPENMP
#endif
