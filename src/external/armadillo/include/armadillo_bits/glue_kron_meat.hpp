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


//! \addtogroup glue_kron
//! @{



//! \brief
//! both input matrices have the same element type
template<typename eT>
inline
void
glue_kron::direct_kron(Mat<eT>& out, const Mat<eT>& A, const Mat<eT>& B)
  {
  arma_extra_debug_sigprint();

  const uword A_rows = A.n_rows;
  const uword A_cols = A.n_cols;
  const uword B_rows = B.n_rows;
  const uword B_cols = B.n_cols;

  out.set_size(A_rows*B_rows, A_cols*B_cols);

  if(out.is_empty())  { return; }

  for(uword j = 0; j < A_cols; j++)
    {
    for(uword i = 0; i < A_rows; i++)
      {
      out.submat(i*B_rows, j*B_cols, (i+1)*B_rows-1, (j+1)*B_cols-1) = A.at(i,j) * B;
      }
    }
  }



//! \brief
//! different types of input matrices
//! A -> complex, B -> basic element type
template<typename T>
inline
void
glue_kron::direct_kron(Mat< std::complex<T> >& out, const Mat< std::complex<T> >& A, const Mat<T>& B)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  const uword A_rows = A.n_rows;
  const uword A_cols = A.n_cols;
  const uword B_rows = B.n_rows;
  const uword B_cols = B.n_cols;

  out.set_size(A_rows*B_rows, A_cols*B_cols);

  if(out.is_empty())  { return; }

  Mat<eT> tmp_B = conv_to< Mat<eT> >::from(B);

  for(uword j = 0; j < A_cols; j++)
    {
    for(uword i = 0; i < A_rows; i++)
      {
      out.submat(i*B_rows, j*B_cols, (i+1)*B_rows-1, (j+1)*B_cols-1) = A.at(i,j) * tmp_B;
      }
    }
  }



//! \brief
//! different types of input matrices
//! A -> basic element type, B -> complex
template<typename T>
inline
void
glue_kron::direct_kron(Mat< std::complex<T> >& out, const Mat<T>& A, const Mat< std::complex<T> >& B)
  {
  arma_extra_debug_sigprint();

  const uword A_rows = A.n_rows;
  const uword A_cols = A.n_cols;
  const uword B_rows = B.n_rows;
  const uword B_cols = B.n_cols;

  out.set_size(A_rows*B_rows, A_cols*B_cols);

  if(out.is_empty())  { return; }

  for(uword j = 0; j < A_cols; j++)
    {
    for(uword i = 0; i < A_rows; i++)
      {
      out.submat(i*B_rows, j*B_cols, (i+1)*B_rows-1, (j+1)*B_cols-1) = A.at(i,j) * B;
      }
    }
  }



//! \brief
//! apply Kronecker product for two objects with same element type
template<typename T1, typename T2>
inline
void
glue_kron::apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_kron>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1> A_tmp(X.A);
  const unwrap<T2> B_tmp(X.B);

  const Mat<eT>& A = A_tmp.M;
  const Mat<eT>& B = B_tmp.M;

  if( (&out != &A) && (&out != &B) )
    {
    glue_kron::direct_kron(out, A, B);
    }
  else
    {
    Mat<eT> tmp;

    glue_kron::direct_kron(tmp, A, B);

    out.steal_mem(tmp);
    }
  }



//! @}
