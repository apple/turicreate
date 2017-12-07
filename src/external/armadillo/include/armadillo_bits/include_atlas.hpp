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


#if defined(ARMA_USE_ATLAS)
  #if !defined(ARMA_ATLAS_INCLUDE_DIR)
    extern "C"
      {
      #include <cblas.h>
      #include <clapack.h>
      }
  #else
    #define ARMA_STR1(x) x
    #define ARMA_STR2(x) ARMA_STR1(x)

    #define ARMA_CBLAS   ARMA_STR2(ARMA_ATLAS_INCLUDE_DIR)ARMA_STR2(cblas.h)
    #define ARMA_CLAPACK ARMA_STR2(ARMA_ATLAS_INCLUDE_DIR)ARMA_STR2(clapack.h)

    extern "C"
      {
      #include ARMA_INCFILE_WRAP(ARMA_CBLAS)
      #include ARMA_INCFILE_WRAP(ARMA_CLAPACK)
      }

    #undef ARMA_STR1
    #undef ARMA_STR2
    #undef ARMA_CBLAS
    #undef ARMA_CLAPACK
  #endif
#endif
