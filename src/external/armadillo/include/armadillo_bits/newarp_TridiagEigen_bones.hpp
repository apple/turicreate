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


//! Calculate the eigenvalues and eigenvectors of a symmetric tridiagonal matrix.
//! This class is a wrapper of the Lapack functions `_steqr`.
template<typename eT>
class TridiagEigen
  {
  private:

  blas_int n;
  Col<eT>  main_diag;  // Main diagonal elements of the matrix
  Col<eT>  sub_diag;   // Sub-diagonal elements of the matrix
  Mat<eT>  evecs;      // To store eigenvectors
  bool     computed;


  public:

  //! Default constructor. Computation can
  //! be performed later by calling the compute() method.
  inline TridiagEigen();

  //! Constructor to create an object that calculates the eigenvalues
  //! and eigenvectors of a symmetric tridiagonal matrix `mat_obj`.
  inline TridiagEigen(const Mat<eT>& mat_obj);

  //! Compute the eigenvalue decomposition of a symmetric tridiagonal matrix.
  inline void compute(const Mat<eT>& mat_obj);

  //! Retrieve the eigenvalues.
  inline Col<eT> eigenvalues();

  //! Retrieve the eigenvectors.
  inline Mat<eT> eigenvectors();
  };


}  // namespace newarp
