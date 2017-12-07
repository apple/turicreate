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


//! \addtogroup fn_inplace_trans
//! @{



template<typename eT>
inline
typename
enable_if2
  <
  is_cx<eT>::no,
  void
  >::result
inplace_htrans
  (
        Mat<eT>& X,
  const char*    method = "std"
  )
  {
  arma_extra_debug_sigprint();

  inplace_strans(X, method);
  }



template<typename eT>
inline
typename
enable_if2
  <
  is_cx<eT>::yes,
  void
  >::result
inplace_htrans
  (
        Mat<eT>& X,
  const char*    method = "std"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'l')), "inplace_htrans(): unknown method specified" );

  const bool low_memory = (sig == 'l');

  if( (low_memory == false) || (X.n_rows == X.n_cols) )
    {
    op_htrans::apply_mat_inplace(X);
    }
  else
    {
    inplace_strans(X, method);

    X = conj(X);
    }
  }



template<typename eT>
inline
typename
enable_if2
  <
  is_cx<eT>::no,
  void
  >::result
inplace_trans
  (
        Mat<eT>& X,
  const char*    method = "std"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'l')), "inplace_trans(): unknown method specified" );

  inplace_strans(X, method);
  }



template<typename eT>
inline
typename
enable_if2
  <
  is_cx<eT>::yes,
  void
  >::result
inplace_trans
  (
        Mat<eT>& X,
  const char*    method = "std"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'l')), "inplace_trans(): unknown method specified" );

  inplace_htrans(X, method);
  }



//! @}
