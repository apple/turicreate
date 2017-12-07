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


namespace newarp
{


//! Tiny functions to check attributes of complex numbers
struct cx_attrib
  {
  template<typename T>
  arma_inline static bool is_real   (const std::complex<T>& v, const T eps) { return (std::abs(v.imag()) <= eps); }

  template<typename T>
  arma_inline static bool is_complex(const std::complex<T>& v, const T eps) { return (std::abs(v.imag()) >  eps); }

  template<typename T>
  arma_inline static bool is_conj(const std::complex<T>& v1, const std::complex<T>& v2, const T eps)  { return (std::abs(v1 - std::conj(v2)) <= eps); }
  };


}  // namespace newarp
