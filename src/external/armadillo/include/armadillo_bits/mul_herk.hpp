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


//! \addtogroup herk
//! @{



class herk_helper
  {
  public:

  template<typename eT>
  inline
  static
  void
  inplace_conj_copy_upper_tri_to_lower_tri(Mat<eT>& C)
    {
    // under the assumption that C is a square matrix

    const uword N = C.n_rows;

    for(uword k=0; k < N; ++k)
      {
      eT* colmem = C.colptr(k);

      for(uword i=(k+1); i < N; ++i)
        {
        colmem[i] = std::conj( C.at(k,i) );
        }
      }
    }


  template<typename eT>
  static
  arma_hot
  inline
  eT
  dot_conj_row(const uword n_elem, const eT* const A, const Mat<eT>& B, const uword row)
    {
    arma_extra_debug_sigprint();

    typedef typename get_pod_type<eT>::result T;

    T val_real = T(0);
    T val_imag = T(0);

    for(uword i=0; i<n_elem; ++i)
      {
      const std::complex<T>& X = A[i];
      const std::complex<T>& Y = B.at(row,i);

      const T a = X.real();
      const T b = X.imag();

      const T c = Y.real();
      const T d = Y.imag();

      val_real += (a*c) + (b*d);
      val_imag += (b*c) - (a*d);
      }

    return std::complex<T>(val_real, val_imag);
    }

  };



template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class herk_vec
  {
  public:

  template<typename T, typename TA>
  arma_hot
  inline
  static
  void
  apply
    (
          Mat< std::complex<T> >& C,
    const TA&                     A,
    const T                       alpha = T(1),
    const T                       beta  = T(0)
    )
    {
    arma_extra_debug_sigprint();

    typedef std::complex<T> eT;

    const uword A_n_rows = A.n_rows;
    const uword A_n_cols = A.n_cols;

    // for beta != 0, C is assumed to be hermitian

    // do_trans_A == false  ->   C = alpha * A   * A^H + beta*C
    // do_trans_A == true   ->   C = alpha * A^H * A   + beta*C

    const eT* A_mem = A.memptr();

    if(do_trans_A == false)
      {
      if(A_n_rows == 1)
        {
        const eT acc = op_cdot::direct_cdot(A_n_cols, A_mem, A_mem);

             if( (use_alpha == false) && (use_beta == false) )  { C[0] =       acc;             }
        else if( (use_alpha == true ) && (use_beta == false) )  { C[0] = alpha*acc;             }
        else if( (use_alpha == false) && (use_beta == true ) )  { C[0] =       acc + beta*C[0]; }
        else if( (use_alpha == true ) && (use_beta == true ) )  { C[0] = alpha*acc + beta*C[0]; }
        }
      else
      for(uword row_A=0; row_A < A_n_rows; ++row_A)
        {
        const eT& A_rowdata = A_mem[row_A];

        for(uword k=row_A; k < A_n_rows; ++k)
          {
          const eT acc = A_rowdata * std::conj( A_mem[k] );

          if( (use_alpha == false) && (use_beta == false) )
            {
                              C.at(row_A, k) = acc;
            if(row_A != k)  { C.at(k, row_A) = std::conj(acc); }
            }
          else
          if( (use_alpha == true) && (use_beta == false) )
            {
            const eT val = alpha*acc;

                              C.at(row_A, k) = val;
            if(row_A != k)  { C.at(k, row_A) = std::conj(val); }
            }
          else
          if( (use_alpha == false) && (use_beta == true) )
            {
                              C.at(row_A, k) =           acc  + beta*C.at(row_A, k);
            if(row_A != k)  { C.at(k, row_A) = std::conj(acc) + beta*C.at(k, row_A); }
            }
          else
          if( (use_alpha == true) && (use_beta == true) )
            {
            const eT val = alpha*acc;

                              C.at(row_A, k) =           val  + beta*C.at(row_A, k);
            if(row_A != k)  { C.at(k, row_A) = std::conj(val) + beta*C.at(k, row_A); }
            }
          }
        }
      }
    else
    if(do_trans_A == true)
      {
      if(A_n_cols == 1)
        {
        const eT acc = op_cdot::direct_cdot(A_n_rows, A_mem, A_mem);

             if( (use_alpha == false) && (use_beta == false) )  { C[0] =       acc;             }
        else if( (use_alpha == true ) && (use_beta == false) )  { C[0] = alpha*acc;             }
        else if( (use_alpha == false) && (use_beta == true ) )  { C[0] =       acc + beta*C[0]; }
        else if( (use_alpha == true ) && (use_beta == true ) )  { C[0] = alpha*acc + beta*C[0]; }
        }
      else
      for(uword col_A=0; col_A < A_n_cols; ++col_A)
        {
        // col_A is interpreted as row_A when storing the results in matrix C

        const eT A_coldata = std::conj( A_mem[col_A] );

        for(uword k=col_A; k < A_n_cols ; ++k)
          {
          const eT acc = A_coldata * A_mem[k];

          if( (use_alpha == false) && (use_beta == false) )
            {
                              C.at(col_A, k) = acc;
            if(col_A != k)  { C.at(k, col_A) = std::conj(acc); }
            }
          else
          if( (use_alpha == true ) && (use_beta == false) )
            {
            const eT val = alpha*acc;

                              C.at(col_A, k) = val;
            if(col_A != k)  { C.at(k, col_A) = std::conj(val); }
            }
          else
          if( (use_alpha == false) && (use_beta == true ) )
            {
                              C.at(col_A, k) =           acc  + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = std::conj(acc) + beta*C.at(k, col_A); }
            }
          else
          if( (use_alpha == true ) && (use_beta == true ) )
            {
            const eT val = alpha*acc;

                              C.at(col_A, k) =           val  + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = std::conj(val) + beta*C.at(k, col_A); }
            }
          }
        }
      }
    }

  };



