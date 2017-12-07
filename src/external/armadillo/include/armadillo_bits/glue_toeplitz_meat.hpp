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



//! \addtogroup glue_toeplitz
//! @{



template<typename T1, typename T2>
inline
void
glue_toeplitz::apply(Mat<typename T1::elem_type>& out, const Glue<T1, T2, glue_toeplitz>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> tmp1(in.A, out);
  const unwrap_check<T2> tmp2(in.B, out);

  const Mat<eT>& A = tmp1.M;
  const Mat<eT>& B = tmp2.M;

  arma_debug_check
    (
    ( ((A.is_vec() == false) && (A.is_empty() == false)) || ((B.is_vec() == false) && (B.is_empty() == false)) ),
    "toeplitz(): given object is not a vector"
    );

  const uword A_N = A.n_elem;
  const uword B_N = B.n_elem;

  const eT* A_mem = A.memptr();
  const eT* B_mem = B.memptr();

  out.set_size(A_N, B_N);

  if( out.is_empty() )  { return; }

  for(uword col=0; col < B_N; ++col)
    {
    eT* col_mem = out.colptr(col);

    uword i = 0;
    for(uword row=col; row < A_N; ++row, ++i)  { col_mem[row] = A_mem[i]; }
    }

  for(uword row=0; row < A_N; ++row)
    {
    uword i = 1;
    for(uword col=(row+1); col < B_N; ++col, ++i)  { out.at(row,col) = B_mem[i]; }
    }
  }



//! @}
