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



//! \addtogroup op_clamp
//! @{



class op_clamp
  {
  public:

  // matrices

  template<typename T1> inline static void apply(Mat<typename T1::elem_type>& out, const mtOp<typename T1::elem_type, T1, op_clamp>& in);

  template<typename T1> inline static void apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const typename T1::elem_type min_val, const typename T1::elem_type max_val);


  // cubes

  template<typename T1> inline static void apply(Cube<typename T1::elem_type>& out, const mtOpCube<typename T1::elem_type, T1, op_clamp>& in);

  template<typename T1> inline static void apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const typename T1::elem_type min_val, const typename T1::elem_type max_val);
  };



//! @}
