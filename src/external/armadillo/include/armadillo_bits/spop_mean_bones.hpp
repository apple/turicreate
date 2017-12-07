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


//! \addtogroup spop_mean
//! @{


//! Class for finding mean values of a sparse matrix
class spop_mean
  {
  public:

  // Apply mean into an output sparse matrix (or vector).
  template<typename T1>
  inline static void apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_mean>& in);

  template<typename T1>
  inline static void apply_noalias_fast(SpMat<typename T1::elem_type>& out, const SpProxy<T1>& p, const uword dim);

  template<typename T1>
  inline static void apply_noalias_slow(SpMat<typename T1::elem_type>& out, const SpProxy<T1>& p, const uword dim);

  // Take direct mean of a set of values.  Length of array and number of values can be different.
  template<typename eT>
  inline static eT direct_mean(const eT* const X, const uword length, const uword N);

  template<typename eT>
  inline static eT direct_mean_robust(const eT* const X, const uword length, const uword N);

  template<typename T1>
  inline static typename T1::elem_type mean_all(const SpBase<typename T1::elem_type, T1>& X);

  // Take the mean using an iterator.
  template<typename T1, typename eT>
  inline static eT iterator_mean(T1& it, const T1& end, const uword n_zero, const eT junk);

  template<typename T1, typename eT>
  inline static eT iterator_mean_robust(T1& it, const T1& end, const uword n_zero, const eT junk);
  };



//! @}
