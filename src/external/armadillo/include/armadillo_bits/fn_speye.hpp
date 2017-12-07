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


//! \addtogroup fn_speye
//! @{



//! Generate a sparse matrix with the values along the main diagonal set to one
template<typename obj_type>
arma_warn_unused
inline
obj_type
speye(const uword n_rows, const uword n_cols, const typename arma_SpMat_SpCol_SpRow_only<obj_type>::result* junk = NULL)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  if(is_SpCol<obj_type>::value == true)
    {
    arma_debug_check( (n_cols != 1), "speye(): incompatible size" );
    }
  else
  if(is_SpRow<obj_type>::value == true)
    {
    arma_debug_check( (n_rows != 1), "speye(): incompatible size" );
    }

  obj_type out;

  out.eye(n_rows, n_cols);

  return out;
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
speye(const SizeMat& s, const typename arma_SpMat_SpCol_SpRow_only<obj_type>::result* junk = NULL)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return speye<obj_type>(s.n_rows, s.n_cols);
  }



// Convenience shortcut method (no template parameter necessary)
arma_warn_unused
inline
sp_mat
speye(const uword n_rows, const uword n_cols)
  {
  arma_extra_debug_sigprint();

  sp_mat out;

  out.eye(n_rows, n_cols);

  return out;
  }



arma_warn_unused
inline
sp_mat
speye(const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  sp_mat out;

  out.eye(s.n_rows, s.n_cols);

  return out;
  }



//! @}
