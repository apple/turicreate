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



//! \brief
//! principal component analysis -- 4 arguments version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
//! tsquared_out -> Hotelling's T^2 statistic
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat<typename T1::elem_type>&     coeff_out,
         Mat<typename T1::elem_type>&     score_out,
         Col<typename T1::elem_type>&     latent_out,
         Col<typename T1::elem_type>&     tsquared_out,
  const Base<typename T1::elem_type, T1>& X,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col<eT> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();

      //Col<eT> s_tmp = zeros< Col<eT> >(n_cols);
      Col<eT> s_tmp(n_cols);
      s_tmp.zeros();

      s_tmp.rows(0,n_rows-2) = s.rows(0,n_rows-2);
      s = s_tmp;

      // compute the Hotelling's T-squared
      s_tmp.rows(0,n_rows-2) = eT(1) / s_tmp.rows(0,n_rows-2);

      const Mat<eT> S = score_out * diagmat(Col<eT>(s_tmp));
      tsquared_out = sum(S%S,1);
      }
    else
      {
      // compute the Hotelling's T-squared
      const Mat<eT> S = score_out * diagmat(Col<eT>( eT(1) / s));
      tsquared_out = sum(S%S,1);
      }

    // compute the eigenvalues of the principal vectors
    latent_out = s%s;
    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);

    score_out.copy_size(in);
    score_out.zeros();

    latent_out.set_size(n_cols);
    latent_out.zeros();

    tsquared_out.set_size(n_rows);
    tsquared_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 3 arguments version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat<typename T1::elem_type>&     coeff_out,
         Mat<typename T1::elem_type>&     score_out,
         Col<typename T1::elem_type>&     latent_out,
  const Base<typename T1::elem_type, T1>& X,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col<eT> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();

      Col<eT> s_tmp = zeros< Col<eT> >(n_cols);
      s_tmp.rows(0,n_rows-2) = s.rows(0,n_rows-2);
      s = s_tmp;
      }

    // compute the eigenvalues of the principal vectors
    latent_out = s%s;

    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);

    score_out.copy_size(in);
    score_out.zeros();

    latent_out.set_size(n_cols);
    latent_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 2 arguments version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat<typename T1::elem_type>&     coeff_out,
         Mat<typename T1::elem_type>&     score_out,
  const Base<typename T1::elem_type, T1>& X,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col<eT> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();

      Col<eT> s_tmp = zeros< Col<eT> >(n_cols);
      s_tmp.rows(0,n_rows-2) = s.rows(0,n_rows-2);
      s = s_tmp;
      }
    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);
    score_out.copy_size(in);
    score_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 1 argument version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat<typename T1::elem_type>&     coeff_out,
  const Base<typename T1::elem_type, T1>& X,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const unwrap<T1>    Y( X.get_ref() );
  const Mat<eT>& in = Y.M;

  if(in.n_elem != 0)
    {
    Mat<eT> tmp = in; tmp.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col<eT> s;

    const bool svd_ok = svd(U, s, coeff_out, tmp);

    if(svd_ok == false)  { return false; }
    }
  else
    {
    coeff_out.eye(in.n_cols, in.n_cols);
    }

  return true;
  }



