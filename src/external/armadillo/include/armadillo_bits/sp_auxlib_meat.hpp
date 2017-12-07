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


//! \addtogroup sp_auxlib
//! @{


inline
sp_auxlib::form_type
sp_auxlib::interpret_form_str(const char* form_str)
  {
  arma_extra_debug_sigprint();

  // the order of the 3 if statements below is important
  if( form_str    == NULL    )  { return form_none; }
  if( form_str[0] == char(0) )  { return form_none; }
  if( form_str[1] == char(0) )  { return form_none; }

  const char c1 = form_str[0];
  const char c2 = form_str[1];

  if(c1 == 'l')
    {
    if(c2 == 'm')  { return form_lm; }
    if(c2 == 'r')  { return form_lr; }
    if(c2 == 'i')  { return form_li; }
    if(c2 == 'a')  { return form_la; }
    }
  else
  if(c1 == 's')
    {
    if(c2 == 'm')  { return form_sm; }
    if(c2 == 'r')  { return form_sr; }
    if(c2 == 'i')  { return form_si; }
    if(c2 == 'a')  { return form_sa; }
    }

  return form_none;
  }



//! immediate eigendecomposition of symmetric real sparse object
template<typename eT, typename T1>
inline
bool
sp_auxlib::eigs_sym(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol)
  {
  arma_extra_debug_sigprint();

  #if   defined(ARMA_USE_NEWARP)
    {
    return sp_auxlib::eigs_sym_newarp(eigval, eigvec, X, n_eigvals, form_str, default_tol);
    }
  #elif defined(ARMA_USE_ARPACK)
    {
    return sp_auxlib::eigs_sym_arpack(eigval, eigvec, X, n_eigvals, form_str, default_tol);
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    arma_stop_logic_error("eigs_sym(): use of NEWARP or ARPACK must be enabled");
    return false;
    }
  #endif
  }



template<typename eT, typename T1>
inline
bool
sp_auxlib::eigs_sym_newarp(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_NEWARP)
    {
    const form_type form_val = sp_auxlib::interpret_form_str(form_str);

    arma_debug_check( (form_val != form_lm) && (form_val != form_sm) && (form_val != form_la) && (form_val != form_sa), "eigs_sym(): unknown form specified" );

    const newarp::SparseGenMatProd<eT> op(X.get_ref());

    arma_debug_check( (op.n_rows != op.n_cols), "eigs_sym(): given matrix must be square sized" );

    arma_debug_check( (n_eigvals >= op.n_rows), "eigs_sym(): n_eigvals must be less than the number of rows in the matrix" );

    // If the matrix is empty, the case is trivial.
    if( (op.n_cols == 0) || (n_eigvals == 0) ) // We already know n_cols == n_rows.
      {
      eigval.reset();
      eigvec.reset();
      return true;
      }

    uword n   = op.n_rows;
    uword ncv = n_eigvals + 2 + 1;

    if(ncv < (2 * n_eigvals + 1)) { ncv = 2 * n_eigvals + 1; }
    if(ncv > n)                   { ncv = n; }

    eT tol = (std::max)(default_tol, std::numeric_limits<eT>::epsilon());

    // eigval.set_size(n_eigvals);
    // eigvec.set_size(n, n_eigvals);

    bool status = true;

    uword nconv = 0;

    try
      {
      if(form_val == form_lm)
        {
        newarp::SymEigsSolver< eT, newarp::EigsSelect::LARGEST_MAGN, newarp::SparseGenMatProd<eT> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_sm)
        {
        newarp::SymEigsSolver< eT, newarp::EigsSelect::SMALLEST_MAGN, newarp::SparseGenMatProd<eT> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_la)
        {
        newarp::SymEigsSolver< eT, newarp::EigsSelect::LARGEST_ALGE, newarp::SparseGenMatProd<eT> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_sa)
        {
        newarp::SymEigsSolver< eT, newarp::EigsSelect::SMALLEST_ALGE, newarp::SparseGenMatProd<eT> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      }
    catch(const std::runtime_error&)
      {
      status = false;
      }

    if(status == true)
      {
      if(nconv == 0)  { status = false; }
      }

    return status;
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    return false;
    }
  #endif
  }



