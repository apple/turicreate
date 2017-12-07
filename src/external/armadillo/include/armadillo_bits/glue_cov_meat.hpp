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


//! \addtogroup glue_cov
//! @{



template<typename eT>
inline
void
glue_cov::direct_cov(Mat<eT>& out, const Mat<eT>& A, const Mat<eT>& B, const uword norm_type)
  {
  arma_extra_debug_sigprint();

  if(A.is_vec() && B.is_vec())
    {
    arma_debug_check( (A.n_elem != B.n_elem), "cov(): the number of elements in A and B must match" );

    const eT* A_ptr = A.memptr();
    const eT* B_ptr = B.memptr();

    eT A_acc   = eT(0);
    eT B_acc   = eT(0);
    eT out_acc = eT(0);

    const uword N = A.n_elem;

    for(uword i=0; i<N; ++i)
      {
      const eT A_tmp = A_ptr[i];
      const eT B_tmp = B_ptr[i];

      A_acc += A_tmp;
      B_acc += B_tmp;

      out_acc += A_tmp * B_tmp;
      }

    out_acc -= (A_acc * B_acc)/eT(N);

    const eT norm_val = (norm_type == 0) ? ( (N > 1) ? eT(N-1) : eT(1) ) : eT(N);

    out.set_size(1,1);
    out[0] = out_acc/norm_val;
    }
  else
    {
    arma_debug_assert_mul_size(A, B, true, false, "cov()");

    const uword N = A.n_rows;
    const eT norm_val = (norm_type == 0) ? ( (N > 1) ? eT(N-1) : eT(1) ) : eT(N);

    out = trans(A) * B;
    out -= (trans(sum(A)) * sum(B))/eT(N);
    out /= norm_val;
    }
  }



template<typename T>
inline
void
glue_cov::direct_cov(Mat< std::complex<T> >& out, const Mat< std::complex<T> >& A, const Mat< std::complex<T> >& B, const uword norm_type)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  if(A.is_vec() && B.is_vec())
    {
    arma_debug_check( (A.n_elem != B.n_elem), "cov(): the number of elements in A and B must match" );

    const eT* A_ptr = A.memptr();
    const eT* B_ptr = B.memptr();

    eT A_acc   = eT(0);
    eT B_acc   = eT(0);
    eT out_acc = eT(0);

    const uword N = A.n_elem;

    for(uword i=0; i<N; ++i)
      {
      const eT A_tmp = A_ptr[i];
      const eT B_tmp = B_ptr[i];

      A_acc += A_tmp;
      B_acc += B_tmp;

      out_acc += std::conj(A_tmp) * B_tmp;
      }

    out_acc -= (std::conj(A_acc) * B_acc)/eT(N);

    const eT norm_val = (norm_type == 0) ? ( (N > 1) ? eT(N-1) : eT(1) ) : eT(N);

    out.set_size(1,1);
    out[0] = out_acc/norm_val;
    }
  else
    {
    arma_debug_assert_mul_size(A, B, true, false, "cov()");

    const uword N = A.n_rows;
    const eT norm_val = (norm_type == 0) ? ( (N > 1) ? eT(N-1) : eT(1) ) : eT(N);

    out = trans(A) * B;                     // out = strans(conj(A)) * B;
    out -= (trans(sum(A)) * sum(B))/eT(N);  // out -= (strans(conj(sum(A))) * sum(B))/eT(N);
    out /= norm_val;
    }
  }



template<typename T1, typename T2>
inline
void
glue_cov::apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_cov>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> A_tmp(X.A, out);
  const unwrap_check<T2> B_tmp(X.B, out);

  const Mat<eT>& A = A_tmp.M;
  const Mat<eT>& B = B_tmp.M;

  const uword norm_type = X.aux_uword;

  if(&A != &B)
    {
    glue_cov::direct_cov(out, A, B, norm_type);
    }
  else
    {
    op_cov::direct_cov(out, A, norm_type);
    }

  }



//! @}