//! \brief
//! principal component analysis -- 4 arguments complex version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
//! tsquared_out -> Hotelling's T^2 statistic
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat< std::complex<typename T1::pod_type> >&     coeff_out,
         Mat< std::complex<typename T1::pod_type> >&     score_out,
         Col<              typename T1::pod_type  >&     latent_out,
         Col< std::complex<typename T1::pod_type> >&     tsquared_out,
  const Base< std::complex<typename T1::pod_type>, T1 >& X,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type     T;
  typedef          std::complex<T> eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col< T> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();

      Col<T> s_tmp = zeros< Col<T> >(n_cols);
      s_tmp.rows(0,n_rows-2) = s.rows(0,n_rows-2);
      s = s_tmp;

      // compute the Hotelling's T-squared
      s_tmp.rows(0,n_rows-2) = 1.0 / s_tmp.rows(0,n_rows-2);
      const Mat<eT> S = score_out * diagmat(Col<T>(s_tmp));
      tsquared_out = sum(S%S,1);
      }
    else
      {
      // compute the Hotelling's T-squared
      const Mat<eT> S = score_out * diagmat(Col<T>(T(1) / s));
      tsquared_out = sum(S%S,1);
      }

    // compute the eigenvalues of the principal vectors
    latent_out = s%s;

    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);

    score_out.copy_size(in);
    score_out.zeros();

    latent_out.set_size(n_cols);
    latent_out.zeros();

    tsquared_out.set_size(n_rows);
    tsquared_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 3 arguments complex version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat< std::complex<typename T1::pod_type> >&     coeff_out,
         Mat< std::complex<typename T1::pod_type> >&     score_out,
         Col<              typename T1::pod_type  >&     latent_out,
  const Base< std::complex<typename T1::pod_type>, T1 >& X,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type     T;
  typedef          std::complex<T> eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col< T> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();

      Col<T> s_tmp = zeros< Col<T> >(n_cols);
      s_tmp.rows(0,n_rows-2) = s.rows(0,n_rows-2);
      s = s_tmp;
      }

    // compute the eigenvalues of the principal vectors
    latent_out = s%s;
    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);

    score_out.copy_size(in);
    score_out.zeros();

    latent_out.set_size(n_cols);
    latent_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 2 arguments complex version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat< std::complex<typename T1::pod_type> >&     coeff_out,
         Mat< std::complex<typename T1::pod_type> >&     score_out,
  const Base< std::complex<typename T1::pod_type>, T1 >& X,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type     T;
  typedef          std::complex<T> eT;

  const unwrap_check<T1> Y( X.get_ref(), score_out );
  const Mat<eT>& in    = Y.M;

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows > 1) // more than one sample
    {
    // subtract the mean - use score_out as temporary matrix
    score_out = in;  score_out.each_row() -= mean(in);

    // singular value decomposition
    Mat<eT> U;
    Col< T> s;

    const bool svd_ok = svd(U, s, coeff_out, score_out);

    if(svd_ok == false)  { return false; }

    // normalize the eigenvalues
    s /= std::sqrt( double(n_rows - 1) );

    // project the samples to the principals
    score_out *= coeff_out;

    if(n_rows <= n_cols) // number of samples is less than their dimensionality
      {
      score_out.cols(n_rows-1,n_cols-1).zeros();
      }

    }
  else // 0 or 1 samples
    {
    coeff_out.eye(n_cols, n_cols);

    score_out.copy_size(in);
    score_out.zeros();
    }

  return true;
  }



//! \brief
//! principal component analysis -- 1 argument complex version
//! computation is done via singular value decomposition
//! coeff_out    -> principal component coefficients
template<typename T1>
inline
bool
op_princomp::direct_princomp
  (
         Mat< std::complex<typename T1::pod_type> >&     coeff_out,
  const Base< std::complex<typename T1::pod_type>, T1 >& X,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type     T;
  typedef          std::complex<T> eT;

  const unwrap<T1>    Y( X.get_ref() );
  const Mat<eT>& in = Y.M;

  if(in.n_elem != 0)
    {
    // singular value decomposition
    Mat<eT> U;
    Col< T> s;

    Mat<eT> tmp = in;  tmp.each_row() -= mean(in);

    const bool svd_ok = svd(U, s, coeff_out, tmp);

    if(svd_ok == false)  { return false; }
    }
  else
    {
    coeff_out.eye(in.n_cols, in.n_cols);
    }

  return true;
  }



template<typename T1>
inline
void
op_princomp::apply
  (
        Mat<typename T1::elem_type>& out,
  const Op<T1,op_princomp>&          in
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_check<T1> tmp(in.m, out);
  const Mat<eT>& A     = tmp.M;

  const bool status = op_princomp::direct_princomp(out, A);

  if(status == false)
    {
    out.soft_reset();

    arma_stop_runtime_error("princomp(): decomposition failed");
    }
  }



//! @}