template<typename eT, typename T1>
inline
bool
sp_auxlib::eigs_sym_arpack(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_ARPACK)
    {
    const form_type form_val = sp_auxlib::interpret_form_str(form_str);

    arma_debug_check( (form_val != form_lm) && (form_val != form_sm) && (form_val != form_la) && (form_val != form_sa), "eigs_sym(): unknown form specified" );

    char  which_sm[3] = "SM";
    char  which_lm[3] = "LM";
    char  which_sa[3] = "SA";
    char  which_la[3] = "LA";
    char* which;
    switch (form_val)
      {
      case form_sm:  which = which_sm;  break;
      case form_lm:  which = which_lm;  break;
      case form_sa:  which = which_sa;  break;
      case form_la:  which = which_la;  break;

      default:       which = which_lm;  break;
      }

    // Make a sparse proxy object.
    SpProxy<T1> p(X.get_ref());

    // Make sure it's square.
    arma_debug_check( (p.get_n_rows() != p.get_n_cols()), "eigs_sym(): given matrix must be square sized" );

    // Make sure we aren't asking for every eigenvalue.
    // The _saupd() functions allow asking for one more eigenvalue than the _naupd() functions.
    arma_debug_check( (n_eigvals >= p.get_n_rows()), "eigs_sym(): n_eigvals must be less than the number of rows in the matrix" );

    // If the matrix is empty, the case is trivial.
    if( (p.get_n_cols() == 0) || (n_eigvals == 0) ) // We already know n_cols == n_rows.
      {
      eigval.reset();
      eigvec.reset();
      return true;
      }

    // Set up variables that get used for neupd().
    blas_int n, ncv, ldv, lworkl, info;
    eT tol = default_tol;
    podarray<eT> resid, v, workd, workl;
    podarray<blas_int> iparam, ipntr;
    podarray<eT> rwork; // Not used in this case.

    run_aupd(n_eigvals, which, p, true /* sym, not gen */, n, tol, resid, ncv, v, ldv, iparam, ipntr, workd, workl, lworkl, rwork, info);

    if(info != 0)
      {
      return false;
      }

    // The process has converged, and now we need to recover the actual eigenvectors using seupd()
    blas_int rvec = 1; // .TRUE
    blas_int nev  = n_eigvals;

    char howmny = 'A';
    char bmat   = 'I'; // We are considering the standard eigenvalue problem.

    podarray<blas_int> select(ncv); // Logical array of dimension NCV.
    blas_int ldz = n;

    // seupd() will output directly into the eigval and eigvec objects.
    eigval.zeros(n_eigvals);
    eigvec.zeros(n, n_eigvals);

    arpack::seupd(&rvec, &howmny, select.memptr(), eigval.memptr(), eigvec.memptr(), &ldz, (eT*) NULL, &bmat, &n, which, &nev, &tol, resid.memptr(), &ncv, v.memptr(), &ldv, iparam.memptr(), ipntr.memptr(), workd.memptr(), workl.memptr(), &lworkl, &info);

    // Check for errors.
    if(info != 0)
      {
      arma_debug_warn("eigs_sym(): ARPACK error ", info, " in seupd()");
      return false;
      }

    return (info == 0);
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    return false;
    }
  #endif
  }



//! immediate eigendecomposition of non-symmetric real sparse object
template<typename T, typename T1>
inline
bool
sp_auxlib::eigs_gen(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_NEWARP)
    {
    return sp_auxlib::eigs_gen_newarp(eigval, eigvec, X, n_eigvals, form_str, default_tol);
    }
  #elif defined(ARMA_USE_ARPACK)
    {
    return sp_auxlib::eigs_gen_arpack(eigval, eigvec, X, n_eigvals, form_str, default_tol);
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    arma_stop_logic_error("eigs_gen(): use of NEWARP or ARPACK must be enabled");
    return false;
    }
  #endif
  }



template<typename T, typename T1>
inline
bool
sp_auxlib::eigs_gen_newarp(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_NEWARP)
    {
    const form_type form_val = sp_auxlib::interpret_form_str(form_str);

    arma_debug_check( (form_val == form_none), "eigs_gen(): unknown form specified" );

    const newarp::SparseGenMatProd<T> op(X.get_ref());

    arma_debug_check( (op.n_rows != op.n_cols), "eigs_sym(): given matrix must be square sized" );

    arma_debug_check( (n_eigvals + 1 >= op.n_rows), "eigs_gen(): n_eigvals + 1 must be less than the number of rows in the matrix" );

    // If the matrix is empty, the case is trivial.
    if( (op.n_cols == 0) || (n_eigvals == 0) ) // We already know n_cols == n_rows.
      {
      eigval.reset();
      eigvec.reset();
      return true;
      }

    uword n   = op.n_rows;
    uword ncv = n_eigvals + 2 + 1;

    if(ncv < (2 * n_eigvals + 1)) { ncv = 2 * n_eigvals + 1; }
    if(ncv > n)                   { ncv = n; }

    T tol = (std::max)(default_tol, std::numeric_limits<T>::epsilon());

    // eigval.set_size(n_eigvals);
    // eigvec.set_size(n, n_eigvals);

    bool status = true;

    uword nconv = 0;

    try
      {
      if(form_val == form_lm)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::LARGEST_MAGN, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_sm)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::SMALLEST_MAGN, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_lr)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::LARGEST_REAL, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_sr)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::SMALLEST_REAL, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_li)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::LARGEST_IMAG, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      else
      if(form_val == form_si)
        {
        newarp::GenEigsSolver< T, newarp::EigsSelect::SMALLEST_IMAG, newarp::SparseGenMatProd<T> > eigs(op, n_eigvals, ncv);
        eigs.init();
        nconv  = eigs.compute(1000, tol);
        eigval = eigs.eigenvalues();
        eigvec = eigs.eigenvectors();
        }
      }
    catch(const std::runtime_error&)
      {
      status = false;
      }

    if(status == true)
      {
      if(nconv == 0)  { status = false; }
      }

    return status;
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    return false;
    }
  #endif
  }