template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class herk_emul
  {
  public:

  template<typename T, typename TA>
  arma_hot
  inline
  static
  void
  apply
    (
          Mat< std::complex<T> >& C,
    const TA&                     A,
    const T                       alpha = T(1),
    const T                       beta  = T(0)
    )
    {
    arma_extra_debug_sigprint();

    typedef std::complex<T> eT;

    // do_trans_A == false  ->   C = alpha * A   * A^H + beta*C
    // do_trans_A == true   ->   C = alpha * A^H * A   + beta*C

    if(do_trans_A == false)
      {
      Mat<eT> AA;

      op_htrans::apply_mat_noalias(AA, A);

      herk_emul<true, use_alpha, use_beta>::apply(C, AA, alpha, beta);
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

        for(uword k=col_A; k < A_n_cols ; ++k)
          {
          const eT acc = op_cdot::direct_cdot(A_n_rows, A_coldata, A.colptr(k));

          if( (use_alpha == false) && (use_beta == false) )
            {
                              C.at(col_A, k) = acc;
            if(col_A != k)  { C.at(k, col_A) = std::conj(acc); }
            }
          else
          if( (use_alpha == true) && (use_beta == false) )
            {
            const eT val = alpha*acc;

                              C.at(col_A, k) = val;
            if(col_A != k)  { C.at(k, col_A) = std::conj(val); }
            }
          else
          if( (use_alpha == false) && (use_beta == true) )
            {
                              C.at(col_A, k) =           acc  + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = std::conj(acc) + beta*C.at(k, col_A); }
            }
          else
          if( (use_alpha == true) && (use_beta == true) )
            {
            const eT val = alpha*acc;

                              C.at(col_A, k) =           val  + beta*C.at(col_A, k);
            if(col_A != k)  { C.at(k, col_A) = std::conj(val) + beta*C.at(k, col_A); }
            }
          }
        }
      }
    }

  };



