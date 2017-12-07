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
DenseGenMatProd<eT>::DenseGenMatProd(const Mat<eT>& mat_obj)
  : op_mat(mat_obj)
  , n_rows(mat_obj.n_rows)
  , n_cols(mat_obj.n_cols)
  {
  arma_extra_debug_sigprint();
  }



// Perform the matrix-vector multiplication operation \f$y=Ax\f$.
// y_out = A * x_in
template<typename eT>
inline
void
DenseGenMatProd<eT>::perform_op(eT* x_in, eT* y_out) const
  {
  arma_extra_debug_sigprint();

  Col<eT> x(x_in , n_cols, false);
  Col<eT> y(y_out, n_rows, false);
  y = op_mat * x;
  }


}  // namespace newarp
