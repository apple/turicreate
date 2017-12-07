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



//! \addtogroup op_hist
//! @{



class op_hist
  {
  public:

  template<typename eT>
  inline static void apply_noalias(Mat<uword>& out, const Mat<eT>& A, const uword n_bins, const bool A_is_row);

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_hist>& X);
  };



//! @}
