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
class DoubleShiftQR
  {
  private:

  uword               n;        // Dimension of the matrix
  Mat<eT>             mat_H;    // A copy of the matrix to be factorised
  eT                  shift_s;  // Shift constant
  eT                  shift_t;  // Shift constant
  Mat<eT>             ref_u;    // Householder reflectors
  Col<unsigned short> ref_nr;   // How many rows does each reflector affects
                                // 3 - A general reflector
                                // 2 - A Givens rotation
                                // 1 - An identity transformation
  const eT            prec;     // Approximately zero
  const eT            eps_rel;
  const eT            eps_abs;
  bool                computed; // Whether matrix has been factorised

  inline      void compute_reflector(const eT& x1, const eT& x2, const eT& x3, uword ind);
  arma_inline void compute_reflector(const eT* x, uword ind);

  // Update the block X = H(il:iu, il:iu)
  inline void update_block(uword il, uword iu);

  // P = I - 2 * u * u' = P'
  // PX = X - 2 * u * (u'X)
  inline void apply_PX(Mat<eT>& X, uword oi, uword oj, uword nrow, uword ncol, uword u_ind);

  // x is a pointer to a vector
  // Px = x - 2 * dot(x, u) * u
  inline void apply_PX(eT* x, uword u_ind);

  // XP = X - 2 * (X * u) * u'
  inline void apply_XP(Mat<eT>& X, uword oi, uword oj, uword nrow, uword ncol, uword u_ind);


  public:

  inline DoubleShiftQR(uword size);

  inline DoubleShiftQR(const Mat<eT>& mat_obj, eT s, eT t);

  inline void compute(const Mat<eT>& mat_obj, eT s, eT t);

  inline Mat<eT> matrix_QtHQ();

  inline void apply_QtY(Col<eT>& y);

  inline void apply_YQ(Mat<eT>& Y);
  };


}  // namespace newarp
