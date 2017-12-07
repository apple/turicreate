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


//! This class implements the eigen solver for general real matrices.
template<typename eT, int SelectionRule, typename OpType>
class GenEigsSolver
  {
  protected:

  const OpType&           op;        // object to conduct matrix operation, e.g. matrix-vector product
  const uword             nev;       // number of eigenvalues requested
  Col< std::complex<eT> > ritz_val;  // ritz values

  // Sort the first nev Ritz pairs in decreasing magnitude order
  // This is used to return the final results
  virtual void sort_ritzpair();


  private:

  const uword             dim_n;     // dimension of matrix A
  const uword             ncv;       // number of ritz values
  uword                   nmatop;    // number of matrix operations called
  uword                   niter;     // number of restarting iterations
  Mat<eT>                 fac_V;     // V matrix in the Arnoldi factorisation
  Mat<eT>                 fac_H;     // H matrix in the Arnoldi factorisation
  Col<eT>                 fac_f;     // residual in the Arnoldi factorisation
  Mat< std::complex<eT> > ritz_vec;  // ritz vectors
  Col< std::complex<eT> > ritz_est;  // last row of ritz_vec
  std::vector<bool>       ritz_conv; // indicator of the convergence of ritz values
  const eT                eps;       // the machine precision
                                     // e.g. ~= 1e-16 for double type
  const eT                approx0;   // a number that is approximately zero
                                     // approx0 = eps^(2/3)
                                     // used to test the orthogonality of vectors,
                                     // and in convergence test, tol*approx0 is
                                     // the absolute tolerance

  // Arnoldi factorisation starting from step-k
  inline void factorise_from(uword from_k, uword to_m, const Col<eT>& fk);

  // Implicitly restarted Arnoldi factorisation
  inline void restart(uword k);

  // Calculate the number of converged Ritz values
  inline uword num_converged(eT tol);

  // Return the adjusted nev for restarting
  inline uword nev_adjusted(uword nconv);

  // Retrieve and sort ritz values and ritz vectors
  inline void retrieve_ritzpair();


  public:

  //! Constructor to create a solver object.
  inline GenEigsSolver(const OpType& op_, uword nev_, uword ncv_);

  //! Providing the initial residual vector for the algorithm.
  inline void init(eT* init_resid);

  //! Providing a random initial residual vector.
  inline void init();

  //! Conducting the major computation procedure.
  inline uword compute(uword maxit = 1000, eT tol = 1e-10);

  //! Returning the number of iterations used in the computation.
  inline int num_iterations() { return niter; }

  //! Returning the number of matrix operations used in the computation.
  inline int num_operations() { return nmatop; }

  //! Returning the converged eigenvalues.
  inline Col< std::complex<eT> > eigenvalues();

  //! Returning the eigenvectors associated with the converged eigenvalues.
  inline Mat< std::complex<eT> > eigenvectors(uword nvec);

  //! Returning all converged eigenvectors.
  inline Mat< std::complex<eT> > eigenvectors() { return eigenvectors(nev); }
  };


}  // namespace newarp
