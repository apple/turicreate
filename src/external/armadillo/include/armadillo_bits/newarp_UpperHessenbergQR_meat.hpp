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


template<typename eT>
inline
UpperHessenbergQR<eT>::UpperHessenbergQR()
  : n(0)
  , computed(false)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
UpperHessenbergQR<eT>::UpperHessenbergQR(const Mat<eT>& mat_obj)
  : n(mat_obj.n_rows)
  , mat_T(n, n)
  , rot_cos(n - 1)
  , rot_sin(n - 1)
  , computed(false)
  {
  arma_extra_debug_sigprint();

  compute(mat_obj);
  }



template<typename eT>
void
UpperHessenbergQR<eT>::compute(const Mat<eT>& mat_obj)
  {
  arma_extra_debug_sigprint();

  n = mat_obj.n_rows;
  mat_T.set_size(n, n);
  rot_cos.set_size(n - 1);
  rot_sin.set_size(n - 1);

  // Make a copy of mat_obj
  mat_T = mat_obj;

  eT xi, xj, r, c, s, eps = std::numeric_limits<eT>::epsilon();
  eT *ptr;
  for(uword i = 0; i < n - 1; i++)
    {
    // Make sure mat_T is upper Hessenberg
    // Zero the elements below mat_T(i + 1, i)
    if(i < n - 2) { mat_T(span(i + 2, n - 1), i).zeros(); }

    xi = mat_T(i,     i);  // mat_T(i, i)
    xj = mat_T(i + 1, i);  // mat_T(i + 1, i)
    r = arma_hypot(xi, xj);
    if(r <= eps)
      {
      r = 0;
      rot_cos(i) = c = 1;
      rot_sin(i) = s = 0;
      }
    else
      {
      rot_cos(i) = c = xi / r;
      rot_sin(i) = s = -xj / r;
      }

    // For a complete QR decomposition,
    // we first obtain the rotation matrix
    // G = [ cos  sin]
    //     [-sin  cos]
    // and then do T[i:(i + 1), i:(n - 1)] = G' * T[i:(i + 1), i:(n - 1)]

    // mat_T.submat(i, i, i + 1, n - 1) = Gt * mat_T.submat(i, i, i + 1, n - 1);
    mat_T(i,     i) = r;    // mat_T(i, i)     => r
    mat_T(i + 1, i) = 0;    // mat_T(i + 1, i) => 0
    ptr = &mat_T(i, i + 1); // mat_T(i, k), k = i+1, i+2, ..., n-1
    for(uword j = i + 1; j < n; j++, ptr += n)
      {
      eT tmp = ptr[0];
      ptr[0] = c * tmp - s * ptr[1];
      ptr[1] = s * tmp + c * ptr[1];
      }
    }

  computed = true;
  }



template<typename eT>
Mat<eT>
UpperHessenbergQR<eT>::matrix_RQ()
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (computed == false), "newarp::UpperHessenbergQR::matrix_RQ(): need to call compute() first" );

  // Make a copy of the R matrix
  Mat<eT> RQ = trimatu(mat_T);

  for(uword i = 0; i < n - 1; i++)
    {
    // RQ[, i:(i + 1)] = RQ[, i:(i + 1)] * Gi
    // Gi = [ cos[i]  sin[i]]
    //      [-sin[i]  cos[i]]
    const eT c = rot_cos(i);
    const eT s = rot_sin(i);
    eT *Yi, *Yi1;
    Yi  = RQ.colptr(i);
    Yi1 = RQ.colptr(i + 1);
    for(uword j = 0; j < i + 2; j++)
      {
      eT tmp = Yi[j];
      Yi[j]  = c * tmp - s * Yi1[j];
      Yi1[j] = s * tmp + c * Yi1[j];
      }

    /* Yi = RQ(span(0, i + 1), i);
    RQ(span(0, i + 1), i)     = (*c) * Yi - (*s) * RQ(span(0, i + 1), i + 1);
    RQ(span(0, i + 1), i + 1) = (*s) * Yi + (*c) * RQ(span(0, i + 1), i + 1); */
    }

  return RQ;
  }



template<typename eT>
inline
void
UpperHessenbergQR<eT>::apply_YQ(Mat<eT>& Y)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (computed == false), "newarp::UpperHessenbergQR::apply_YQ(): need to call compute() first" );

  eT *Y_col_i, *Y_col_i1;
  uword nrow = Y.n_rows;
  for(uword i = 0; i < n - 1; i++)
    {
    const eT c = rot_cos(i);
    const eT s = rot_sin(i);
    Y_col_i  = Y.colptr(i);
    Y_col_i1 = Y.colptr(i + 1);
    for(uword j = 0; j < nrow; j++)
      {
      eT tmp = Y_col_i[j];
      Y_col_i[j]  = c * tmp - s * Y_col_i1[j];
      Y_col_i1[j] = s * tmp + c * Y_col_i1[j];
      }
    }
  }



