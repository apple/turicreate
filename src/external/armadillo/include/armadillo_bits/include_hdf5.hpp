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


#if defined(ARMA_USE_HDF5)
  #if !defined(ARMA_HDF5_INCLUDE_DIR)
    #include <hdf5.h>
  #else
    #define ARMA_STR1(x) x
    #define ARMA_STR2(x) ARMA_STR1(x)

    #define ARMA_HDF5_HEADER ARMA_STR2(ARMA_HDF5_INCLUDE_DIR)ARMA_STR2(hdf5.h)

    #include ARMA_INCFILE_WRAP(ARMA_HDF5_HEADER)

    #undef ARMA_STR1
    #undef ARMA_STR2
    #undef ARMA_HDF5_HEADER
  #endif

  #if defined(H5_USE_16_API_DEFAULT) || defined(H5_USE_16_API)
    #pragma message ("WARNING: disabling use of HDF5 due to its incompatible configuration")
    #undef ARMA_USE_HDF5
    #undef ARMA_USE_HDF5_ALT
  #endif
#endif
