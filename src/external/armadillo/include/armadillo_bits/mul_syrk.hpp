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


//! \addtogroup syrk
//! @{



class syrk_helper
  {
  public:

  template<typename eT>
  inline
  static
  void
  inplace_copy_upper_tri_to_lower_tri(Mat<eT>& C)
    {
    // under the assumption that C is a square matrix

    const uword N = C.n_rows;

    for(uword k=0; k < N; ++k)
      {
      eT* colmem = C.colptr(k);

      uword i, j;
      for(i=(k+1), j=(k+2); j < N; i+=2, j+=2)
        {
        const eT tmp_i = C.at(k,i);
        const eT tmp_j = C.at(k,j);

        colmem[i] = tmp_i;
        colmem[j] = tmp_j;
        }

      if(i < N)
        {
        colmem[i] = C.at(k,i);
        }
      }
    }
  };



//! partial emulation of BLAS function syrk(), specialised for A being a vector
template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class syrk_vec
  {
  public:

  template<typename eT, typename TA>
  arma_hot
  inline
  static
  void
  apply
    (
          Mat<eT>& C,
    const TA&      A,
    const eT       alpha = eT(1),
    const eT       beta  = eT(0)
    )
    {
    arma_extra_debug_sigprint();

    const uword A_n1 = (do_trans_A == false) ? A.n_rows : A.n_cols;
    const uword A_n2 = (do_trans_A == false) ? A.n_cols : A.n_rows;

    const eT* A_mem = A.memptr();

    if(A_n1 == 1)
      {
      const eT acc1 = op_dot::direct_dot(A_n2, A_mem, A_mem);

           if( (use_alpha == false) && (use_beta == false) )  { C[0] =       acc1;             }
      else if( (use_alpha == true ) && (use_beta == false) )  { C[0] = alpha*acc1;             }
      else if( (use_alpha == false) && (use_beta == true ) )  { C[0] =       acc1 + beta*C[0]; }
      else if( (use_alpha == true ) && (use_beta == true ) )  { C[0] = alpha*acc1 + beta*C[0]; }
      }
    else
    for(uword k=0; k < A_n1; ++k)
      {
      const eT A_k = A_mem[k];

      uword i,j;
      for(i=(k), j=(k+1); j < A_n1; i+=2, j+=2)
        {
        const eT acc1 = A_k * A_mem[i];
        const eT acc2 = A_k * A_mem[j];

        if( (use_alpha == false) && (use_beta == false) )
          {
          C.at(k, i) = acc1;
          C.at(k, j) = acc2;

          C.at(i, k) = acc1;
          C.at(j, k) = acc2;
          }
        else
        if( (use_alpha == true ) && (use_beta == false) )
          {
          const eT val1 = alpha*acc1;
          const eT val2 = alpha*acc2;

          C.at(k, i) = val1;
          C.at(k, j) = val2;

          C.at(i, k) = val1;
          C.at(j, k) = val2;
          }
        else
        if( (use_alpha == false) && (use_beta == true) )
          {
          C.at(k, i) = acc1 + beta*C.at(k, i);
          C.at(k, j) = acc2 + beta*C.at(k, j);

          if(i != k) { C.at(i, k) = acc1 + beta*C.at(i, k); }
                       C.at(j, k) = acc2 + beta*C.at(j, k);
          }
        else
        if( (use_alpha == true ) && (use_beta == true) )
          {
          const eT val1 = alpha*acc1;
          const eT val2 = alpha*acc2;

          C.at(k, i) = val1 + beta*C.at(k, i);
          C.at(k, j) = val2 + beta*C.at(k, j);

          if(i != k)  { C.at(i, k) = val1 + beta*C.at(i, k); }
                        C.at(j, k) = val2 + beta*C.at(j, k);
          }
        }

      if(i < A_n1)
        {
        const eT acc1 = A_k * A_mem[i];

        if( (use_alpha == false) && (use_beta == false) )
          {
          C.at(k, i) = acc1;
          C.at(i, k) = acc1;
          }
        else
        if( (use_alpha == true) && (use_beta == false) )
          {
          const eT val1 = alpha*acc1;

          C.at(k, i) = val1;
          C.at(i, k) = val1;
          }
        else
        if( (use_alpha == false) && (use_beta == true) )
          {
                        C.at(k, i) = acc1 + beta*C.at(k, i);
          if(i != k)  { C.at(i, k) = acc1 + beta*C.at(i, k); }
          }
        else
        if( (use_alpha == true) && (use_beta == true) )
          {
          const eT val1 = alpha*acc1;

                        C.at(k, i) = val1 + beta*C.at(k, i);
          if(i != k)  { C.at(i, k) = val1 + beta*C.at(i, k); }
          }
        }
      }
    }

  };



