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


//! \addtogroup mp_misc
//! @{




template<typename eT, const bool use_smaller_thresh = false>
struct mp_gate
  {
  arma_inline
  static
  bool
  eval(const uword n_elem)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      const bool length_ok = (is_cx<eT>::yes || use_smaller_thresh) ? (n_elem >= (arma_config::mp_threshold/uword(2))) : (n_elem >= arma_config::mp_threshold);

      if(length_ok)
        {
        if(omp_in_parallel())  { return false; }
        }

      return length_ok;
      }
    #else
      {
      arma_ignore(n_elem);

      return false;
      }
    #endif
    }
  };



struct mp_thread_limit
  {
  arma_inline
  static
  int
  get()
    {
    #if defined(ARMA_USE_OPENMP)
      int n_threads = (std::min)(int(arma_config::mp_threads), int((std::max)(int(1), int(omp_get_max_threads()))));
    #else
      int n_threads = int(1);
    #endif

    return n_threads;
    }
  };



//! @}
