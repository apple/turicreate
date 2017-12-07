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



//! \addtogroup op_unique
//! @{



class op_unique
  {
  public:

  template<typename T1>
  inline static bool apply_helper(Mat<typename T1::elem_type>& out, const Proxy<T1>& P);

  template<typename T1>
  inline static void apply(Mat<typename T1::elem_type>& out, const Op<T1,op_unique>& in);
  };



template<typename eT>
struct arma_unique_comparator
  {
  arma_inline
  bool
  operator() (const eT a, const eT b) const
    {
    return ( a < b );
    }
  };



template<typename T>
struct arma_unique_comparator< std::complex<T> >
  {
  arma_inline
  bool
  operator() (const std::complex<T>& a, const std::complex<T>& b) const
    {
    const T a_real = a.real();
    const T b_real = b.real();

    return (  (a_real < b_real) ? true : ((a_real == b_real) ? (a.imag() < b.imag()) : false)  );
    }
  };



//! @}
