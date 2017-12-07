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



//! \addtogroup glue_cross
//! @{



template<typename T1, typename T2>
inline
void
glue_cross::apply(Mat<typename T1::elem_type>& out, const Glue<T1, T2, glue_cross>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> PA(X.A);
  const Proxy<T2> PB(X.B);

  arma_debug_check( ((PA.get_n_elem() != 3) || (PB.get_n_elem() != 3)), "cross(): input vectors must have 3 elements" );

  const uword PA_n_rows = Proxy<T1>::is_row ? 1 : PA.get_n_rows();
  const uword PA_n_cols = Proxy<T1>::is_col ? 1 : PA.get_n_cols();

  out.set_size(PA_n_rows, PA_n_cols);

  eT* out_mem = out.memptr();

  if( (Proxy<T1>::use_at == false) && (Proxy<T2>::use_at == false) )
    {
    typename Proxy<T1>::ea_type A = PA.get_ea();
    typename Proxy<T2>::ea_type B = PB.get_ea();

    const eT ax = A[0];
    const eT ay = A[1];
    const eT az = A[2];

    const eT bx = B[0];
    const eT by = B[1];
    const eT bz = B[2];

    out_mem[0] = ay*bz - az*by;
    out_mem[1] = az*bx - ax*bz;
    out_mem[2] = ax*by - ay*bx;
    }
  else
    {
    const bool PA_is_col = Proxy<T1>::is_col ? true : (PA_n_cols       == 1);
    const bool PB_is_col = Proxy<T2>::is_col ? true : (PB.get_n_cols() == 1);

    const eT ax = PA.at(0,0);
    const eT ay = PA_is_col ? PA.at(1,0) : PA.at(0,1);
    const eT az = PA_is_col ? PA.at(2,0) : PA.at(0,2);

    const eT bx = PB.at(0,0);
    const eT by = PB_is_col ? PB.at(1,0) : PB.at(0,1);
    const eT bz = PB_is_col ? PB.at(2,0) : PB.at(0,2);

    out_mem[0] = ay*bz - az*by;
    out_mem[1] = az*bx - ax*bz;
    out_mem[2] = ax*by - ay*bx;
    }
  }



//! @}