template<typename T, typename T1>
inline
bool
sp_auxlib::eigs_gen_arpack(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_ARPACK)
    {
    const form_type form_val = sp_auxlib::interpret_form_str(form_str);

    arma_debug_check( (form_val == form_none), "eigs_gen(): unknown form specified" );

    char which_lm[3] = "LM";
    char which_sm[3] = "SM";
    char which_lr[3] = "LR";
    char which_sr[3] = "SR";
    char which_li[3] = "LI";
    char which_si[3] = "SI";

    char* which;

    switch(form_val)
      {
      case form_lm:  which = which_lm;  break;
      case form_sm:  which = which_sm;  break;
      case form_lr:  which = which_lr;  break;
      case form_sr:  which = which_sr;  break;
      case form_li:  which = which_li;  break;
      case form_si:  which = which_si;  break;

      default:       which = which_lm;
      }


    // Make a sparse proxy object.
    SpProxy<T1> p(X.get_ref());

    // Make sure it's square.
    arma_debug_check( (p.get_n_rows() != p.get_n_cols()), "eigs_gen(): given matrix must be square sized" );

    // Make sure we aren't asking for every eigenvalue.
    arma_debug_check( (n_eigvals + 1 >= p.get_n_rows()), "eigs_gen(): n_eigvals + 1 must be less than the number of rows in the matrix" );

    // If the matrix is empty, the case is trivial.
    if( (p.get_n_cols() == 0) || (n_eigvals == 0) ) // We already know n_cols == n_rows.
      {
      eigval.reset();
      eigvec.reset();
      return true;
      }

    // Set up variables that get used for neupd().
    blas_int n, ncv, ldv, lworkl, info;
    T tol = default_tol;
    podarray<T> resid, v, workd, workl;
    podarray<blas_int> iparam, ipntr;
    podarray<T> rwork; // Not used in the real case.

    run_aupd(n_eigvals, which, p, false /* gen, not sym */, n, tol, resid, ncv, v, ldv, iparam, ipntr, workd, workl, lworkl, rwork, info);

    if(info != 0)
      {
      return false;
      }

    // The process has converged, and now we need to recover the actual eigenvectors using neupd().
    blas_int rvec = 1; // .TRUE
    blas_int nev = n_eigvals;

    char howmny = 'A';
    char bmat   = 'I'; // We are considering the standard eigenvalue problem.

    podarray<blas_int> select(ncv); // Logical array of dimension NCV.
    podarray<T> dr(nev + 1); // Real array of dimension NEV + 1.
    podarray<T> di(nev + 1); // Real array of dimension NEV + 1.
    podarray<T> z(n * (nev + 1)); // Real N by NEV array if HOWMNY = 'A'.
    blas_int ldz = n;
    podarray<T> workev(3 * ncv);

    dr.zeros();
    di.zeros();
    z.zeros();

    arpack::neupd(&rvec, &howmny, select.memptr(), dr.memptr(), di.memptr(), z.memptr(), &ldz, (T*) NULL, (T*) NULL, workev.memptr(), &bmat, &n, which, &nev, &tol, resid.memptr(), &ncv, v.memptr(), &ldv, iparam.memptr(), ipntr.memptr(), workd.memptr(), workl.memptr(), &lworkl, rwork.memptr(), &info);

    // Check for errors.
    if(info != 0)
      {
      arma_debug_warn("eigs_gen(): ARPACK error ", info, " in neupd()");
      return false;
      }

    // Put it into the outputs.
    eigval.set_size(n_eigvals);
    eigvec.zeros(n, n_eigvals);

    for (uword i = 0; i < n_eigvals; ++i)
      {
      eigval[i] = std::complex<T>(dr[i], di[i]);
      }

    // Now recover the eigenvectors.
    for (uword i = 0; i < n_eigvals; ++i)
      {
      // ARPACK ?neupd lays things out kinda odd in memory;
      // so does LAPACK ?geev -- see auxlib::eig_gen()
      if((i < n_eigvals - 1) && (eigval[i] == std::conj(eigval[i + 1])))
        {
        for (uword j = 0; j < uword(n); ++j)
          {
          eigvec.at(j, i)     = std::complex<T>(z[n * i + j],  z[n * (i + 1) + j]);
          eigvec.at(j, i + 1) = std::complex<T>(z[n * i + j], -z[n * (i + 1) + j]);
          }
        ++i; // Skip the next one.
        }
      else
      if((i == n_eigvals - 1) && (std::complex<T>(eigval[i]).imag() != 0.0))
        {
        // We don't have the matched conjugate eigenvalue.
        for (uword j = 0; j < uword(n); ++j)
          {
          eigvec.at(j, i) = std::complex<T>(z[n * i + j], z[n * (i + 1) + j]);
          }
        }
      else
        {
        // The eigenvector is entirely real.
        for (uword j = 0; j < uword(n); ++j)
          {
          eigvec.at(j, i) = std::complex<T>(z[n * i + j], T(0));
          }
        }
      }

    return (info == 0);
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    return false;
    }
  #endif
  }



