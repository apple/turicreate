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



//! \addtogroup glue_min
//! @{



class glue_min
  {
  public:

  // dense matrices

  template<typename T1, typename T2> inline static void apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_min>& X);

  template<typename eT, typename T1, typename T2> inline static void apply(Mat< eT              >& out, const Proxy<T1>& PA, const Proxy<T2>& PB);

  template<typename  T, typename T1, typename T2> inline static void apply(Mat< std::complex<T> >& out, const Proxy<T1>& PA, const Proxy<T2>& PB);


  // cubes

  template<typename T1, typename T2> inline static void apply(Cube<typename T1::elem_type>& out, const GlueCube<T1,T2,glue_min>& X);

  template<typename eT, typename T1, typename T2> inline static void apply(Cube< eT              >& out, const ProxyCube<T1>& PA, const ProxyCube<T2>& PB);

  template<typename  T, typename T1, typename T2> inline static void apply(Cube< std::complex<T> >& out, const ProxyCube<T1>& PA, const ProxyCube<T2>& PB);
  };



//! @}
