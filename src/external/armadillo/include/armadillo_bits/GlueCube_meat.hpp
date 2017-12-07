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


//! \addtogroup GlueCube
//! @{



template<typename T1, typename T2, typename glue_type>
inline
GlueCube<T1,T2,glue_type>::GlueCube(const BaseCube<typename T1::elem_type, T1>& in_A, const BaseCube<typename T1::elem_type, T2>& in_B)
  : A(in_A.get_ref())
  , B(in_B.get_ref())
  {
  arma_extra_debug_sigprint();
  }



template<typename T1, typename T2, typename glue_type>
inline
GlueCube<T1,T2,glue_type>::~GlueCube()
  {
  arma_extra_debug_sigprint();
  }



//! @}
