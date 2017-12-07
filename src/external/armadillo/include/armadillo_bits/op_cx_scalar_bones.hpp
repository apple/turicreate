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


//! \addtogroup op_cx_scalar
//! @{



class op_cx_scalar_times
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_times>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_times>& X
    );

  };



class op_cx_scalar_plus
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>& X
    );

  };



class op_cx_scalar_minus_pre
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>& X
    );

  };



class op_cx_scalar_minus_post
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>& X
    );

  };



class op_cx_scalar_div_pre
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_pre>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_pre>& X
    );

  };



class op_cx_scalar_div_post
  {
  public:

  template<typename T1>
  inline static void
  apply
    (
          Mat< typename std::complex<typename T1::pod_type> >& out,
    const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_post>& X
    );

  template<typename T1>
  inline static void
  apply
    (
             Cube< typename std::complex<typename T1::pod_type> >& out,
    const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_post>& X
    );

  };



//! @}