template<const bool do_trans_A=false, const bool use_alpha=false, const bool use_beta=false>
class herk
  {
  public:

  template<typename T, typename TA>
  inline
  static
  void
  apply_blas_type( Mat<std::complex<T> >& C, const TA& A, const T alpha = T(1), const T beta = T(0) )
    {
    arma_extra_debug_sigprint();

    const uword threshold = 16;

    if(A.is_vec())
      {
      // work around poor handling of vectors by herk() in ATLAS 3.8.4 and standard BLAS

      herk_vec<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);

      return;
      }


    if( (A.n_elem <= threshold) )
      {
      herk_emul<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
      }
    else
      {
      #if defined(ARMA_USE_ATLAS)
        {
        if(use_beta == true)
          {
          typedef typename std::complex<T> eT;

          // use a temporary matrix, as we can't assume that matrix C is already symmetric
          Mat<eT> D(C.n_rows, C.n_cols);

          herk<do_trans_A, use_alpha, false>::apply_blas_type(D,A,alpha);

          // NOTE: assuming beta=1; this is okay for now, as currently glue_times only uses beta=1
          arrayops::inplace_plus(C.memptr(), D.memptr(), C.n_elem);

          return;
          }

        atlas::cblas_herk<T>
          (
          atlas::CblasColMajor,
          atlas::CblasUpper,
          (do_trans_A) ? CblasConjTrans : atlas::CblasNoTrans,
          C.n_cols,
          (do_trans_A) ? A.n_rows : A.n_cols,
          (use_alpha) ? alpha : T(1),
          A.mem,
          (do_trans_A) ? A.n_rows : C.n_cols,
          (use_beta) ? beta : T(0),
          C.memptr(),
          C.n_cols
          );

        herk_helper::inplace_conj_copy_upper_tri_to_lower_tri(C);
        }
      #elif defined(ARMA_USE_BLAS)
        {
        if(use_beta == true)
          {
          typedef typename std::complex<T> eT;

          // use a temporary matrix, as we can't assume that matrix C is already symmetric
          Mat<eT> D(C.n_rows, C.n_cols);

          herk<do_trans_A, use_alpha, false>::apply_blas_type(D,A,alpha);

          // NOTE: assuming beta=1; this is okay for now, as currently glue_times only uses beta=1
          arrayops::inplace_plus(C.memptr(), D.memptr(), C.n_elem);

          return;
          }

        arma_extra_debug_print("blas::herk()");

        const char uplo = 'U';

        const char trans_A = (do_trans_A) ? 'C' : 'N';

        const blas_int n = blas_int(C.n_cols);
        const blas_int k = (do_trans_A) ? blas_int(A.n_rows) : blas_int(A.n_cols);

        const T local_alpha = (use_alpha) ? alpha : T(1);
        const T local_beta  = (use_beta)  ? beta  : T(0);

        const blas_int lda = (do_trans_A) ? k : n;

        arma_extra_debug_print( arma_str::format("blas::herk(): trans_A = %c") % trans_A );

        blas::herk<T>
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

        herk_helper::inplace_conj_copy_upper_tri_to_lower_tri(C);
        }
      #else
        {
        herk_emul<do_trans_A, use_alpha, use_beta>::apply(C,A,alpha,beta);
        }
      #endif
      }

    }



  template<typename eT, typename TA>
  inline
  static
  void
  apply( Mat<eT>& C, const TA& A, const eT alpha = eT(1), const eT beta = eT(0), const typename arma_not_cx<eT>::result* junk = 0 )
    {
    arma_ignore(C);
    arma_ignore(A);
    arma_ignore(alpha);
    arma_ignore(beta);
    arma_ignore(junk);

    // herk() cannot be used by non-complex matrices

    return;
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat< std::complex<float> >& C,
    const TA&                         A,
    const float                       alpha = float(1),
    const float                       beta  = float(0)
    )
    {
    herk<do_trans_A, use_alpha, use_beta>::apply_blas_type(C,A,alpha,beta);
    }



  template<typename TA>
  arma_inline
  static
  void
  apply
    (
          Mat< std::complex<double> >& C,
    const TA&                          A,
    const double                       alpha = double(1),
    const double                       beta  = double(0)
    )
    {
    herk<do_trans_A, use_alpha, use_beta>::apply_blas_type(C,A,alpha,beta);
    }

  };



//! @}
