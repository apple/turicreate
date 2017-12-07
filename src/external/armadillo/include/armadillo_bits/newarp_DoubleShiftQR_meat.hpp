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
void
DoubleShiftQR<eT>::compute_reflector(const eT& x1, const eT& x2, const eT& x3, uword ind)
  {
  arma_extra_debug_sigprint();

  // In general case the reflector affects 3 rows
  ref_nr(ind) = 3;
  eT x2x3 = eT(0);
  // If x3 is zero, decrease nr by 1
  if(std::abs(x3) < prec)
    {
    // If x2 is also zero, nr will be 1, and we can exit this function
    if(std::abs(x2) < prec)
      {
      ref_nr(ind) = 1;
      return;
      }
    else
      {
      ref_nr(ind) = 2;
      }
    x2x3 = std::abs(x2);
    }
  else
    {
    x2x3 = arma_hypot(x2, x3);
    }

  // x1' = x1 - rho * ||x||
  // rho = -sign(x1), if x1 == 0, we choose rho = 1
  eT x1_new = x1 - ((x1 <= 0) - (x1 > 0)) * arma_hypot(x1, x2x3);
  eT x_norm = arma_hypot(x1_new, x2x3);
  // Double check the norm of new x
  if(x_norm < prec)
    {
    ref_nr(ind) = 1;
    return;
    }
  ref_u(0, ind) = x1_new / x_norm;
  ref_u(1, ind) = x2 / x_norm;
  ref_u(2, ind) = x3 / x_norm;
  }


