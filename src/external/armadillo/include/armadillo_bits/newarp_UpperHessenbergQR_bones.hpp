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


//! Perform the QR decomposition of an upper Hessenberg matrix.
template<typename eT>
class UpperHessenbergQR
  {
  protected:

  uword   n;
  Mat<eT> mat_T;
  // Gi = [ cos[i]  sin[i]]
  //      [-sin[i]  cos[i]]
  // Q = G1 * G2 * ... * G_{n-1}
  Col<eT> rot_cos;
  Col<eT> rot_sin;
  bool    computed;


  public:

  //! Default constructor. Computation can
  //! be performed later by calling the compute() method.
  inline UpperHessenbergQR();

  //! Constructor to create an object that performs and stores the
  //! QR decomposition of an upper Hessenberg matrix `mat_obj`.
  inline UpperHessenbergQR(const Mat<eT>& mat_obj);

  //! Conduct the QR factorisation of an upper Hessenberg matrix.
  virtual void compute(const Mat<eT>& mat_obj);

  //! Return the \f$RQ\f$ matrix, the multiplication of \f$R\f$ and \f$Q\f$,
  //! which is an upper Hessenberg matrix.
  virtual Mat<eT> matrix_RQ();

  //! Apply the \f$Q\f$ matrix to another matrix \f$Y\f$.
  inline void apply_YQ(Mat<eT>& Y);
  };



//! Perform the QR decomposition of a tridiagonal matrix, a special
//! case of upper Hessenberg matrices.
template<typename eT>
class TridiagQR : public UpperHessenbergQR<eT>
  {
  public:

  //! Default constructor. Computation can
  //! be performed later by calling the compute() method.
  inline TridiagQR();

  //! Constructor to create an object that performs and stores the
  //! QR decomposition of a tridiagonal matrix `mat_obj`.
  inline TridiagQR(const Mat<eT>& mat_obj);

  //! Conduct the QR factorisation of a tridiagonal matrix.
  inline void compute(const Mat<eT>& mat_obj);

  //! Return the \f$RQ\f$ matrix, the multiplication of \f$R\f$ and \f$Q\f$,
  //! which is a tridiagonal matrix.
  inline Mat<eT> matrix_RQ();
  };


}  // namespace newarp
