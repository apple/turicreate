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


//! \addtogroup typedef_elem
//! @{


namespace junk
  {
  struct arma_elem_size_test
    {

    // arma_static_check( (sizeof(size_t) < sizeof(uword)),  ERROR___TYPE_SIZE_T_IS_SMALLER_THAN_UWORD );

    arma_static_check( (sizeof(u8) != 1), ERROR___TYPE_U8_HAS_UNSUPPORTED_SIZE );
    arma_static_check( (sizeof(s8) != 1), ERROR___TYPE_S8_HAS_UNSUPPORTED_SIZE );

    arma_static_check( (sizeof(u16) != 2), ERROR___TYPE_U16_HAS_UNSUPPORTED_SIZE );
    arma_static_check( (sizeof(s16) != 2), ERROR___TYPE_S16_HAS_UNSUPPORTED_SIZE );

    arma_static_check( (sizeof(u32) != 4), ERROR___TYPE_U32_HAS_UNSUPPORTED_SIZE );
    arma_static_check( (sizeof(s32) != 4), ERROR___TYPE_S32_HAS_UNSUPPORTED_SIZE );

    #if defined(ARMA_USE_U64S64)
      arma_static_check( (sizeof(u64) != 8), ERROR___TYPE_U64_HAS_UNSUPPORTED_SIZE );
      arma_static_check( (sizeof(s64) != 8), ERROR___TYPE_S64_HAS_UNSUPPORTED_SIZE );
    #endif

    arma_static_check( (sizeof(float)  != 4), ERROR___TYPE_FLOAT_HAS_UNSUPPORTED_SIZE );
    arma_static_check( (sizeof(double) != 8), ERROR___TYPE_DOUBLE_HAS_UNSUPPORTED_SIZE );

    arma_static_check( (sizeof(std::complex<float>)  != 8),  ERROR___TYPE_COMPLEX_FLOAT_HAS_UNSUPPORTED_SIZE );
    arma_static_check( (sizeof(std::complex<double>) != 16), ERROR___TYPE_COMPLEX_DOUBLE_HAS_UNSUPPORTED_SIZE );

    };
  }


//! @}
