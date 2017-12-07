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


//! \addtogroup fn_syl_lyap
//! @{


//! find the solution of the Sylvester equation AX + XB = C
template<typename T1, typename T2, typename T3>
inline
bool
syl
  (
        Mat <typename T1::elem_type>   & out,
  const Base<typename T1::elem_type,T1>& in_A,
  const Base<typename T1::elem_type,T2>& in_B,
  const Base<typename T1::elem_type,T3>& in_C,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> tmp_A(in_A.get_ref(), out);
  const unwrap_check<T2> tmp_B(in_B.get_ref(), out);
  const unwrap_check<T3> tmp_C(in_C.get_ref(), out);

  const Mat<eT>& A = tmp_A.M;
  const Mat<eT>& B = tmp_B.M;
  const Mat<eT>& C = tmp_C.M;

  const bool status = auxlib::syl(out, A, B, C);

  if(status == false)
    {
    out.soft_reset();
    arma_debug_warn("syl(): solution not found");
    }

  return status;
  }



template<typename T1, typename T2, typename T3>
arma_warn_unused
inline
Mat<typename T1::elem_type>
syl
  (
  const Base<typename T1::elem_type,T1>& in_A,
  const Base<typename T1::elem_type,T2>& in_B,
  const Base<typename T1::elem_type,T3>& in_C,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap<T1> tmp_A( in_A.get_ref() );
  const unwrap<T2> tmp_B( in_B.get_ref() );
  const unwrap<T3> tmp_C( in_C.get_ref() );

  const Mat<eT>& A = tmp_A.M;
  const Mat<eT>& B = tmp_B.M;
  const Mat<eT>& C = tmp_C.M;

  Mat<eT> out;

  const bool status = auxlib::syl(out, A, B, C);

  if(status == false)
    {
    out.soft_reset();
    arma_stop_runtime_error("syl(): solution not found");
    }

  return out;
  }



//! @}
