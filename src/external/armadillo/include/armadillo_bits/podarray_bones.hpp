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


//! \addtogroup podarray
//! @{



struct podarray_prealloc_n_elem
  {
  static const uword val = 16;
  };



//! A lightweight array for POD types. For internal use only!
template<typename eT>
class podarray
  {
  public:

  arma_aligned const uword n_elem; //!< number of elements held
  arma_aligned       eT*   mem;    //!< pointer to memory used by the object


  protected:
  //! internal memory, to avoid calling the 'new' operator for small amounts of memory.
  arma_align_mem eT mem_local[ podarray_prealloc_n_elem::val ];


  public:

  inline ~podarray();
  inline  podarray();

  inline                 podarray (const podarray& x);
  inline const podarray& operator=(const podarray& x);

  arma_inline explicit podarray(const uword new_N);

  arma_inline explicit podarray(const eT* X, const uword new_N);

  template<typename T1>
  inline explicit podarray(const Proxy<T1>& P);

  arma_inline eT& operator[] (const uword i);
  arma_inline eT  operator[] (const uword i) const;

  arma_inline eT& operator() (const uword i);
  arma_inline eT  operator() (const uword i) const;

  inline void set_min_size(const uword min_n_elem);

  inline void set_size(const uword new_n_elem);
  inline void reset();


  inline void fill(const eT val);

  inline void zeros();
  inline void zeros(const uword new_n_elem);

  arma_inline       eT* memptr();
  arma_inline const eT* memptr() const;

  arma_hot inline void copy_row(const Mat<eT>& A, const uword row);


  protected:

  inline void init_cold(const uword new_n_elem);
  inline void init_warm(const uword new_n_elem);
  };



//! @}