//! immediate eigendecomposition of non-symmetric complex sparse object
template<typename T, typename T1>
inline
bool
sp_auxlib::eigs_gen(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase< std::complex<T>, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_ARPACK)
    {
    const form_type form_val = sp_auxlib::interpret_form_str(form_str);

    arma_debug_check( (form_val == form_none), "eigs_gen(): unknown form specified" );

    char which_lm[3] = "LM";
    char which_sm[3] = "SM";
    char which_lr[3] = "LR";
    char which_sr[3] = "SR";
    char which_li[3] = "LI";
    char which_si[3] = "SI";

    char* which;

    switch(form_val)
      {
      case form_lm:  which = which_lm;  break;
      case form_sm:  which = which_sm;  break;
      case form_lr:  which = which_lr;  break;
      case form_sr:  which = which_sr;  break;
      case form_li:  which = which_li;  break;
      case form_si:  which = which_si;  break;

      default:       which = which_lm;
      }


    // Make a sparse proxy object.
    SpProxy<T1> p(X.get_ref());

    // Make sure it's square.
    arma_debug_check( (p.get_n_rows() != p.get_n_cols()), "eigs_gen(): given matrix must be square sized" );

    // Make sure we aren't asking for every eigenvalue.
    arma_debug_check( (n_eigvals + 1 >= p.get_n_rows()), "eigs_gen(): n_eigvals + 1 must be less than the number of rows in the matrix" );

    // If the matrix is empty, the case is trivial.
    if( (p.get_n_cols() == 0) || (n_eigvals == 0) ) // We already know n_cols == n_rows.
      {
      eigval.reset();
      eigvec.reset();
      return true;
      }

    // Set up variables that get used for neupd().
    blas_int n, ncv, ldv, lworkl, info;
    T tol = default_tol;
    podarray< std::complex<T> > resid, v, workd, workl;
    podarray<blas_int> iparam, ipntr;
    podarray<T> rwork;

    run_aupd(n_eigvals, which, p, false /* gen, not sym */, n, tol, resid, ncv, v, ldv, iparam, ipntr, workd, workl, lworkl, rwork, info);

    if(info != 0)
      {
      return false;
      }

    // The process has converged, and now we need to recover the actual eigenvectors using neupd().
    blas_int rvec = 1; // .TRUE
    blas_int nev  = n_eigvals;

    char howmny = 'A';
    char bmat   = 'I'; // We are considering the standard eigenvalue problem.

    podarray<blas_int> select(ncv); // Logical array of dimension NCV.
    podarray<std::complex<T> > d(nev + 1); // Real array of dimension NEV + 1.
    podarray<std::complex<T> > z(n * nev); // Real N by NEV array if HOWMNY = 'A'.
    blas_int ldz = n;
    podarray<std::complex<T> > workev(2 * ncv);

    // Prepare the outputs; neupd() will write directly to them.
    eigval.zeros(n_eigvals);
    eigvec.zeros(n, n_eigvals);
    std::complex<T> sigma;

    arpack::neupd(&rvec, &howmny, select.memptr(), eigval.memptr(),
(std::complex<T>*) NULL, eigvec.memptr(), &ldz, (std::complex<T>*) &sigma, (std::complex<T>*) NULL, workev.memptr(), &bmat, &n, which, &nev, &tol, resid.memptr(), &ncv, v.memptr(), &ldv, iparam.memptr(), ipntr.memptr(), workd.memptr(), workl.memptr(), &lworkl, rwork.memptr(), &info);

    // Check for errors.
    if(info != 0)
      {
      arma_debug_warn("eigs_gen(): ARPACK error ", info, " in neupd()");
      return false;
      }

    return (info == 0);
    }
  #else
    {
    arma_ignore(eigval);
    arma_ignore(eigvec);
    arma_ignore(X);
    arma_ignore(n_eigvals);
    arma_ignore(form_str);
    arma_ignore(default_tol);

    arma_stop_logic_error("eigs_gen(): use of ARPACK must be enabled for decomposition of complex matrices");
    return false;
    }
  #endif
  }



template<typename T1, typename T2>
inline
bool
sp_auxlib::spsolve_simple(Mat<typename T1::elem_type>& X, const SpBase<typename T1::elem_type, T1>& A_expr, const Base<typename T1::elem_type, T2>& B_expr, const superlu_opts& user_opts)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_SUPERLU)
    {
    typedef typename T1::elem_type eT;

    superlu::superlu_options_t  options;
    sp_auxlib::set_superlu_opts(options, user_opts);

    const unwrap_spmat<T1> tmp1(A_expr.get_ref());
    const SpMat<eT>& A =   tmp1.M;

    X = B_expr.get_ref();   // superlu::gssv() uses X as input (the B matrix) and as output (the solution)

    if(A.n_rows > A.n_cols)
      {
      arma_stop_logic_error("spsolve(): solving over-determined systems currently not supported");
      X.soft_reset();
      return false;
      }
    else if(A.n_rows < A.n_cols)
      {
      arma_stop_logic_error("spsolve(): solving under-determined systems currently not supported");
      X.soft_reset();
      return false;
      }

    arma_debug_check( (A.n_rows != X.n_rows), "spsolve(): number of rows in the given objects must be the same" );

    if(A.is_empty() || X.is_empty())
      {
      X.zeros(A.n_cols, X.n_cols);
      return true;
      }

    if(arma_config::debug)
      {
      bool overflow;

      overflow = (A.n_nonzero > INT_MAX);
      overflow = (A.n_rows > INT_MAX) || overflow;
      overflow = (A.n_cols > INT_MAX) || overflow;
      overflow = (X.n_rows > INT_MAX) || overflow;
      overflow = (X.n_cols > INT_MAX) || overflow;

      if(overflow)
        {
        arma_stop_runtime_error("spsolve(): integer overflow: matrix dimensions are too large for integer type used by SuperLU");
        return false;
        }
      }

    superlu::SuperMatrix x;  arrayops::inplace_set(reinterpret_cast<char*>(&x), char(0), sizeof(superlu::SuperMatrix));
    superlu::SuperMatrix a;  arrayops::inplace_set(reinterpret_cast<char*>(&a), char(0), sizeof(superlu::SuperMatrix));

    const bool status_x = wrap_to_supermatrix(x, X);
    const bool status_a = copy_to_supermatrix(a, A);

    if( (status_x == false) || (status_a == false) )
      {
      destroy_supermatrix(a);
      destroy_supermatrix(x);
      X.soft_reset();
      return false;
      }

    superlu::SuperMatrix l;  arrayops::inplace_set(reinterpret_cast<char*>(&l), char(0), sizeof(superlu::SuperMatrix));
    superlu::SuperMatrix u;  arrayops::inplace_set(reinterpret_cast<char*>(&u), char(0), sizeof(superlu::SuperMatrix));

    // paranoia: use SuperLU's memory allocation, in case it reallocs

    int* perm_c = (int*) superlu::malloc( (A.n_cols+1) * sizeof(int));  // extra paranoia: increase array length by 1
    int* perm_r = (int*) superlu::malloc( (A.n_rows+1) * sizeof(int));

    arma_check_bad_alloc( (perm_c == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (perm_r == 0), "spsolve(): out of memory" );

    arrayops::inplace_set(perm_c, 0, A.n_cols+1);
    arrayops::inplace_set(perm_r, 0, A.n_rows+1);

    superlu::SuperLUStat_t stat;
    superlu::init_stat(&stat);

    int info = 0; // Return code.

    superlu::gssv<eT>(&options, &a, perm_c, perm_r, &l, &u, &x, &stat, &info);


    // Process the return code.
    if( (info > 0) && (info <= int(A.n_cols)) )
      {
      // std::stringstream tmp;
      // tmp << "spsolve(): could not solve system; LU factorisation completed, but detected zero in U(" << (info-1) << ',' << (info-1) << ')';
      // arma_debug_warn(tmp.str());
      }
    else
    if(info > int(A.n_cols))
      {
      arma_debug_warn("spsolve(): memory allocation failure: could not allocate ", (info - int(A.n_cols)), " bytes");
      }
    else
    if(info < 0)
      {
      arma_debug_warn("spsolve(): unknown SuperLU error code from gssv(): ", info);
      }


    superlu::free_stat(&stat);

    superlu::free(perm_c);
    superlu::free(perm_r);

    destroy_supermatrix(u);
    destroy_supermatrix(l);
    destroy_supermatrix(a);
    destroy_supermatrix(x);  // No need to extract the data from x, since it's using the same memory as X

    return (info == 0);
    }
  #else
    {
    arma_ignore(X);
    arma_ignore(A_expr);
    arma_ignore(B_expr);
    arma_ignore(user_opts);
    arma_stop_logic_error("spsolve(): use of SuperLU must be enabled");
    return false;
    }
  #endif
  }



