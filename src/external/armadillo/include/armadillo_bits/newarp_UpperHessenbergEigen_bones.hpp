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


//! Calculate the eigenvalues and eigenvectors of an upper Hessenberg matrix.
//! This class is a wrapper of the Lapack functions `_lahqr` and `_trevc`.
template<typename eT>
class UpperHessenbergEigen
  {
  private:

  blas_int                n;
  Mat<eT>                 mat_Z;  // In the first stage, H = ZTZ', Z is an orthogonal matrix
                                  // In the second stage, Z will be overwritten by the eigenvectors of H
  Mat<eT>                 mat_T;  // H = ZTZ', T is a Schur form matrix
  Col< std::complex<eT> > evals;  // eigenvalues of H
  bool                    computed;


  public:

  //! Default constructor. Computation can
  //! be performed later by calling the compute() method.
  inline UpperHessenbergEigen();

  //! Constructor to create an object that calculates the eigenvalues
  //! and eigenvectors of an upper Hessenberg matrix `mat_obj`.
  inline UpperHessenbergEigen(const Mat<eT>& mat_obj);

  //! Compute the eigenvalue decomposition of an upper Hessenberg matrix.
  inline void compute(const Mat<eT>& mat_obj);

  //! Retrieve the eigenvalues.
  inline Col< std::complex<eT> > eigenvalues();

  //! Retrieve the eigenvectors.
  inline Mat< std::complex<eT> > eigenvectors();
  };


}  // namespace newarp
