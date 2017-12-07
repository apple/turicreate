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


//! \addtogroup op_princomp
//! @{



class op_princomp
  {
  public:

  //
  // real element versions

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat<typename T1::elem_type>&     coeff_out,
    const Base<typename T1::elem_type, T1>& X,
    const typename arma_not_cx<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat<typename T1::elem_type>&     coeff_out,
           Mat<typename T1::elem_type>&     score_out,
    const Base<typename T1::elem_type, T1>& X,
    const typename arma_not_cx<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat<typename T1::elem_type>&     coeff_out,
           Mat<typename T1::elem_type>&     score_out,
           Col<typename T1::elem_type>&     latent_out,
    const Base<typename T1::elem_type, T1>& X,
    const typename arma_not_cx<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat<typename T1::elem_type>&     coeff_out,
           Mat<typename T1::elem_type>&     score_out,
           Col<typename T1::elem_type>&     latent_out,
           Col<typename T1::elem_type>&     tsquared_out,
    const Base<typename T1::elem_type, T1>& X,
    const typename arma_not_cx<typename T1::elem_type>::result* junk = 0
    );


  //
  // complex element versions

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat< std::complex<typename T1::pod_type> >&     coeff_out,
    const Base< std::complex<typename T1::pod_type>, T1 >& X,
    const typename arma_cx_only<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat< std::complex<typename T1::pod_type> >&     coeff_out,
           Mat< std::complex<typename T1::pod_type> >&     score_out,
    const Base< std::complex<typename T1::pod_type>, T1 >& X,
    const typename arma_cx_only<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat< std::complex<typename T1::pod_type> >&     coeff_out,
           Mat< std::complex<typename T1::pod_type> >&     score_out,
           Col<              typename T1::pod_type  >&     latent_out,
    const Base< std::complex<typename T1::pod_type>, T1 >& X,
    const typename arma_cx_only<typename T1::elem_type>::result* junk = 0
    );

  template<typename T1>
  inline static bool
  direct_princomp
    (
           Mat< std::complex<typename T1::pod_type> >&     coeff_out,
           Mat< std::complex<typename T1::pod_type> >&     score_out,
           Col<              typename T1::pod_type  >&     latent_out,
           Col< std::complex<typename T1::pod_type> >&     tsquared_out,
    const Base< std::complex<typename T1::pod_type>, T1 >& X,
    const typename arma_cx_only<typename T1::elem_type>::result* junk = 0
    );


  template<typename T1>
  inline static void
  apply(Mat<typename T1::elem_type>& out, const Op<T1,op_princomp>& in);

  };



//! @}
