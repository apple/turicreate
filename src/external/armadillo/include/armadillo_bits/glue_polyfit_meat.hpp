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


//! \addtogroup glue_polyfit
//! @{



template<typename eT>
inline
bool
glue_polyfit::apply_noalias(Mat<eT>& out, const Col<eT>& X, const Col<eT>& Y, const uword N)
  {
  arma_extra_debug_sigprint();

  // create Vandermonde matrix

  Mat<eT> V(X.n_elem, N+1);

  V.tail_cols(1).ones();

  for(uword i=1; i <= N; ++i)
    {
    const uword j = N-i;

    Col<eT> V_col_j  (V.colptr(j  ), V.n_rows, false, false);
    Col<eT> V_col_jp1(V.colptr(j+1), V.n_rows, false, false);

    V_col_j = V_col_jp1 % X;
    }

  Mat<eT> Q;
  Mat<eT> R;

  const bool status1 = auxlib::qr_econ(Q, R, V);

  if(status1 == false)  { return false; }

  const bool status2 = auxlib::solve_tri(out, R, (Q.t() * Y), uword(0));

  if(status2 == false)  { return false; }

  return true;
  }



template<typename T1, typename T2>
inline
bool
glue_polyfit::apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& X_expr, const Base<typename T1::elem_type, T2>& Y_expr, const uword N)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const quasi_unwrap<T1> UX(X_expr.get_ref());
  const quasi_unwrap<T2> UY(Y_expr.get_ref());

  const Mat<eT>& X = UX.M;
  const Mat<eT>& Y = UY.M;

  arma_debug_check
    (
    ( ((X.is_vec() == false) && (X.is_empty() == false)) || ((Y.is_vec() == false) && (Y.is_empty() == false)) ),
    "polyfit(): given object is not a vector"
    );

  arma_debug_check( (X.n_elem != Y.n_elem), "polyfit(): given vectors must have the same number of elements" );

  if(X.n_elem == 0)
    {
    out.reset();
    return true;
    }

  arma_debug_check( (N >= X.n_elem), "polyfit(): N must be less than the number of elements in X" );

  const Col<eT> X_as_colvec( const_cast<eT*>(X.memptr()), X.n_elem, false, false);
  const Col<eT> Y_as_colvec( const_cast<eT*>(Y.memptr()), Y.n_elem, false, false);

  bool status = false;

  if(UX.is_alias(out) || UY.is_alias(out))
    {
    Mat<eT> tmp;
    status = glue_polyfit::apply_noalias(tmp, X_as_colvec, Y_as_colvec, N);
    out.steal_mem(tmp);
    }
  else
    {
    status = glue_polyfit::apply_noalias(out, X_as_colvec, Y_as_colvec, N);
    }

  return status;
  }



template<typename T1, typename T2>
inline
void
glue_polyfit::apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_polyfit>& expr)
  {
  arma_extra_debug_sigprint();

  const bool status = glue_polyfit::apply_direct(out, expr.A, expr.B, expr.aux_uword);

  if(status == false)
    {
    out.soft_reset();
    arma_stop_runtime_error("polyfit(): failed");
    }
  }



//! @}
