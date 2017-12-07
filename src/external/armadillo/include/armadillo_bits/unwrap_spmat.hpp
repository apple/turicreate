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


//! \addtogroup unwrap_spmat
//! @{



template<typename T1>
struct unwrap_spmat
  {
  typedef typename T1::elem_type eT;

  typedef SpMat<eT> stored_type;

  inline
  unwrap_spmat(const T1& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const SpMat<eT> M;
  };



template<typename eT>
struct unwrap_spmat< SpMat<eT> >
  {
  typedef SpMat<eT> stored_type;

  inline
  unwrap_spmat(const SpMat<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();

    M.sync();
    }

  const SpMat<eT>& M;
  };



template<typename eT>
struct unwrap_spmat< SpRow<eT> >
  {
  typedef SpRow<eT> stored_type;

  inline
  unwrap_spmat(const SpRow<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();

    M.sync();
    }

  const SpRow<eT>& M;
  };



template<typename eT>
struct unwrap_spmat< SpCol<eT> >
  {
  typedef SpCol<eT> stored_type;

  inline
  unwrap_spmat(const SpCol<eT>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();

    M.sync();
    }

  const SpCol<eT>& M;
  };



template<typename T1, typename spop_type>
struct unwrap_spmat< SpOp<T1, spop_type> >
  {
  typedef typename T1::elem_type eT;

  typedef SpMat<eT> stored_type;

  inline
  unwrap_spmat(const SpOp<T1, spop_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const SpMat<eT> M;
  };



template<typename T1, typename T2, typename spglue_type>
struct unwrap_spmat< SpGlue<T1, T2, spglue_type> >
  {
  typedef typename T1::elem_type eT;

  typedef SpMat<eT> stored_type;

  inline
  unwrap_spmat(const SpGlue<T1, T2, spglue_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const SpMat<eT> M;
  };



template<typename out_eT, typename T1, typename spop_type>
struct unwrap_spmat< mtSpOp<out_eT, T1, spop_type> >
  {
  typedef SpMat<out_eT> stored_type;

  inline
  unwrap_spmat(const mtSpOp<out_eT, T1, spop_type>& A)
    : M(A)
    {
    arma_extra_debug_sigprint();
    }

  const SpMat<out_eT> M;
  };



//! @}