template<typename eT>
arma_inline
void
DoubleShiftQR<eT>::compute_reflector(const eT* x, uword ind)
  {
  arma_extra_debug_sigprint();

  compute_reflector(x[0], x[1], x[2], ind);
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::update_block(uword il, uword iu)
  {
  arma_extra_debug_sigprint();

  // Block size
  uword bsize = iu - il + 1;

  // If block size == 1, there is no need to apply reflectors
  if(bsize == 1)
    {
    ref_nr(il) = 1;
    return;
    }

  // For block size == 2, do a Givens rotation on M = X * X - s * X + t * I
  if(bsize == 2)
    {
    // m00 = x00 * (x00 - s) + x01 * x10 + t
    eT m00 = mat_H(il, il) * (mat_H(il, il) - shift_s) +
             mat_H(il, il + 1) * mat_H(il + 1, il) +
             shift_t;
    // m10 = x10 * (x00 + x11 - s)
    eT m10 = mat_H(il + 1, il) * (mat_H(il, il) + mat_H(il + 1, il + 1) - shift_s);
    // This causes nr=2
    compute_reflector(m00, m10, 0, il);
    // Apply the reflector to X
    apply_PX(mat_H, il, il, 2, n - il, il);
    apply_XP(mat_H, 0, il, il + 2, 2, il);

    ref_nr(il + 1) = 1;
    return;
    }

  // For block size >=3, use the regular strategy
  eT m00 = mat_H(il, il) * (mat_H(il, il) - shift_s) +
           mat_H(il, il + 1) * mat_H(il + 1, il) +
           shift_t;
  eT m10 = mat_H(il + 1, il) * (mat_H(il, il) + mat_H(il + 1, il + 1) - shift_s);
  // m20 = x21 * x10
  eT m20 = mat_H(il + 2, il + 1) * mat_H(il + 1, il);
  compute_reflector(m00, m10, m20, il);

  // Apply the first reflector
  apply_PX(mat_H, il, il, 3, n - il, il);
  apply_XP(mat_H, 0, il, il + std::min(bsize, uword(4)), 3, il);

  // Calculate the following reflectors
  // If entering this loop, block size is at least 4.
  for(uword i = 1; i < bsize - 2; i++)
    {
    compute_reflector(mat_H.colptr(il + i - 1) + il + i, il + i);
    // Apply the reflector to X
    apply_PX(mat_H, il + i, il + i - 1, 3, n + 1 - il - i, il + i);
    apply_XP(mat_H, 0, il + i, il + std::min(bsize, uword(i + 4)), 3, il + i);
    }

  // The last reflector
  // This causes nr=2
  compute_reflector(mat_H(iu - 1, iu - 2), mat_H(iu, iu - 2), 0, iu - 1);
  // Apply the reflector to X
  apply_PX(mat_H, iu - 1, iu - 2, 2, n + 2 - iu, iu - 1);
  apply_XP(mat_H, 0, iu - 1, il + bsize, 2, iu - 1);

  ref_nr(iu) = 1;
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::apply_PX(Mat<eT>& X, uword oi, uword oj, uword nrow, uword ncol, uword u_ind)
  {
  arma_extra_debug_sigprint();

  if(ref_nr(u_ind) == 1) { return; }

  // Householder reflectors at index u_ind
  Col<eT> u(ref_u.colptr(u_ind), 3, false);

  const uword stride = X.n_rows;
  const eT u0_2 = 2 * u(0);
  const eT u1_2 = 2 * u(1);

  eT* xptr = &X(oi, oj);
  if(ref_nr(u_ind) == 2 || nrow == 2)
    {
    for(uword i = 0; i < ncol; i++, xptr += stride)
      {
      eT tmp = u0_2 * xptr[0] + u1_2 * xptr[1];
      xptr[0] -= tmp * u(0);
      xptr[1] -= tmp * u(1);
      }
    }
  else
    {
    const eT u2_2 = 2 * u(2);
    for(uword i = 0; i < ncol; i++, xptr += stride)
      {
      eT tmp = u0_2 * xptr[0] + u1_2 * xptr[1] + u2_2 * xptr[2];
      xptr[0] -= tmp * u(0);
      xptr[1] -= tmp * u(1);
      xptr[2] -= tmp * u(2);
      }
    }
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::apply_PX(eT* x, uword u_ind)
  {
  arma_extra_debug_sigprint();

  if(ref_nr(u_ind) == 1) { return; }

  eT u0 = ref_u(0, u_ind),
     u1 = ref_u(1, u_ind),
     u2 = ref_u(2, u_ind);

  // When the reflector only contains two elements, u2 has been set to zero
  bool nr_is_2 = (ref_nr(u_ind) == 2);
  eT dot2 = x[0] * u0 + x[1] * u1 + (nr_is_2 ? 0 : (x[2] * u2));
  dot2 *= 2;
  x[0] -= dot2 * u0;
  x[1] -= dot2 * u1;
  if(!nr_is_2) { x[2] -= dot2 * u2; }
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::apply_XP(Mat<eT>& X, uword oi, uword oj, uword nrow, uword ncol, uword u_ind)
  {
  arma_extra_debug_sigprint();

  if(ref_nr(u_ind) == 1) { return; }

  // Householder reflectors at index u_ind
  Col<eT> u(ref_u.colptr(u_ind), 3, false);
  uword stride = X.n_rows;
  const eT u0_2 = 2 * u(0);
  const eT u1_2 = 2 * u(1);
  eT* X0 = &X(oi, oj);
  eT* X1 = X0 + stride;  // X0 => X(oi, oj), X1 => X(oi, oj + 1)

  if(ref_nr(u_ind) == 2 || ncol == 2)
    {
    // tmp = 2 * u0 * X0 + 2 * u1 * X1
    // X0 => X0 - u0 * tmp
    // X1 => X1 - u1 * tmp
    for(uword i = 0; i < nrow; i++)
      {
      eT tmp = u0_2 * X0[i] + u1_2 * X1[i];
      X0[i] -= tmp * u(0);
      X1[i] -= tmp * u(1);
      }
    }
  else
    {
    eT* X2 = X1 + stride;  // X2 => X(oi, oj + 2)
    const eT u2_2 = 2 * u(2);
    for(uword i = 0; i < nrow; i++)
      {
      eT tmp = u0_2 * X0[i] + u1_2 * X1[i] + u2_2 * X2[i];
      X0[i] -= tmp * u(0);
      X1[i] -= tmp * u(1);
      X2[i] -= tmp * u(2);
      }
    }
  }



template<typename eT>
inline
DoubleShiftQR<eT>::DoubleShiftQR(uword size)
  : n(size)
  , prec(std::numeric_limits<eT>::epsilon())
  , eps_rel(prec)
  , eps_abs(prec)
  , computed(false)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
DoubleShiftQR<eT>::DoubleShiftQR(const Mat<eT>& mat_obj, eT s, eT t)
  : n(mat_obj.n_rows)
  , mat_H(n, n)
  , shift_s(s)
  , shift_t(t)
  , ref_u(3, n)
  , ref_nr(n)
  , prec(std::numeric_limits<eT>::epsilon())
  , eps_rel(prec)
  , eps_abs(prec)
  , computed(false)
  {
  arma_extra_debug_sigprint();

  compute(mat_obj, s, t);
  }



template<typename eT>
void
DoubleShiftQR<eT>::compute(const Mat<eT>& mat_obj, eT s, eT t)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (mat_obj.is_square() == false), "newarp::DoubleShiftQR::compute(): matrix must be square" );

  n = mat_obj.n_rows;
  mat_H.set_size(n, n);
  shift_s = s;
  shift_t = t;
  ref_u.set_size(3, n);
  ref_nr.set_size(n);

  // Make a copy of mat_obj
  mat_H = mat_obj;

  // Obtain the indices of zero elements in the subdiagonal,
  // so that H can be divided into several blocks
  std::vector<uword> zero_ind;
  zero_ind.reserve(n - 1);
  zero_ind.push_back(0);
  eT* Hii = mat_H.memptr();
  for(uword i = 0; i < n - 2; i++, Hii += (n + 1))
    {
    // Hii[1] => mat_H(i + 1, i)
    const eT h = std::abs(Hii[1]);
    if(h <= eps_abs || h <= eps_rel * (std::abs(Hii[0]) + std::abs(Hii[n + 1])))
      {
      Hii[1] = 0;
      zero_ind.push_back(i + 1);
      }
    // Make sure mat_H is upper Hessenberg
    // Zero the elements below mat_H(i + 1, i)
    std::fill(Hii + 2, Hii + n - i, eT(0));
    }
  zero_ind.push_back(n);

  for(std::vector<uword>::size_type i = 0; i < zero_ind.size() - 1; i++)
    {
    uword start = zero_ind[i];
    uword end = zero_ind[i + 1] - 1;
    // Compute refelctors from each block X
    update_block(start, end);
    }

  computed = true;
  }



template<typename eT>
Mat<eT>
DoubleShiftQR<eT>::matrix_QtHQ()
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (computed == false), "newarp::DoubleShiftQR::matrix_QtHQ(): need to call compute() first" );

  return mat_H;
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::apply_QtY(Col<eT>& y)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (computed == false), "newarp::DoubleShiftQR::apply_QtY(): need to call compute() first" );

  eT* y_ptr = y.memptr();
  for(uword i = 0; i < n - 1; i++, y_ptr++)
    {
    apply_PX(y_ptr, i);
    }
  }



template<typename eT>
inline
void
DoubleShiftQR<eT>::apply_YQ(Mat<eT>& Y)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (computed == false), "newarp::DoubleShiftQR::apply_YQ(): need to call compute() first" );

  uword nrow = Y.n_rows;
  for(uword i = 0; i < n - 2; i++)
    {
    apply_XP(Y, 0, i, nrow, 3, i);
    }

  apply_XP(Y, 0, n - 2, nrow, 2, n - 2);
  }


}  // namespace newarp
