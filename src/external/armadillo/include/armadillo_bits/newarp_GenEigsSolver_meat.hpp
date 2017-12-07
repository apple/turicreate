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


namespace newarp
{


template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::factorise_from(uword from_k, uword to_m, const Col<eT>& fk)
  {
  arma_extra_debug_sigprint();

  if(to_m <= from_k) { return; }

  fac_f = fk;

  Col<eT> w(dim_n);
  eT beta = norm(fac_f);
  // Keep the upperleft k x k submatrix of H and set other elements to 0
  fac_H.tail_cols(ncv - from_k).zeros();
  fac_H.submat(span(from_k, ncv - 1), span(0, from_k - 1)).zeros();
  for(uword i = from_k; i <= to_m - 1; i++)
    {
    bool restart = false;
    // If beta = 0, then the next V is not full rank
    // We need to generate a new residual vector that is orthogonal
    // to the current V, which we call a restart
    if(beta < eps)
      {
      // Generate new random vector for fac_f
      blas_int idist = 2;
      blas_int iseed[4] = {1, 3, 5, 7};
      iseed[0] = (i + 100) % 4095;
      blas_int n = dim_n;
      lapack::larnv(&idist, iseed, &n, fac_f.memptr());
      // f <- f - V * V' * f, so that f is orthogonal to V
      Mat<eT> Vs(fac_V.memptr(), dim_n, i, false); // First i columns
      Col<eT> Vf = Vs.t() * fac_f;
      fac_f -= Vs * Vf;
      // beta <- ||f||
      beta = norm(fac_f);

      restart = true;
      }

    // v <- f / ||f||
    fac_V.col(i) = fac_f / beta; // The (i+1)-th column

    // Note that H[i+1, i] equals to the unrestarted beta
    if(restart) { fac_H(i, i - 1) = 0.0; } else { fac_H(i, i - 1) = beta; }

    // w <- A * v, v = fac_V.col(i)
    op.perform_op(fac_V.colptr(i), w.memptr());
    nmatop++;

    // First i+1 columns of V
    Mat<eT> Vs(fac_V.memptr(), dim_n, i + 1, false);
    // h = fac_H(0:i, i)
    Col<eT> h(fac_H.colptr(i), i + 1, false);
    // h <- V' * w
    h = Vs.t() * w;

    // f <- w - V * h
    fac_f = w - Vs * h;
    beta = norm(fac_f);

    if(beta > 0.717 * norm(h)) { continue; }

    // f/||f|| is going to be the next column of V, so we need to test
    // whether V' * (f/||f||) ~= 0
    Col<eT> Vf = Vs.t() * fac_f;
    // If not, iteratively correct the residual
    uword count = 0;
    while(count < 5 && abs(Vf).max() > approx0 * beta)
      {
      // f <- f - V * Vf
      fac_f -= Vs * Vf;
      // h <- h + Vf
      h += Vf;
      // beta <- ||f||
      beta = norm(fac_f);

      Vf = Vs.t() * fac_f;
      count++;
      }
    }
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::restart(uword k)
  {
  arma_extra_debug_sigprint();

  if(k >= ncv) { return; }

  DoubleShiftQR<eT>     decomp_ds(ncv);
  UpperHessenbergQR<eT> decomp;

  Mat<eT> Q(ncv, ncv, fill::eye);

  for(uword i = k; i < ncv; i++)
    {
    if(cx_attrib::is_complex(ritz_val(i), eT(0)) && (i < (ncv - 1)) && cx_attrib::is_conj(ritz_val(i), ritz_val(i + 1), eT(0)))
      {
      // H - mu * I = Q1 * R1
      // H <- R1 * Q1 + mu * I = Q1' * H * Q1
      // H - conj(mu) * I = Q2 * R2
      // H <- R2 * Q2 + conj(mu) * I = Q2' * H * Q2
      //
      // (H - mu * I) * (H - conj(mu) * I) = Q1 * Q2 * R2 * R1 = Q * R
      eT s = 2 * ritz_val(i).real();
      eT t = std::norm(ritz_val(i));
      decomp_ds.compute(fac_H, s, t);

      // Q -> Q * Qi
      decomp_ds.apply_YQ(Q);
      // H -> Q'HQ
      fac_H = decomp_ds.matrix_QtHQ();

      i++;
      }
    else
      {
      // QR decomposition of H - mu * I, mu is real
      fac_H.diag() -= ritz_val(i).real();
      decomp.compute(fac_H);

      // Q -> Q * Qi
      decomp.apply_YQ(Q);
      // H -> Q'HQ = RQ + mu * I
      fac_H = decomp.matrix_RQ();
      fac_H.diag() += ritz_val(i).real();
      }
    }
  // V -> VQ
  // Q has some elements being zero
  // The first (ncv - k + i) elements of the i-th column of Q are non-zero
  Mat<eT> Vs(dim_n, k + 1);
  uword nnz;
  for(uword i = 0; i < k; i++)
    {
    nnz = ncv - k + i + 1;
    Mat<eT> V(fac_V.memptr(), dim_n, nnz, false);
    Col<eT> q(Q.colptr(i), nnz, false);
    Col<eT> v(Vs.colptr(i), dim_n, false);
    v = V * q;
    }

  Vs.col(k) = fac_V * Q.col(k);
  fac_V.head_cols(k + 1) = Vs;

  Col<eT> fk = fac_f * Q(ncv - 1, k - 1) + fac_V.col(k) * fac_H(k, k - 1);
  factorise_from(k, ncv, fk);
  retrieve_ritzpair();
  }



template<typename eT, int SelectionRule, typename OpType>
inline
uword
GenEigsSolver<eT, SelectionRule, OpType>::num_converged(eT tol)
  {
  arma_extra_debug_sigprint();

  // thresh = tol * max(prec, abs(theta)), theta for ritz value
  const eT f_norm = arma::norm(fac_f);
  for(uword i = 0; i < nev; i++)
    {
    eT thresh = tol * std::max(approx0, std::abs(ritz_val(i)));
    eT resid = std::abs(ritz_est(i)) * f_norm;
    ritz_conv[i] = (resid < thresh);
    }

  return std::count(ritz_conv.begin(), ritz_conv.end(), true);
  }



template<typename eT, int SelectionRule, typename OpType>
inline
uword
GenEigsSolver<eT, SelectionRule, OpType>::nev_adjusted(uword nconv)
  {
  arma_extra_debug_sigprint();

  uword nev_new = nev;

  for(uword i = nev; i < ncv; i++)
    {
    if(std::abs(ritz_est(i)) < eps) { nev_new++; }
    }
  // Adjust nev_new again, according to dnaup2.f line 660~674 in ARPACK
  nev_new += std::min(nconv, (ncv - nev_new) / 2);
  if(nev_new == 1 && ncv >= 6)
    {
    nev_new = ncv / 2;
    }
  else
  if(nev_new == 1 && ncv > 3)
    {
    nev_new = 2;
    }

  if(nev_new > ncv - 2) { nev_new = ncv - 2; }

  // Increase nev by one if ritz_val[nev - 1] and
  // ritz_val[nev] are conjugate pairs
  if(cx_attrib::is_complex(ritz_val(nev_new - 1), eps) && cx_attrib::is_conj(ritz_val(nev_new - 1), ritz_val(nev_new), eps))
    {
    nev_new++;
    }

  return nev_new;
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::retrieve_ritzpair()
  {
  arma_extra_debug_sigprint();

  UpperHessenbergEigen<eT> decomp(fac_H);

  Col< std::complex<eT> > evals = decomp.eigenvalues();
  Mat< std::complex<eT> > evecs = decomp.eigenvectors();

  SortEigenvalue< std::complex<eT>, SelectionRule > sorting(evals.memptr(), evals.n_elem);
  std::vector<uword> ind = sorting.index();

  // Copy the ritz values and vectors to ritz_val and ritz_vec, respectively
  for(uword i = 0; i < ncv; i++)
    {
    ritz_val(i) = evals(ind[i]);
    ritz_est(i) = evecs(ncv - 1, ind[i]);
    }
  for(uword i = 0; i < nev; i++)
    {
    ritz_vec.col(i) = evecs.col(ind[i]);
    }
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::sort_ritzpair()
  {
  arma_extra_debug_sigprint();

  // SortEigenvalue< std::complex<eT>, EigsSelect::LARGEST_MAGN > sorting(ritz_val.memptr(), nev);

  // sort Ritz values according to SelectionRule, to be consistent with ARPACK
  SortEigenvalue< std::complex<eT>, SelectionRule > sorting(ritz_val.memptr(), nev);

  std::vector<uword> ind = sorting.index();

  Col< std::complex<eT> > new_ritz_val(ncv);
  Mat< std::complex<eT> > new_ritz_vec(ncv, nev);
  std::vector<bool>       new_ritz_conv(nev);

  for(uword i = 0; i < nev; i++)
    {
    new_ritz_val(i) = ritz_val(ind[i]);
    new_ritz_vec.col(i) = ritz_vec.col(ind[i]);
    new_ritz_conv[i] = ritz_conv[ind[i]];
    }

  ritz_val.swap(new_ritz_val);
  ritz_vec.swap(new_ritz_vec);
  ritz_conv.swap(new_ritz_conv);
  }



template<typename eT, int SelectionRule, typename OpType>
inline
GenEigsSolver<eT, SelectionRule, OpType>::GenEigsSolver(const OpType& op_, uword nev_, uword ncv_)
  : op(op_)
  , nev(nev_)
  , dim_n(op.n_rows)
  , ncv(ncv_ > dim_n ? dim_n : ncv_)
  , nmatop(0)
  , niter(0)
  , eps(std::numeric_limits<eT>::epsilon())
  , approx0(std::pow(eps, eT(2.0) / 3))
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (nev_ < 1 || nev_ > dim_n - 2),    "newarp::GenEigsSolver: nev must satisfy 1 <= nev <= n - 2, n is the size of matrix" );
  arma_debug_check( (ncv_ < nev_ + 2 || ncv_ > dim_n), "newarp::GenEigsSolver: ncv must satisfy nev + 2 <= ncv <= n, n is the size of matrix" );
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::init(eT* init_resid)
  {
  arma_extra_debug_sigprint();

  // Reset all matrices/vectors to zero
  fac_V.zeros(dim_n, ncv);
  fac_H.zeros(ncv, ncv);
  fac_f.zeros(dim_n);
  ritz_val.zeros(ncv);
  ritz_vec.zeros(ncv, nev);
  ritz_est.zeros(ncv);
  ritz_conv.assign(nev, false);

  nmatop = 0;
  niter = 0;

  Col<eT> r(init_resid, dim_n, false);
  // The first column of fac_V
  Col<eT> v(fac_V.colptr(0), dim_n, false);
  eT rnorm = norm(r);
  arma_check( (rnorm < eps), "newarp::GenEigsSolver::init(): initial residual vector cannot be zero" );
  v = r / rnorm;

  Col<eT> w(dim_n);
  op.perform_op(v.memptr(), w.memptr());
  nmatop++;

  fac_H(0, 0) = dot(v, w);
  fac_f = w - v * fac_H(0, 0);
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
GenEigsSolver<eT, SelectionRule, OpType>::init()
  {
  arma_extra_debug_sigprint();

  podarray<eT> init_resid(dim_n);
  blas_int idist = 2;                // Uniform(-1, 1)
  blas_int iseed[4] = {1, 3, 5, 7};  // Fixed random seed
  blas_int n = dim_n;
  lapack::larnv(&idist, iseed, &n, init_resid.memptr());
  init(init_resid.memptr());
  }



template<typename eT, int SelectionRule, typename OpType>
inline
uword
GenEigsSolver<eT, SelectionRule, OpType>::compute(uword maxit, eT tol)
  {
  arma_extra_debug_sigprint();

  // The m-step Arnoldi factorisation
  factorise_from(1, ncv, fac_f);
  retrieve_ritzpair();
  // Restarting
  uword i, nconv = 0, nev_adj;
  for(i = 0; i < maxit; i++)
    {
    nconv = num_converged(tol);
    if(nconv >= nev) { break; }

    nev_adj = nev_adjusted(nconv);
    restart(nev_adj);
    }
  // Sorting results
  sort_ritzpair();

  niter = i + 1;

  return std::min(nev, nconv);
  }



template<typename eT, int SelectionRule, typename OpType>
inline
Col< std::complex<eT> >
GenEigsSolver<eT, SelectionRule, OpType>::eigenvalues()
  {
  arma_extra_debug_sigprint();

  uword nconv = std::count(ritz_conv.begin(), ritz_conv.end(), true);
  Col< std::complex<eT> > res(nconv);

  if(nconv > 0)
    {
    uword j = 0;
    for(uword i = 0; i < nev; i++)
      {
      if(ritz_conv[i])
        {
        res(j) = ritz_val(i);
        j++;
        }
      }
    }

  return res;
  }



template<typename eT, int SelectionRule, typename OpType>
inline
Mat< std::complex<eT> >
GenEigsSolver<eT, SelectionRule, OpType>::eigenvectors(uword nvec)
  {
  arma_extra_debug_sigprint();

  uword nconv = std::count(ritz_conv.begin(), ritz_conv.end(), true);
  nvec = std::min(nvec, nconv);
  Mat< std::complex<eT> > res(dim_n, nvec);

  if(nvec > 0)
    {
    Mat< std::complex<eT> > ritz_vec_conv(ncv, nvec);
    uword j = 0;
    for(uword i = 0; (i < nev) && (j < nvec); i++)
      {
      if(ritz_conv[i])
        {
        ritz_vec_conv.col(j) = ritz_vec.col(i);
        j++;
        }
      }

    res = fac_V * ritz_vec_conv;
    }

  return res;
  }


}  // namespace newarp
