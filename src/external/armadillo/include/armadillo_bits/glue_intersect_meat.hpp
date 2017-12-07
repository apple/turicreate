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


//! \addtogroup glue_intersect
//! @{



template<typename T1, typename T2>
inline
void
glue_intersect::apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_intersect>& X)
  {
  arma_extra_debug_sigprint();

  uvec iA;
  uvec iB;

  glue_intersect::apply(out, iA, iB, X.A, X.B, false);
  }



template<typename T1, typename T2>
inline
void
glue_intersect::apply(Mat<typename T1::elem_type>& out, uvec& iA, uvec& iB, const Base<typename T1::elem_type,T1>& A_expr, const Base<typename T1::elem_type,T2>& B_expr, const bool calc_indx)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const quasi_unwrap<T1> UA(A_expr.get_ref());
  const quasi_unwrap<T2> UB(B_expr.get_ref());

  if(UA.M.is_empty() || UB.M.is_empty())
    {
    out.reset();
    iA.reset();
    iB.reset();
    return;
    }

  uvec A_uniq_indx;
  uvec B_uniq_indx;

  Mat<eT> A_uniq;
  Mat<eT> B_uniq;

  if(calc_indx)
    {
    A_uniq_indx = find_unique(UA.M);
    B_uniq_indx = find_unique(UB.M);

    A_uniq = UA.M.elem(A_uniq_indx);
    B_uniq = UB.M.elem(B_uniq_indx);
    }
  else
    {
    A_uniq = unique(UA.M);
    B_uniq = unique(UB.M);
    }

  const uword C_n_elem = A_uniq.n_elem + B_uniq.n_elem;

  Col<eT> C(C_n_elem);

  arrayops::copy(C.memptr(),                 A_uniq.memptr(), A_uniq.n_elem);
  arrayops::copy(C.memptr() + A_uniq.n_elem, B_uniq.memptr(), B_uniq.n_elem);

  uvec    C_sorted_indx;
  Col<eT> C_sorted;

  if(calc_indx)
    {
    C_sorted_indx = sort_index(C);
    C_sorted      = C.elem(C_sorted_indx);
    }
  else
    {
    C_sorted = sort(C);
    }

  const eT* C_sorted_mem = C_sorted.memptr();

  uvec   jj(C_n_elem);  // worst case length

  uword* jj_mem   = jj.memptr();
  uword  jj_count = 0;

  for(uword i=0; i < (C_n_elem-1); ++i)
    {
    if( C_sorted_mem[i] == C_sorted_mem[i+1] )
      {
      jj_mem[jj_count] = i;
      ++jj_count;
      }
    }

  if(jj_count == 0)
    {
    out.reset();
    iA.reset();
    iB.reset();
    return;
    }

  const uvec ii(jj.memptr(), jj_count, false);

  if(UA.M.is_rowvec() && UB.M.is_rowvec())
    {
    out.set_size(1, ii.n_elem);

    Mat<eT> out_alias(out.memptr(), ii.n_elem, 1, false, true);

    // NOTE: this relies on .elem() not changing the size of the output and not reallocating memory for the output
    out_alias = C_sorted.elem(ii);
    }
  else
    {
    out = C_sorted.elem(ii);
    }

  if(calc_indx)
    {
    iA = A_uniq_indx.elem(C_sorted_indx.elem(ii  )                );
    iB = B_uniq_indx.elem(C_sorted_indx.elem(ii+1) - A_uniq.n_elem);
    }
  }



//! @}
