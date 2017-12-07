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
SymEigsSolver<eT, SelectionRule, OpType>::factorise_from(uword from_k, uword to_m, const Col<eT>& fk)
  {
  arma_extra_debug_sigprint();

  if(to_m <= from_k) { return; }

  fac_f = fk;

  Col<eT> w(dim_n);
  eT beta = norm(fac_f), Hii = 0.0;
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
    Col<eT> v(fac_V.colptr(i), dim_n, false); // The (i+1)-th column
    v = fac_f / beta;

    // Note that H[i+1, i] equals to the unrestarted beta
    if(restart) { fac_H(i, i - 1) = 0.0; } else { fac_H(i, i - 1) = beta; }

    // w <- A * v, v = fac_V.col(i)
    op.perform_op(v.memptr(), w.memptr());
    nmatop++;

    Hii = dot(v, w);
    fac_H(i - 1, i) = fac_H(i, i - 1); // Due to symmetry
    fac_H(i, i) = Hii;

    // f <- w - V * V' * w = w - H[i+1, i] * V{i} - H[i+1, i+1] * V{i+1}
    // If restarting, we know that H[i+1, i] = 0
    if(restart)
      {
      fac_f = w - Hii * v;
      }
    else
      {
      fac_f = w - fac_H(i, i - 1) * fac_V.col(i - 1) - Hii * v;
      }

    beta = norm(fac_f);

    // f/||f|| is going to be the next column of V, so we need to test
    // whether V' * (f/||f||) ~= 0
    Mat<eT> Vs(fac_V.memptr(), dim_n, i + 1, false); // First i+1 columns
    Col<eT> Vf = Vs.t() * fac_f;
    // If not, iteratively correct the residual
    uword count = 0;
    while(count < 5 && abs(Vf).max() > approx0 * beta)
      {
      // f <- f - V * Vf
      fac_f -= Vs * Vf;
      // h <- h + Vf
      fac_H(i - 1, i) += Vf[i - 1];
      fac_H(i, i - 1) = fac_H(i - 1, i);
      fac_H(i, i) += Vf[i];
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
SymEigsSolver<eT, SelectionRule, OpType>::restart(uword k)
  {
  arma_extra_debug_sigprint();

  if(k >= ncv) { return; }

  TridiagQR<eT> decomp;
  Mat<eT> Q = eye< Mat<eT> >(ncv, ncv);

  for(uword i = k; i < ncv; i++)
    {
    // QR decomposition of H-mu*I, mu is the shift
    fac_H.diag() -= ritz_val(i);
    decomp.compute(fac_H);

    // Q -> Q * Qi
    decomp.apply_YQ(Q);

    // H -> Q'HQ
    // Since QR = H - mu * I, we have H = QR + mu * I
    // and therefore Q'HQ = RQ + mu * I
    fac_H = decomp.matrix_RQ();
    fac_H.diag() += ritz_val(i);
    }

  // V -> VQ, only need to update the first k+1 columns
  // Q has some elements being zero
  // The first (ncv - k + i) elements of the i-th column of Q are non-zero
  Mat<eT> Vs(dim_n, k + 1);
  uword nnz;
  for(uword i = 0; i < k; i++)
    {
    nnz = ncv - k + i + 1;
    Mat<eT> V(fac_V.memptr(), dim_n, nnz, false);
    Col<eT> q(Q.colptr(i), nnz, false);
    Vs.col(i) = V * q;
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
SymEigsSolver<eT, SelectionRule, OpType>::num_converged(eT tol)
  {
  arma_extra_debug_sigprint();

  // thresh = tol * max(approx0, abs(theta)), theta for ritz value
  const eT f_norm = norm(fac_f);
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
SymEigsSolver<eT, SelectionRule, OpType>::nev_adjusted(uword nconv)
  {
  arma_extra_debug_sigprint();

  uword nev_new = nev;
  for(uword i = nev; i < ncv; i++)
    {
    if(std::abs(ritz_est(i)) < eps) { nev_new++; }
    }

  // Adjust nev_new, according to dsaup2.f line 677~684 in ARPACK
  nev_new += std::min(nconv, (ncv - nev_new) / 2);
  if(nev_new >= ncv) { nev_new = ncv - 1; }
  if(nev_new == 1 && ncv >= 6)
    {
    nev_new = ncv / 2;
    }
  else
  if(nev_new == 1 && ncv > 2)
    {
    nev_new = 2;
    }

  return nev_new;
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
SymEigsSolver<eT, SelectionRule, OpType>::retrieve_ritzpair()
  {
  arma_extra_debug_sigprint();

  TridiagEigen<eT> decomp(fac_H);
  Col<eT> evals = decomp.eigenvalues();
  Mat<eT> evecs = decomp.eigenvectors();

  SortEigenvalue<eT, SelectionRule> sorting(evals.memptr(), evals.n_elem);
  std::vector<uword> ind = sorting.index();

  // For BOTH_ENDS, the eigenvalues are sorted according
  // to the LARGEST_ALGE rule, so we need to move those smallest
  // values to the left
  // The order would be
  // Largest => Smallest => 2nd largest => 2nd smallest => ...
  // We keep this order since the first k values will always be
  // the wanted collection, no matter k is nev_updated (used in restart())
  // or is nev (used in sort_ritzpair())
  if(SelectionRule == EigsSelect::BOTH_ENDS)
    {
    std::vector<uword> ind_copy(ind);
    for(uword i = 0; i < ncv; i++)
      {
      // If i is even, pick values from the left (large values)
      // If i is odd, pick values from the right (small values)
      if(i % 2 == 0) { ind[i] = ind_copy[i / 2]; } else { ind[i] = ind_copy[ncv - 1 - i / 2]; }
      }
    }

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
SymEigsSolver<eT, SelectionRule, OpType>::sort_ritzpair()
  {
  arma_extra_debug_sigprint();

  // SortEigenvalue<eT, EigsSelect::LARGEST_MAGN> sorting(ritz_val.memptr(), nev);

  // sort Ritz values in ascending algebraic, to be consistent with ARPACK
  SortEigenvalue<eT, EigsSelect::SMALLEST_ALGE> sorting(ritz_val.memptr(), nev);

  std::vector<uword> ind = sorting.index();

  Col<eT>           new_ritz_val(ncv);
  Mat<eT>           new_ritz_vec(ncv, nev);
  std::vector<bool> new_ritz_conv(nev);

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
SymEigsSolver<eT, SelectionRule, OpType>::SymEigsSolver(const OpType& op_, uword nev_, uword ncv_)
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

  arma_debug_check( (nev_ < 1 || nev_ > dim_n - 1), "newarp::SymEigsSolver: nev must satisfy 1 <= nev <= n - 1, n is the size of matrix" );
  arma_debug_check( (ncv_ <= nev_ || ncv_ > dim_n), "newarp::SymEigsSolver: ncv must satisfy nev < ncv <= n, n is the size of matrix" );
  }



template<typename eT, int SelectionRule, typename OpType>
inline
void
SymEigsSolver<eT, SelectionRule, OpType>::init(eT* init_resid)
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
  arma_check( (rnorm < eps), "newarp::SymEigsSolver::init(): initial residual vector cannot be zero" );
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
SymEigsSolver<eT, SelectionRule, OpType>::init()
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
SymEigsSolver<eT, SelectionRule, OpType>::compute(uword maxit, eT tol)
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
Col<eT>
SymEigsSolver<eT, SelectionRule, OpType>::eigenvalues()
  {
  arma_extra_debug_sigprint();

  uword nconv = std::count(ritz_conv.begin(), ritz_conv.end(), true);
  Col<eT> res(nconv);

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
Mat<eT>
SymEigsSolver<eT, SelectionRule, OpType>::eigenvectors(uword nvec)
  {
  arma_extra_debug_sigprint();

  uword nconv = std::count(ritz_conv.begin(), ritz_conv.end(), true);
  nvec = std::min(nvec, nconv);
  Mat<eT> res(dim_n, nvec);

  if(nvec > 0)
    {
    Mat<eT> ritz_vec_conv(ncv, nvec);

    uword j = 0;
    for(uword i = 0; i < nev && j < nvec; i++)
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
