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


//! \addtogroup spglue_join
//! @{



template<typename T1, typename T2>
inline
void
spglue_join_cols::apply(SpMat<typename T1::elem_type>& out, const SpGlue<T1,T2,spglue_join_cols>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_spmat<T1> A_tmp(X.A);
  const unwrap_spmat<T2> B_tmp(X.B);

  const SpMat<eT>& A = A_tmp.M;
  const SpMat<eT>& B = B_tmp.M;

  if( (&out != &A) && (&out != &B) )
    {
    spglue_join_cols::apply_noalias(out, A, B);
    }
  else
    {
    SpMat<eT> tmp;

    spglue_join_cols::apply_noalias(tmp, A, B);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
spglue_join_cols::apply_noalias(SpMat<eT>& out, const SpMat<eT>& A, const SpMat<eT>& B)
  {
  arma_extra_debug_sigprint();

  const uword A_n_rows = A.n_rows;
  const uword A_n_cols = A.n_cols;

  const uword B_n_rows = B.n_rows;
  const uword B_n_cols = B.n_cols;

  arma_debug_check
    (
    ( (A_n_cols != B_n_cols) && ( (A_n_rows > 0) || (A_n_cols > 0) ) && ( (B_n_rows > 0) || (B_n_cols > 0) ) ),
    "join_cols() / join_vert(): number of columns must be the same"
    );

  out.set_size( A_n_rows + B_n_rows, (std::max)(A_n_cols, B_n_cols) );

  if( out.n_elem > 0 )
    {
    if(A.is_empty() == false)
      {
      out.submat(0,        0,   A_n_rows-1, out.n_cols-1) = A;
      }

    if(B.is_empty() == false)
      {
      out.submat(A_n_rows, 0, out.n_rows-1, out.n_cols-1) = B;
      }
    }
  }



template<typename T1, typename T2>
inline
void
spglue_join_rows::apply(SpMat<typename T1::elem_type>& out, const SpGlue<T1,T2,spglue_join_rows>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_spmat<T1> A_tmp(X.A);
  const unwrap_spmat<T2> B_tmp(X.B);

  const SpMat<eT>& A = A_tmp.M;
  const SpMat<eT>& B = B_tmp.M;

  if( (&out != &A) && (&out != &B) )
    {
    spglue_join_rows::apply_noalias(out, A, B);
    }
  else
    {
    SpMat<eT> tmp;

    spglue_join_rows::apply_noalias(tmp, A, B);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
spglue_join_rows::apply_noalias(SpMat<eT>& out, const SpMat<eT>& A, const SpMat<eT>& B)
  {
  arma_extra_debug_sigprint();

  const uword A_n_rows = A.n_rows;
  const uword A_n_cols = A.n_cols;

  const uword B_n_rows = B.n_rows;
  const uword B_n_cols = B.n_cols;

  arma_debug_check
    (
    ( (A_n_rows != B.n_rows) && ( (A_n_rows > 0) || (A_n_cols > 0) ) && ( (B_n_rows > 0) || (B_n_cols > 0) ) ),
    "join_rows() / join_horiz(): number of rows must be the same"
    );

  out.set_size( (std::max)(A_n_rows, B_n_rows), A_n_cols + B_n_cols );

  if( out.n_elem > 0 )
    {
    if(A.is_empty() == false)
      {
      out.submat(0, 0,        out.n_rows-1,   A.n_cols-1) = A;
      }

    if(B.is_empty() == false)
      {
      out.submat(0, A_n_cols, out.n_rows-1, out.n_cols-1) = B;
      }
    }
  }



//! @}