//! partial emulation of BLAS function syrk()
template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class syrk_emul
  {
  public:

  template<typename eT, typename TA>
  arma_hot
  inline
  static
  void
  apply
    (
          Mat<eT>& C,
    const TA&      A,
    const eT       alpha = eT(1),
    const eT       beta  = eT(0)
    )
    {
    arma_extra_debug_sigprint();

    // do_trans_A == false  ->   C = alpha * A   * A^T + beta*C
    // do_trans_A == true   ->   C = alpha * A^T * A   + beta*C

    if(do_trans_A == false)
      {
      Mat<eT> AA;

      op_strans::apply_mat_noalias(AA, A);

      syrk_emul<true, use_alpha, use_beta>::apply(C, AA, alpha, beta);
      }
    else
    if(do_trans_A == true)
      {
      const uword A_n_rows = A.n_rows;
      const uword A_n_cols = A.n_cols;

      for(uword col_A=0; col_A < A_n_cols; ++col_A)
        {
        // col_A is interpreted as row_A when storing the results in matrix C

        const eT* A_coldata = A.colptr(col_A);

        for(uword k=col_A; k < A_n_cols; ++k)
          {
          const eT acc = op_dot::direct_dot_arma(A_n_rows, A_coldata, A.colptr(k));

          if( (use_alpha == false) && (use_beta == false) )
            {
            C.at(col_A, k) = acc;
            C.at(k, col_A) = acc;
            }
          else
          if( (use_alpha == true ) && (use_beta == false) )
            {
            const eT val = alpha*acc;

            C.at(col_A, k) = val;
            C.at(k, col_A) = val;
            }
          else
          if( (use_alpha == false) && (use_beta == true ) )
            {
                              C.at(col_A, k) = acc + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = acc + beta*C.at(k, col_A); }
            }
          else
          if( (use_alpha == true ) && (use_beta == true ) )
            {
            const eT val = alpha*acc;

                              C.at(col_A, k) = val + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = val + beta*C.at(k, col_A); }
            }
          }
        }
      }
    }

  };