template<typename eT>
inline
TridiagQR<eT>::TridiagQR()
  : UpperHessenbergQR<eT>()
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
TridiagQR<eT>::TridiagQR(const Mat<eT>& mat_obj)
  : UpperHessenbergQR<eT>()
  {
  arma_extra_debug_sigprint();

  this->compute(mat_obj);
  }



template<typename eT>
inline
void
TridiagQR<eT>::compute(const Mat<eT>& mat_obj)
  {
  arma_extra_debug_sigprint();

  this->n = mat_obj.n_rows;
  this->mat_T.set_size(this->n, this->n);
  this->rot_cos.set_size(this->n - 1);
  this->rot_sin.set_size(this->n - 1);

  this->mat_T.zeros();
  this->mat_T.diag()   = mat_obj.diag();
  this->mat_T.diag(1)  = mat_obj.diag(-1);
  this->mat_T.diag(-1) = mat_obj.diag(-1);

  eT xi, xj, r, c, s, tmp, eps = std::numeric_limits<eT>::epsilon();
  eT *ptr; // A number of pointers to avoid repeated address calculation
  for(uword i = 0; i < this->n - 1; i++)
    {
    xi = this->mat_T(i,     i);  // mat_T(i, i)
    xj = this->mat_T(i + 1, i);  // mat_T(i + 1, i)
    r = arma_hypot(xi, xj);
    if(r <= eps)
      {
      r = 0;
      this->rot_cos(i) = c = 1;
      this->rot_sin(i) = s = 0;
      }
    else
      {
      this->rot_cos(i) = c = xi / r;
      this->rot_sin(i) = s = -xj / r;
      }

    // For a complete QR decomposition,
    // we first obtain the rotation matrix
    // G = [ cos  sin]
    //     [-sin  cos]
    // and then do T[i:(i + 1), i:(i + 2)] = G' * T[i:(i + 1), i:(i + 2)]

    // Update T[i, i] and T[i + 1, i]
    // The updated value of T[i, i] is known to be r
    // The updated value of T[i + 1, i] is known to be 0
    this->mat_T(i,     i) = r;
    this->mat_T(i + 1, i) = 0;
    // Update T[i, i + 1] and T[i + 1, i + 1]
    // ptr[0] == T[i, i + 1]
    // ptr[1] == T[i + 1, i + 1]
    ptr = &(this->mat_T(i, i + 1));
    tmp = *ptr;
    ptr[0] = c * tmp - s * ptr[1];
    ptr[1] = s * tmp + c * ptr[1];

    if(i < this->n - 2)
      {
      // Update T[i, i + 2] and T[i + 1, i + 2]
      // ptr[0] == T[i, i + 2] == 0
      // ptr[1] == T[i + 1, i + 2]
      ptr = &(this->mat_T(i, i + 2));
      ptr[0] = -s * ptr[1];
      ptr[1] *= c;
      }
    }

  this->computed = true;
  }



template<typename eT>
Mat<eT>
TridiagQR<eT>::matrix_RQ()
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (this->computed == false), "newarp::TridiagQR::matrix_RQ(): need to call compute() first" );

  // Make a copy of the R matrix
  Mat<eT> RQ(this->n, this->n, fill::zeros);
  RQ.diag() = this->mat_T.diag();
  RQ.diag(1) = this->mat_T.diag(1);

  // [m11  m12] will point to RQ[i:(i+1), i:(i+1)]
  // [m21  m22]
  eT *m11 = RQ.memptr(), *m12, *m21, *m22, tmp;
  for(uword i = 0; i < this->n - 1; i++)
    {
    const eT c = this->rot_cos(i);
    const eT s = this->rot_sin(i);
    m21 = m11 + 1;
    m12 = m11 + this->n;
    m22 = m12 + 1;
    tmp = *m21;

    // Update diagonal and the below-subdiagonal
    *m11 = c * (*m11) - s * (*m12);
    *m21 = c * tmp    - s * (*m22);
    *m22 = s * tmp    + c * (*m22);

    // Move m11 to RQ[i+1, i+1]
    m11  = m22;
    }

  // Copy the below-subdiagonal to above-subdiagonal
  RQ.diag(1) = RQ.diag(-1);

  return RQ;
  }


}  // namespace newarp
