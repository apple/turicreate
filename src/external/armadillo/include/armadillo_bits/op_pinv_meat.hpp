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



//! \addtogroup op_pinv
//! @{



template<typename T1>
inline
void
op_pinv::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_pinv>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const T tol = access::tmp_real(in.aux);

  const bool use_divide_and_conquer = (in.aux_uword_a == 1);

  const bool status = op_pinv::apply_direct(out, in.m, tol, use_divide_and_conquer);

  if(status == false)
    {
    arma_stop_runtime_error("pinv(): svd failed");
    }
  }



template<typename T1>
inline
bool
op_pinv::apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& expr, typename T1::pod_type tol, const bool use_divide_and_conquer)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  arma_debug_check((tol < T(0)), "pinv(): tolerance must be >= 0");

  const Proxy<T1> P(expr.get_ref());

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();

  if( (n_rows*n_cols) == 0 )
    {
    out.set_size(n_cols,n_rows);
    return true;
    }


  // economical SVD decomposition
  Mat<eT> U;
  Col< T> s;
  Mat<eT> V;

  bool status = false;

  if(use_divide_and_conquer)
    {
    status = (n_cols > n_rows) ? auxlib::svd_dc_econ(U, s, V, trans(P.Q)) : auxlib::svd_dc_econ(U, s, V, P.Q);
    }
  else
    {
    status = (n_cols > n_rows) ? auxlib::svd_econ(U, s, V, trans(P.Q), 'b') : auxlib::svd_econ(U, s, V, P.Q, 'b');
    }

  if(status == false)
    {
    out.soft_reset();
    return false;
    }

  const uword s_n_elem = s.n_elem;
  const T*    s_mem    = s.memptr();

  // set tolerance to default if it hasn't been specified
  if( (tol == T(0)) && (s_n_elem > 0) )
    {
    tol = (std::max)(n_rows, n_cols) * s_mem[0] * std::numeric_limits<T>::epsilon();
    }


  uword count = 0;

  for(uword i = 0; i < s_n_elem; ++i)
    {
    count += (s_mem[i] >= tol) ? uword(1) : uword(0);
    }


  if(count > 0)
    {
    Col<T> s2(count);

    T* s2_mem = s2.memptr();

    uword count2 = 0;

    for(uword i=0; i < s_n_elem; ++i)
      {
      const T val = s_mem[i];

      if(val >= tol)  {  s2_mem[count2] = T(1) / val;  ++count2; }
      }


    if(n_rows >= n_cols)
      {
      out = ( (V.n_cols > count) ? V.cols(0,count-1) : V ) * diagmat(s2) * trans( (U.n_cols > count) ? U.cols(0,count-1) : U );
      }
    else
      {
      out = ( (U.n_cols > count) ? U.cols(0,count-1) : U ) * diagmat(s2) * trans( (V.n_cols > count) ? V.cols(0,count-1) : V );
      }
    }
  else
    {
    out.zeros(n_cols, n_rows);
    }

  return true;
  }



//! @}