template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class syrk
  {
  public:

  template<typename eT, typename TA>
  inline
  static
  void
  apply_blas_type( Mat<eT>& C, const TA& A, const eT alpha = eT(1), const eT beta = eT(0) )
    {
    arma_extra_debug_sigprint();

    if(A.is_vec())
      {
      // work around poor handling of vectors by syrk() in ATLAS 3.8.4 and standard BLAS

      syrk_vec<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);

      return;
      }

    const uword threshold = (is_cx<eT>::yes ? 16u : 48u);

    if( A.n_elem <= threshold )
      {
      syrk_emul<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
      }
    else
      {
      #if defined(ARMA_USE_ATLAS)
        {
        if(use_beta == true)
          {
          // use a temporary matrix, as we can't assume that matrix C is already symmetric
          Mat<eT> D(C.n_rows, C.n_cols);

          syrk<do_trans_A, use_alpha, false>::apply_blas_type(D,A,alpha);

          // NOTE: assuming beta=1; this is okay for now, as currently glue_times only uses beta=1
          arrayops::inplace_plus(C.memptr(), D.memptr(), C.n_elem);

          return;
          }

        atlas::cblas_syrk<eT>
          (
          atlas::CblasColMajor,
          atlas::CblasUpper,
          (do_trans_A) ? atlas::CblasTrans : atlas::CblasNoTrans,
          C.n_cols,
          (do_trans_A) ? A.n_rows : A.n_cols,
          (use_alpha) ? alpha : eT(1),
          A.mem,
          (do_trans_A) ? A.n_rows : C.n_cols,
          (use_beta) ? beta : eT(0),
          C.memptr(),
          C.n_cols
          );

        syrk_helper::inplace_copy_upper_tri_to_lower_tri(C);
        }
      #elif defined(ARMA_USE_BLAS)
        {
        if(use_beta == true)
          {
          // use a temporary matrix, as we can't assume that matrix C is already symmetric
          Mat<eT> D(C.n_rows, C.n_cols);

          syrk<do_trans_A, use_alpha, false>::apply_blas_type(D,A,alpha);

          // NOTE: assuming beta=1; this is okay for now, as currently glue_times only uses beta=1
          arrayops::inplace_plus(C.memptr(), D.memptr(), C.n_elem);

          return;
          }

        arma_extra_debug_print("blas::syrk()");

        const char uplo = 'U';

        const char trans_A = (do_trans_A) ? 'T' : 'N';

        const blas_int n = blas_int(C.n_cols);
        const blas_int k = (do_trans_A) ? blas_int(A.n_rows) : blas_int(A.n_cols);

        const eT local_alpha = (use_alpha) ? alpha : eT(1);
        const eT local_beta  = (use_beta)  ? beta  : eT(0);

        const blas_int lda = (do_trans_A) ? k : n;

        arma_extra_debug_print( arma_str::format("blas::syrk(): trans_A = %c") % trans_A );

        blas::syrk<eT>
          (
          &uplo,
          &trans_A,
          &n,
          &k,
          &local_alpha,
          A.mem,
          &lda,
          &local_beta,
          C.memptr(),
          &n // &ldc
          );

        syrk_helper::inplace_copy_upper_tri_to_lower_tri(C);
        }
      #else
        {
        syrk_emul<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
        }
      #endif
      }
    }



  template<typename eT, typename TA>
  inline
  static
  void
  apply( Mat<eT>& C, const TA& A, const eT alpha = eT(1), const eT beta = eT(0) )
    {
    if(is_cx<eT>::no)
      {
      if(A.is_vec())
        {
        syrk_vec<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
        }
      else
        {
        syrk_emul<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
        }
      }
    else
      {
      // handling of complex matrix by syrk_emul() is not yet implemented
      return;
      }
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat<float>& C,
    const TA&         A,
    const float alpha = float(1),
    const float beta  = float(0)
    )
    {
    syrk<do_trans_A, use_alpha, use_beta>::apply_blas_type(C,A,alpha,beta);
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat<double>& C,
    const TA&          A,
    const double alpha = double(1),
    const double beta  = double(0)
    )
    {
    syrk<do_trans_A, use_alpha, use_beta>::apply_blas_type(C,A,alpha,beta);
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat< std::complex<float> >& C,
    const TA&                         A,
    const std::complex<float> alpha = std::complex<float>(1),
    const std::complex<float> beta  = std::complex<float>(0)
    )
    {
    arma_ignore(C);
    arma_ignore(A);
    arma_ignore(alpha);
    arma_ignore(beta);

    // handling of complex matrix by syrk() is not yet implemented
    return;
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat< std::complex<double> >& C,
    const TA&                          A,
    const std::complex<double> alpha = std::complex<double>(1),
    const std::complex<double> beta  = std::complex<double>(0)
    )
    {
    arma_ignore(C);
    arma_ignore(A);
    arma_ignore(alpha);
    arma_ignore(beta);

    // handling of complex matrix by syrk() is not yet implemented
    return;
    }

  };



//! @}
