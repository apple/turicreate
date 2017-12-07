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


//! Define matrix operations on existing matrix objects
template<typename eT>
class DenseGenMatProd
  {
  private:

  const Mat<eT>& op_mat;


  public:

  const uword n_rows;  // number of rows of the underlying matrix
  const uword n_cols;  // number of columns of the underlying matrix

  inline DenseGenMatProd(const Mat<eT>& mat_obj);

  inline void perform_op(eT* x_in, eT* y_out) const;
  };


}  // namespace newarp
