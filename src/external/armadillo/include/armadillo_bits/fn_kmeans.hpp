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


//! \addtogroup fn_kmeans
//! @{



template<typename T1>
inline
typename enable_if2<is_real<typename T1::elem_type>::value, bool>::result
kmeans
  (
         Mat<typename T1::elem_type>&    means,
  const Base<typename T1::elem_type,T1>& data,
  const uword                            k,
  const gmm_seed_mode&                   seed_mode,
  const uword                            n_iter,
  const bool                             print_mode
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  gmm_priv::gmm_diag<eT> model;

  const bool status = model.kmeans_wrapper(means, data.get_ref(), k, seed_mode, n_iter, print_mode);

  if(status == true)
    {
    means = model.means;
    }
  else
    {
    means.soft_reset();
    }

  return status;
  }



//! @}
