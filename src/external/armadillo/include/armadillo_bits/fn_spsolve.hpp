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


//! \addtogroup fn_spsolve
//! @{

//! Solve a system of linear equations, i.e., A*X = B, where X is unknown,
//! A is sparse, and B is dense.  X will be dense too.

template<typename T1, typename T2>
inline
bool
spsolve_helper
  (
           Mat<typename T1::elem_type>&     out,
  const SpBase<typename T1::elem_type, T1>& A,
  const   Base<typename T1::elem_type, T2>& B,
  const char*                          solver,
  const spsolve_opts_base&             settings,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type   T;
  typedef typename T1::elem_type eT;

  const char sig = (solver != NULL) ? solver[0] : char(0);

  arma_debug_check( ((sig != 'l') && (sig != 's')), "spsolve(): unknown solver" );

  T rcond = T(0);

  bool status = false;

  if(sig == 's')  // SuperLU solver
    {
    const superlu_opts& opts = (settings.id == 1) ? static_cast<const superlu_opts&>(settings) : superlu_opts();

    arma_debug_check( ( (opts.pivot_thresh < double(0)) || (opts.pivot_thresh > double(1)) ), "spsolve(): pivot_thresh out of bounds" );

    if( (opts.equilibrate == false) && (opts.refine == superlu_opts::REF_NONE) )
      {
      status = sp_auxlib::spsolve_simple(out, A.get_ref(), B.get_ref(), opts);
      }
    else
      {
      status = sp_auxlib::spsolve_refine(out, rcond, A.get_ref(), B.get_ref(), opts);
      }
    }
  else
  if(sig == 'l')  // brutal LAPACK solver
    {
    if(settings.id != 0)  { arma_debug_warn("spsolve(): ignoring settings not applicable to LAPACK based solver"); }

    Mat<eT> AA;

    bool conversion_ok = false;

    try
      {
      Mat<eT> tmp(A.get_ref());  // conversion from sparse to dense can throw std::bad_alloc

      AA.steal_mem(tmp);

      conversion_ok = true;
      }
    catch(std::bad_alloc&)
      {
      arma_debug_warn("spsolve(): not enough memory to use LAPACK based solver");
      }

    if(conversion_ok)
      {
      arma_debug_check( (AA.n_rows != AA.n_cols), "spsolve(): matrix A must be square sized" );

      status = auxlib::solve_square_refine(out, rcond, AA, B.get_ref(), false);
      }
    }


  if(status == false)
    {
    if(rcond > T(0))  { arma_debug_warn("spsolve(): system seems singular (rcond: ", rcond, ")"); }
    else              { arma_debug_warn("spsolve(): system seems singular");                      }

    out.soft_reset();
    }

  return status;
  }



template<typename T1, typename T2>
inline
bool
spsolve
  (
           Mat<typename T1::elem_type>&     out,
  const SpBase<typename T1::elem_type, T1>& A,
  const   Base<typename T1::elem_type, T2>& B,
  const char*                          solver   = "superlu",
  const spsolve_opts_base&             settings = spsolve_opts_none(),
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool status = spsolve_helper(out, A.get_ref(), B.get_ref(), solver, settings);

  return status;
  }



template<typename T1, typename T2>
arma_warn_unused
inline
Mat<typename T1::elem_type>
spsolve
  (
  const SpBase<typename T1::elem_type, T1>& A,
  const   Base<typename T1::elem_type, T2>& B,
  const char*                          solver   = "superlu",
  const spsolve_opts_base&             settings = spsolve_opts_none(),
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  Mat<eT> out;

  const bool status = spsolve_helper(out, A.get_ref(), B.get_ref(), solver, settings);

  if(status == false)
    {
    arma_stop_runtime_error("spsolve(): solution not found");
    }

  return out;
  }



//! @}
