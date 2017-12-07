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


//! \addtogroup fn_sprandu
//! @{



//! Generate a sparse matrix with a randomly selected subset of the elements
//! set to random values in the [0,1] interval (uniform distribution)
template<typename obj_type>
arma_warn_unused
inline
obj_type
sprandu
  (
  const uword  n_rows,
  const uword  n_cols,
  const double density,
  const typename arma_SpMat_SpCol_SpRow_only<obj_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  if(is_SpCol<obj_type>::value == true)
    {
    arma_debug_check( (n_cols != 1), "sprandu(): incompatible size" );
    }
  else
  if(is_SpRow<obj_type>::value == true)
    {
    arma_debug_check( (n_rows != 1), "sprandu(): incompatible size" );
    }

  obj_type out;

  out.sprandu(n_rows, n_cols, density);

  return out;
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
sprandu(const SizeMat& s, const double density, const typename arma_SpMat_SpCol_SpRow_only<obj_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return sprandu<obj_type>(s.n_rows, s.n_cols, density);
  }



arma_warn_unused
inline
sp_mat
sprandu(const uword n_rows, const uword n_cols, const double density)
  {
  arma_extra_debug_sigprint();

  sp_mat out;

  out.sprandu(n_rows, n_cols, density);

  return out;
  }



arma_warn_unused
inline
sp_mat
sprandu(const SizeMat& s, const double density)
  {
  arma_extra_debug_sigprint();

  sp_mat out;

  out.sprandu(s.n_rows, s.n_cols, density);

  return out;
  }



//! Generate a sparse matrix with the non-zero values in the same locations as in the given sparse matrix X,
//! with the non-zero values set to random values in the [0,1] interval (uniform distribution)
template<typename T1>
arma_warn_unused
inline
SpMat<typename T1::elem_type>
sprandu(const SpBase<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  SpMat<eT> out( X.get_ref() );

  arma_rng::randu<eT>::fill( access::rwp(out.values), out.n_nonzero );

  return out;
  }



//! @}