template<typename T1, typename T2>
inline
bool
sp_auxlib::spsolve_refine(Mat<typename T1::elem_type>& X, typename T1::pod_type& out_rcond, const SpBase<typename T1::elem_type, T1>& A_expr, const Base<typename T1::elem_type, T2>& B_expr, const superlu_opts& user_opts)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_SUPERLU)
    {
    typedef typename T1::pod_type   T;
    typedef typename T1::elem_type eT;

    superlu::superlu_options_t  options;
    sp_auxlib::set_superlu_opts(options, user_opts);

    const unwrap_spmat<T1> tmp1(A_expr.get_ref());
    const SpMat<eT>& A =   tmp1.M;

    const unwrap<T2>          tmp2(B_expr.get_ref());
    const Mat<eT>& B_unwrap = tmp2.M;

    const bool B_is_modified = ( (user_opts.equilibrate) || (&B_unwrap == &X) );

    Mat<eT> B_copy;  if(B_is_modified)  { B_copy = B_unwrap; }

    const Mat<eT>& B = (B_is_modified) ?  B_copy : B_unwrap;

    if(A.n_rows > A.n_cols)
      {
      arma_stop_logic_error("spsolve(): solving over-determined systems currently not supported");
      X.soft_reset();
      return false;
      }
    else if(A.n_rows < A.n_cols)
      {
      arma_stop_logic_error("spsolve(): solving under-determined systems currently not supported");
      X.soft_reset();
      return false;
      }

    arma_debug_check( (A.n_rows != B.n_rows), "spsolve(): number of rows in the given objects must be the same" );

    X.zeros(A.n_cols, B.n_cols);  // set the elements to zero, as we don't trust the SuperLU spaghetti code

    if(A.is_empty() || B.is_empty())
      {
      return true;
      }

    if(arma_config::debug)
      {
      bool overflow;

      overflow = (A.n_nonzero > INT_MAX);
      overflow = (A.n_rows > INT_MAX) || overflow;
      overflow = (A.n_cols > INT_MAX) || overflow;
      overflow = (B.n_rows > INT_MAX) || overflow;
      overflow = (B.n_cols > INT_MAX) || overflow;
      overflow = (X.n_rows > INT_MAX) || overflow;
      overflow = (X.n_cols > INT_MAX) || overflow;

      if(overflow)
        {
        arma_stop_runtime_error("spsolve(): integer overflow: matrix dimensions are too large for integer type used by SuperLU");
        return false;
        }
      }

    superlu::SuperMatrix x;  arrayops::inplace_set(reinterpret_cast<char*>(&x), char(0), sizeof(superlu::SuperMatrix));
    superlu::SuperMatrix a;  arrayops::inplace_set(reinterpret_cast<char*>(&a), char(0), sizeof(superlu::SuperMatrix));
    superlu::SuperMatrix b;  arrayops::inplace_set(reinterpret_cast<char*>(&b), char(0), sizeof(superlu::SuperMatrix));

    const bool status_x = wrap_to_supermatrix(x, X);
    const bool status_a = copy_to_supermatrix(a, A);  // NOTE: superlu::gssvx() modifies 'a' if equilibration is enabled
    const bool status_b = wrap_to_supermatrix(b, B);  // NOTE: superlu::gssvx() modifies 'b' if equilibration is enabled

    if( (status_x == false) || (status_a == false) || (status_b == false) )
      {
      destroy_supermatrix(x);
      destroy_supermatrix(a);
      destroy_supermatrix(b);
      X.soft_reset();
      return false;
      }

    superlu::SuperMatrix l;  arrayops::inplace_set(reinterpret_cast<char*>(&l), char(0), sizeof(superlu::SuperMatrix));
    superlu::SuperMatrix u;  arrayops::inplace_set(reinterpret_cast<char*>(&u), char(0), sizeof(superlu::SuperMatrix));

    // paranoia: use SuperLU's memory allocation, in case it reallocs

    int* perm_c = (int*) superlu::malloc( (A.n_cols+1) * sizeof(int) );  // extra paranoia: increase array length by 1
    int* perm_r = (int*) superlu::malloc( (A.n_rows+1) * sizeof(int) );
    int* etree  = (int*) superlu::malloc( (A.n_cols+1) * sizeof(int) );

    T* R    = (T*) superlu::malloc( (A.n_rows+1) * sizeof(T) );
    T* C    = (T*) superlu::malloc( (A.n_cols+1) * sizeof(T) );
    T* ferr = (T*) superlu::malloc( (B.n_cols+1) * sizeof(T) );
    T* berr = (T*) superlu::malloc( (B.n_cols+1) * sizeof(T) );

    arma_check_bad_alloc( (perm_c == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (perm_r == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (etree  == 0), "spsolve(): out of memory" );

    arma_check_bad_alloc( (R    == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (C    == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (ferr == 0), "spsolve(): out of memory" );
    arma_check_bad_alloc( (berr == 0), "spsolve(): out of memory" );

    arrayops::inplace_set(perm_c, int(0), A.n_cols+1);
    arrayops::inplace_set(perm_r, int(0), A.n_rows+1);
    arrayops::inplace_set(etree,  int(0), A.n_cols+1);

    arrayops::inplace_set(R,    T(0), A.n_rows+1);
    arrayops::inplace_set(C,    T(0), A.n_cols+1);
    arrayops::inplace_set(ferr, T(0), B.n_cols+1);
    arrayops::inplace_set(berr, T(0), B.n_cols+1);

    superlu::GlobalLU_t glu;
    arrayops::inplace_set(reinterpret_cast<char*>(&glu), char(0), sizeof(superlu::GlobalLU_t));

    superlu::mem_usage_t  mu;
    arrayops::inplace_set(reinterpret_cast<char*>(&mu), char(0), sizeof(superlu::mem_usage_t));

    superlu::SuperLUStat_t stat;
    superlu::init_stat(&stat);

    char equed[8];       // extra characters for paranoia
    T    rpg   = T(0);
    T    rcond = T(0);
    int  info  = int(0); // Return code.

    char  work[8];
    int  lwork = int(0);  // 0 means superlu will allocate memory

    superlu::gssvx<eT>(&options, &a, perm_c, perm_r, etree, equed, R, C, &l, &u, &work[0], lwork, &b, &x, &rpg, &rcond, ferr, berr, &glu, &mu, &stat, &info);

    // Process the return code.
    if( (info > 0) && (info <= int(A.n_cols)) )
      {
      // std::stringstream tmp;
      // tmp << "spsolve(): could not solve system; LU factorisation completed, but detected zero in U(" << (info-1) << ',' << (info-1) << ')';
      // arma_debug_warn(tmp.str());
      }
    else
    if(info == int(A.n_cols+1))
      {
      // arma_debug_warn("spsolve(): system solved, but rcond is less than machine precision");
      }
    else
    if(info > int(A.n_cols+1))
      {
      arma_debug_warn("spsolve(): memory allocation failure: could not allocate ", (info - int(A.n_cols)), " bytes");
      }
    else
    if(info < 0)
      {
      arma_debug_warn("spsolve(): unknown SuperLU error code from gssvx(): ", info);
      }

    superlu::free_stat(&stat);

    superlu::free(berr);
    superlu::free(ferr);
    superlu::free(C);
    superlu::free(R);
    superlu::free(etree);
    superlu::free(perm_r);
    superlu::free(perm_c);

    destroy_supermatrix(u);
    destroy_supermatrix(l);
    destroy_supermatrix(b);
    destroy_supermatrix(a);
    destroy_supermatrix(x);  // No need to extract the data from x, since it's using the same memory as X

    out_rcond = rcond;

    return (info == 0);
    }
  #else
    {
    arma_ignore(X);
    arma_ignore(out_rcond);
    arma_ignore(A_expr);
    arma_ignore(B_expr);
    arma_ignore(user_opts);
    arma_stop_logic_error("spsolve(): use of SuperLU must be enabled");
    return false;
    }
  #endif
  }



#if defined(ARMA_USE_SUPERLU)

  inline
  void
  sp_auxlib::set_superlu_opts(superlu::superlu_options_t& options, const superlu_opts& user_opts)
    {
    arma_extra_debug_sigprint();

    // default options as the starting point
    superlu::set_default_opts(&options);

    // our settings
    options.Trans           = superlu::NOTRANS;
    options.ConditionNumber = superlu::YES;

    // process user_opts

    if(user_opts.equilibrate == true)   { options.Equil = superlu::YES; }
    if(user_opts.equilibrate == false)  { options.Equil = superlu::NO;  }

    if(user_opts.symmetric == true)   { options.SymmetricMode = superlu::YES; }
    if(user_opts.symmetric == false)  { options.SymmetricMode = superlu::NO;  }

    options.DiagPivotThresh = user_opts.pivot_thresh;

    if(user_opts.permutation == superlu_opts::NATURAL)        { options.ColPerm = superlu::NATURAL;       }
    if(user_opts.permutation == superlu_opts::MMD_ATA)        { options.ColPerm = superlu::MMD_ATA;       }
    if(user_opts.permutation == superlu_opts::MMD_AT_PLUS_A)  { options.ColPerm = superlu::MMD_AT_PLUS_A; }
    if(user_opts.permutation == superlu_opts::COLAMD)         { options.ColPerm = superlu::COLAMD;        }

    if(user_opts.refine == superlu_opts::REF_NONE)    { options.IterRefine = superlu::NOREFINE;   }
    if(user_opts.refine == superlu_opts::REF_SINGLE)  { options.IterRefine = superlu::SLU_SINGLE; }
    if(user_opts.refine == superlu_opts::REF_DOUBLE)  { options.IterRefine = superlu::SLU_DOUBLE; }
    if(user_opts.refine == superlu_opts::REF_EXTRA)   { options.IterRefine = superlu::SLU_EXTRA;  }
    }



  template<typename eT>
  inline
  bool
  sp_auxlib::copy_to_supermatrix(superlu::SuperMatrix& out, const SpMat<eT>& A)
    {
    arma_extra_debug_sigprint();

    // We store in column-major CSC.
    out.Stype = superlu::SLU_NC;

    if(is_float<eT>::value)
      {
      out.Dtype = superlu::SLU_S;
      }
    else
    if(is_double<eT>::value)
      {
      out.Dtype = superlu::SLU_D;
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      out.Dtype = superlu::SLU_C;
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      out.Dtype = superlu::SLU_Z;
      }

    out.Mtype = superlu::SLU_GE; // Just a general matrix.  We don't know more now.

    // We have to actually create the object which stores the data.
    // This gets cleaned by destroy_supermatrix().
    // We have to use SuperLU's stupid memory allocation routines since they are
    // not guaranteed to be new and delete.  See the comments in def_superlu.hpp
    superlu::NCformat* nc = (superlu::NCformat*)superlu::malloc(sizeof(superlu::NCformat));

    if(nc == NULL)  { return false; }

    nc->nnz    = A.n_nonzero;
    nc->nzval  = (void*)          superlu::malloc(sizeof(eT)             * A.n_nonzero   );
    nc->colptr = (superlu::int_t*)superlu::malloc(sizeof(superlu::int_t) * (A.n_cols + 1));
    nc->rowind = (superlu::int_t*)superlu::malloc(sizeof(superlu::int_t) * A.n_nonzero   );

    if( (nc->nzval == NULL) || (nc->colptr == NULL) || (nc->rowind == NULL) )  { return false; }

    // Fill the matrix.
    arrayops::copy((eT*) nc->nzval, A.values, A.n_nonzero);

    // // These have to be copied by hand, because the types may differ.
    // for (uword i = 0; i <= A.n_cols; ++i)  { nc->colptr[i] = (int_t) A.col_ptrs[i]; }
    // for (uword i = 0; i < A.n_nonzero; ++i) { nc->rowind[i] = (int_t) A.row_indices[i]; }

    arrayops::convert(nc->colptr, A.col_ptrs,    A.n_cols+1 );
    arrayops::convert(nc->rowind, A.row_indices, A.n_nonzero);

    out.nrow  = A.n_rows;
    out.ncol  = A.n_cols;
    out.Store = (void*) nc;

    return true;
    }



  template<typename eT>
  inline
  bool
  sp_auxlib::wrap_to_supermatrix(superlu::SuperMatrix& out, const Mat<eT>& A)
    {
    arma_extra_debug_sigprint();

    // NOTE: this function re-uses memory from matrix A

    // This is being stored as a dense matrix.
    out.Stype = superlu::SLU_DN;

    if(is_float<eT>::value)
      {
      out.Dtype = superlu::SLU_S;
      }
    else
    if(is_double<eT>::value)
      {
      out.Dtype = superlu::SLU_D;
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      out.Dtype = superlu::SLU_C;
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      out.Dtype = superlu::SLU_Z;
      }

    out.Mtype = superlu::SLU_GE;

    // We have to create the object that stores the data.
    superlu::DNformat* dn = (superlu::DNformat*)superlu::malloc(sizeof(superlu::DNformat));

    if(dn == NULL)  { return false; }

    dn->lda   = A.n_rows;
    dn->nzval = (void*) A.memptr();  // re-use memory instead of copying

    out.nrow  = A.n_rows;
    out.ncol  = A.n_cols;
    out.Store = (void*) dn;

    return true;
    }



  inline
  void
  sp_auxlib::destroy_supermatrix(superlu::SuperMatrix& out)
    {
    arma_extra_debug_sigprint();

    // Clean up.
    if(out.Stype == superlu::SLU_NC)
      {
      superlu::destroy_compcol_mat(&out);
      }
    else
    if(out.Stype == superlu::SLU_DN)
      {
      // superlu::destroy_dense_mat(&out);

      // since dn->nzval is set to re-use memory from a Mat object (which manages its own memory),
      // we cannot simply call superlu::destroy_dense_mat().
      // Only the out.Store structure can be freed.

      superlu::DNformat* dn = (superlu::DNformat*) out.Store;

      if(dn != NULL)  { superlu::free(dn); }
      }
    else
    if(out.Stype == superlu::SLU_SC)
      {
      superlu::destroy_supernode_mat(&out);
      }
    else
      {
      // Uh, crap.

      std::stringstream tmp;

      tmp << "sp_auxlib::destroy_supermatrix(): unhandled Stype" << std::endl;
      tmp << "Stype  val: " << out.Stype << std::endl;
      tmp << "Stype name: ";

      if(out.Stype == superlu::SLU_NC)      { tmp << "SLU_NC";     }
      if(out.Stype == superlu::SLU_NCP)     { tmp << "SLU_NCP";    }
      if(out.Stype == superlu::SLU_NR)      { tmp << "SLU_NR";     }
      if(out.Stype == superlu::SLU_SC)      { tmp << "SLU_SC";     }
      if(out.Stype == superlu::SLU_SCP)     { tmp << "SLU_SCP";    }
      if(out.Stype == superlu::SLU_SR)      { tmp << "SLU_SR";     }
      if(out.Stype == superlu::SLU_DN)      { tmp << "SLU_DN";     }
      if(out.Stype == superlu::SLU_NR_loc)  { tmp << "SLU_NR_loc"; }

      arma_debug_warn(tmp.str());
      arma_stop_runtime_error("sp_auxlib::destroy_supermatrix(): internal error");
      }
    }

#endif



template<typename eT, typename T, typename T1>
inline
void
sp_auxlib::run_aupd
  (
  const uword n_eigvals, char* which, const SpProxy<T1>& p, const bool sym,
  blas_int& n, eT& tol,
  podarray<T>& resid, blas_int& ncv, podarray<T>& v, blas_int& ldv,
  podarray<blas_int>& iparam, podarray<blas_int>& ipntr,
  podarray<T>& workd, podarray<T>& workl, blas_int& lworkl, podarray<eT>& rwork,
  blas_int& info
  )
  {
  #if defined(ARMA_USE_ARPACK)
    {
    // ARPACK provides a "reverse communication interface" which is an
    // entertainingly archaic FORTRAN software engineering technique that
    // basically means that we call saupd()/naupd() and it tells us with some
    // return code what we need to do next (usually a matrix-vector product) and
    // then call it again.  So this results in some type of iterative process
    // where we call saupd()/naupd() many times.
    blas_int ido = 0; // This must be 0 for the first call.
    char bmat = 'I'; // We are considering the standard eigenvalue problem.
    n = p.get_n_rows(); // The size of the matrix.
    blas_int nev = n_eigvals;

    resid.set_size(n);

    // Two contraints on NCV: (NCV > NEV + 2) and (NCV <= N)
    //
    // We're calling either arpack::saupd() or arpack::naupd(),
    // which have slighly different minimum constraint and recommended value for NCV:
    // http://www.caam.rice.edu/software/ARPACK/UG/node136.html
    // http://www.caam.rice.edu/software/ARPACK/UG/node138.html

    ncv = nev + 2 + 1;

    if (ncv < (2 * nev + 1)) { ncv = 2 * nev + 1; }
    if (ncv > n            ) { ncv = n;           }

    v.set_size(n * ncv); // Array N by NCV (output).
    rwork.set_size(ncv); // Work array of size NCV for complex calls.
    ldv = n; // "Leading dimension of V exactly as declared in the calling program."

    // IPARAM: integer array of length 11.
    iparam.zeros(11);
    iparam(0) = 1; // Exact shifts (not provided by us).
    iparam(2) = 1000; // Maximum iterations; all the examples use 300, but they were written in the ancient times.
    iparam(6) = 1; // Mode 1: A * x = lambda * x.

    // IPNTR: integer array of length 14 (output).
    ipntr.set_size(14);

    // Real work array used in the basic Arnoldi iteration for reverse communication.
    workd.set_size(3 * n);

    // lworkl must be at least 3 * NCV^2 + 6 * NCV.
    lworkl = 3 * (ncv * ncv) + 6 * ncv;

    // Real work array of length lworkl.
    workl.set_size(lworkl);

    info = 0; // Set to 0 initially to use random initial vector.

    // All the parameters have been set or created.  Time to loop a lot.
    while (ido != 99)
      {
      // Call saupd() or naupd() with the current parameters.
      if(sym)
        arpack::saupd(&ido, &bmat, &n, which, &nev, &tol, resid.memptr(), &ncv, v.memptr(), &ldv, iparam.memptr(), ipntr.memptr(), workd.memptr(), workl.memptr(), &lworkl, &info);
      else
        arpack::naupd(&ido, &bmat, &n, which, &nev, &tol, resid.memptr(), &ncv, v.memptr(), &ldv, iparam.memptr(), ipntr.memptr(), workd.memptr(), workl.memptr(), &lworkl, rwork.memptr(), &info);

      // What do we do now?
      switch (ido)
        {
        case -1:
        case 1:
          {
          // We need to calculate the matrix-vector multiplication y = OP * x
          // where x is of length n and starts at workd(ipntr(0)), and y is of
          // length n and starts at workd(ipntr(1)).

          // operator*(sp_mat, vec) doesn't properly put the result into the
          // right place so we'll just reimplement it here for now...

          // Set the output to point at the right memory.  We have to subtract
          // one from FORTRAN pointers...
          Col<T> out(workd.memptr() + ipntr(1) - 1, n, false /* don't copy */);
          // Set the input to point at the right memory.
          Col<T> in(workd.memptr() + ipntr(0) - 1, n, false /* don't copy */);

          out.zeros();
          typename SpProxy<T1>::const_iterator_type x_it     = p.begin();
          typename SpProxy<T1>::const_iterator_type x_it_end = p.end();

          while(x_it != x_it_end)
            {
            out[x_it.row()] += (*x_it) * in[x_it.col()];
            ++x_it;
            }

          // No need to modify memory further since it was all done in-place.

          break;
          }
        case 99:
          // Nothing to do here, things have converged.
          break;
        default:
          {
          return; // Parent frame can look at the value of info.
          }
        }
      }

    // The process has ended; check the return code.
    if( (info != 0) && (info != 1) )
      {
      // Print warnings if there was a failure.

      if(sym)
        {
        arma_debug_warn("eigs_sym(): ARPACK error ", info, " in saupd()");
        }
      else
        {
        arma_debug_warn("eigs_gen(): ARPACK error ", info, " in naupd()");
        }

      return; // Parent frame can look at the value of info.
      }
    }
  #else
    arma_ignore(n_eigvals);
    arma_ignore(which);
    arma_ignore(p);
    arma_ignore(sym);
    arma_ignore(n);
    arma_ignore(tol);
    arma_ignore(resid);
    arma_ignore(ncv);
    arma_ignore(v);
    arma_ignore(ldv);
    arma_ignore(iparam);
    arma_ignore(ipntr);
    arma_ignore(workd);
    arma_ignore(workl);
    arma_ignore(lworkl);
    arma_ignore(rwork);
    arma_ignore(info);
  #endif
  }
