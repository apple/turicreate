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


//! \addtogroup running_stat_vec
//! @{



template<typename obj_type>
inline
running_stat_vec<obj_type>::~running_stat_vec()
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename obj_type>
inline
running_stat_vec<obj_type>::running_stat_vec(const bool in_calc_cov)
  : calc_cov(in_calc_cov)
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename obj_type>
inline
running_stat_vec<obj_type>::running_stat_vec(const running_stat_vec<obj_type>& in_rsv)
  : calc_cov    (in_rsv.calc_cov)
  , counter     (in_rsv.counter)
  , r_mean      (in_rsv.r_mean)
  , r_var       (in_rsv.r_var)
  , r_cov       (in_rsv.r_cov)
  , min_val     (in_rsv.min_val)
  , max_val     (in_rsv.max_val)
  , min_val_norm(in_rsv.min_val_norm)
  , max_val_norm(in_rsv.max_val_norm)
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename obj_type>
inline
running_stat_vec<obj_type>&
running_stat_vec<obj_type>::operator=(const running_stat_vec<obj_type>& in_rsv)
  {
  arma_extra_debug_sigprint();

  access::rw(calc_cov) = in_rsv.calc_cov;

  counter      = in_rsv.counter;
  r_mean       = in_rsv.r_mean;
  r_var        = in_rsv.r_var;
  r_cov        = in_rsv.r_cov;
  min_val      = in_rsv.min_val;
  max_val      = in_rsv.max_val;
  min_val_norm = in_rsv.min_val_norm;
  max_val_norm = in_rsv.max_val_norm;

  return *this;
  }



//! update statistics to reflect new sample
template<typename obj_type>
template<typename T1>
arma_hot
inline
void
running_stat_vec<obj_type>::operator() (const Base<typename running_stat_vec<obj_type>::T, T1>& X)
  {
  arma_extra_debug_sigprint();

  const quasi_unwrap<T1> tmp(X.get_ref());
  const Mat<T>& sample = tmp.M;

  if( sample.is_empty() )
    {
    return;
    }

  if( sample.is_finite() == false )
    {
    arma_debug_warn("running_stat_vec: sample ignored as it has non-finite elements");
    return;
    }

  running_stat_vec_aux::update_stats(*this, sample);
  }



template<typename obj_type>
template<typename T1>
arma_hot
inline
void
running_stat_vec<obj_type>::operator() (const Base< std::complex<typename running_stat_vec<obj_type>::T>, T1>& X)
  {
  arma_extra_debug_sigprint();

  const quasi_unwrap<T1> tmp(X.get_ref());

  const Mat< std::complex<T> >& sample = tmp.M;

  if( sample.is_empty() )
    {
    return;
    }

  if( sample.is_finite() == false )
    {
    arma_debug_warn("running_stat_vec: sample ignored as it has non-finite elements");
    return;
    }

  running_stat_vec_aux::update_stats(*this, sample);
  }



//! set all statistics to zero
template<typename obj_type>
inline
void
running_stat_vec<obj_type>::reset()
  {
  arma_extra_debug_sigprint();

  counter.reset();

  r_mean.reset();
  r_var.reset();
  r_cov.reset();

  min_val.reset();
  max_val.reset();

  min_val_norm.reset();
  max_val_norm.reset();

  r_var_dummy.reset();
  r_cov_dummy.reset();

  tmp1.reset();
  tmp2.reset();
  }



//! mean or average value
template<typename obj_type>
inline
const typename running_stat_vec<obj_type>::return_type1&
running_stat_vec<obj_type>::mean() const
  {
  arma_extra_debug_sigprint();

  return r_mean;
  }



//! variance
template<typename obj_type>
inline
const typename running_stat_vec<obj_type>::return_type2&
running_stat_vec<obj_type>::var(const uword norm_type)
  {
  arma_extra_debug_sigprint();

  const T N = counter.value();

  if(N > T(1))
    {
    if(norm_type == 0)
      {
      return r_var;
      }
    else
      {
      const T N_minus_1 = counter.value_minus_1();

      r_var_dummy = (N_minus_1/N) * r_var;

      return r_var_dummy;
      }
    }
  else
    {
    r_var_dummy.zeros(r_mean.n_rows, r_mean.n_cols);

    return r_var_dummy;
    }

  }



//! standard deviation
template<typename obj_type>
inline
typename running_stat_vec<obj_type>::return_type2
running_stat_vec<obj_type>::stddev(const uword norm_type) const
  {
  arma_extra_debug_sigprint();

  const T N = counter.value();

  if(N > T(1))
    {
    if(norm_type == 0)
      {
      return sqrt(r_var);
      }
    else
      {
      const T N_minus_1 = counter.value_minus_1();

      return sqrt( (N_minus_1/N) * r_var );
      }
    }
  else
    {
    typedef typename running_stat_vec<obj_type>::return_type2 out_type;
    return out_type();
    }
  }



//! covariance
template<typename obj_type>
inline
const Mat< typename running_stat_vec<obj_type>::eT >&
running_stat_vec<obj_type>::cov(const uword norm_type)
  {
  arma_extra_debug_sigprint();

  if(calc_cov == true)
    {
    const T N = counter.value();

    if(N > T(1))
      {
      if(norm_type == 0)
        {
        return r_cov;
        }
      else
        {
        const T N_minus_1 = counter.value_minus_1();

        r_cov_dummy = (N_minus_1/N) * r_cov;

        return r_cov_dummy;
        }
      }
    else
      {
      const uword out_size = (std::max)(r_mean.n_rows, r_mean.n_cols);

      r_cov_dummy.zeros(out_size, out_size);

      return r_cov_dummy;
      }
    }
  else
    {
    r_cov_dummy.reset();

    return r_cov_dummy;
    }

  }



//! vector with minimum values
template<typename obj_type>
inline
const typename running_stat_vec<obj_type>::return_type1&
running_stat_vec<obj_type>::min() const
  {
  arma_extra_debug_sigprint();

  return min_val;
  }



//! vector with maximum values
template<typename obj_type>
inline
const typename running_stat_vec<obj_type>::return_type1&
running_stat_vec<obj_type>::max() const
  {
  arma_extra_debug_sigprint();

  return max_val;
  }



template<typename obj_type>
inline
typename running_stat_vec<obj_type>::return_type1
running_stat_vec<obj_type>::range() const
  {
  arma_extra_debug_sigprint();

  return (max_val - min_val);
  }



//! number of samples so far
template<typename obj_type>
inline
typename running_stat_vec<obj_type>::T
running_stat_vec<obj_type>::count() const
  {
  arma_extra_debug_sigprint();

  return counter.value();
  }



//



//! update statistics to reflect new sample (version for non-complex numbers)
template<typename obj_type>
inline
void
running_stat_vec_aux::update_stats
  (
  running_stat_vec<obj_type>& x,
  const                  Mat<typename running_stat_vec<obj_type>::eT>& sample,
  const typename arma_not_cx<typename running_stat_vec<obj_type>::eT>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename running_stat_vec<obj_type>::eT eT;
  typedef typename running_stat_vec<obj_type>::T   T;

  const T N = x.counter.value();

  if(N > T(0))
    {
    arma_debug_assert_same_size(x.r_mean, sample, "running_stat_vec(): dimensionality mismatch");

    const uword n_elem      = sample.n_elem;
    const eT*   sample_mem  = sample.memptr();
          eT*   r_mean_mem  = x.r_mean.memptr();
           T*   r_var_mem   = x.r_var.memptr();
          eT*   min_val_mem = x.min_val.memptr();
          eT*   max_val_mem = x.max_val.memptr();

    const T  N_plus_1   = x.counter.value_plus_1();
    const T  N_minus_1  = x.counter.value_minus_1();

    if(x.calc_cov == true)
      {
      Mat<eT>& tmp1 = x.tmp1;
      Mat<eT>& tmp2 = x.tmp2;

      tmp1 = sample - x.r_mean;

      if(sample.n_cols == 1)
        {
        tmp2 = tmp1*trans(tmp1);
        }
      else
        {
        tmp2 = trans(tmp1)*tmp1;
        }

      x.r_cov *= (N_minus_1/N);
      x.r_cov += tmp2 / N_plus_1;
      }


    for(uword i=0; i<n_elem; ++i)
      {
      const eT val = sample_mem[i];

      if(val < min_val_mem[i])
        {
        min_val_mem[i] = val;
        }

      if(val > max_val_mem[i])
        {
        max_val_mem[i] = val;
        }

      const eT r_mean_val = r_mean_mem[i];
      const eT tmp        = val - r_mean_val;

      r_var_mem[i] = N_minus_1/N * r_var_mem[i] + (tmp*tmp)/N_plus_1;

      r_mean_mem[i] = r_mean_val + (val - r_mean_val)/N_plus_1;
      }
    }
  else
    {
    arma_debug_check( (sample.is_vec() == false), "running_stat_vec(): given sample is not a vector");

    x.r_mean.set_size(sample.n_rows, sample.n_cols);

    x.r_var.zeros(sample.n_rows, sample.n_cols);

    if(x.calc_cov == true)
      {
      x.r_cov.zeros(sample.n_elem, sample.n_elem);
      }

    x.min_val.set_size(sample.n_rows, sample.n_cols);
    x.max_val.set_size(sample.n_rows, sample.n_cols);


    const uword n_elem      = sample.n_elem;
    const eT*   sample_mem  = sample.memptr();
          eT*   r_mean_mem  = x.r_mean.memptr();
          eT*   min_val_mem = x.min_val.memptr();
          eT*   max_val_mem = x.max_val.memptr();


    for(uword i=0; i<n_elem; ++i)
      {
      const eT val = sample_mem[i];

       r_mean_mem[i] = val;
      min_val_mem[i] = val;
      max_val_mem[i] = val;
      }
    }

  x.counter++;
  }



//! update statistics to reflect new sample (version for non-complex numbers, complex sample)
template<typename obj_type>
inline
void
running_stat_vec_aux::update_stats
  (
  running_stat_vec<obj_type>& x,
  const          Mat<std::complex< typename running_stat_vec<obj_type>::T > >& sample,
  const typename       arma_not_cx<typename running_stat_vec<obj_type>::eT>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename running_stat_vec<obj_type>::eT eT;

  running_stat_vec_aux::update_stats(x, conv_to< Mat<eT> >::from(sample));
  }



//! update statistics to reflect new sample (version for complex numbers, non-complex sample)
template<typename obj_type>
inline
void
running_stat_vec_aux::update_stats
  (
  running_stat_vec<obj_type>& x,
  const                   Mat<typename running_stat_vec<obj_type>::T >& sample,
  const typename arma_cx_only<typename running_stat_vec<obj_type>::eT>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename running_stat_vec<obj_type>::eT eT;

  running_stat_vec_aux::update_stats(x, conv_to< Mat<eT> >::from(sample));
  }



//! alter statistics to reflect new sample (version for complex numbers, complex sample)
template<typename obj_type>
inline
void
running_stat_vec_aux::update_stats
  (
  running_stat_vec<obj_type>& x,
  const                   Mat<typename running_stat_vec<obj_type>::eT>& sample,
  const typename arma_cx_only<typename running_stat_vec<obj_type>::eT>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename running_stat_vec<obj_type>::eT eT;
  typedef typename running_stat_vec<obj_type>::T   T;

  const T N = x.counter.value();

  if(N > T(0))
    {
    arma_debug_assert_same_size(x.r_mean, sample, "running_stat_vec(): dimensionality mismatch");

    const uword n_elem           = sample.n_elem;
    const eT*   sample_mem       = sample.memptr();
          eT*   r_mean_mem       = x.r_mean.memptr();
           T*   r_var_mem        = x.r_var.memptr();
          eT*   min_val_mem      = x.min_val.memptr();
          eT*   max_val_mem      = x.max_val.memptr();
           T*   min_val_norm_mem = x.min_val_norm.memptr();
           T*   max_val_norm_mem = x.max_val_norm.memptr();

    const T  N_plus_1   = x.counter.value_plus_1();
    const T  N_minus_1  = x.counter.value_minus_1();

    if(x.calc_cov == true)
      {
      Mat<eT>& tmp1 = x.tmp1;
      Mat<eT>& tmp2 = x.tmp2;

      tmp1 = sample - x.r_mean;

      if(sample.n_cols == 1)
        {
        tmp2 = arma::conj(tmp1)*strans(tmp1);
        }
      else
        {
        tmp2 = trans(tmp1)*tmp1;  //tmp2 = strans(conj(tmp1))*tmp1;
        }

      x.r_cov *= (N_minus_1/N);
      x.r_cov += tmp2 / N_plus_1;
      }


    for(uword i=0; i<n_elem; ++i)
      {
      const eT& val      = sample_mem[i];
      const  T  val_norm = std::norm(val);

      if(val_norm < min_val_norm_mem[i])
        {
        min_val_norm_mem[i] = val_norm;
        min_val_mem[i]      = val;
        }

      if(val_norm > max_val_norm_mem[i])
        {
        max_val_norm_mem[i] = val_norm;
        max_val_mem[i]      = val;
        }

      const eT& r_mean_val = r_mean_mem[i];

      r_var_mem[i] = N_minus_1/N * r_var_mem[i] + std::norm(val - r_mean_val)/N_plus_1;

      r_mean_mem[i] = r_mean_val + (val - r_mean_val)/N_plus_1;
      }
    }
  else
    {
    arma_debug_check( (sample.is_vec() == false), "running_stat_vec(): given sample is not a vector");

    x.r_mean.set_size(sample.n_rows, sample.n_cols);

    x.r_var.zeros(sample.n_rows, sample.n_cols);

    if(x.calc_cov == true)
      {
      x.r_cov.zeros(sample.n_elem, sample.n_elem);
      }

    x.min_val.set_size(sample.n_rows, sample.n_cols);
    x.max_val.set_size(sample.n_rows, sample.n_cols);

    x.min_val_norm.set_size(sample.n_rows, sample.n_cols);
    x.max_val_norm.set_size(sample.n_rows, sample.n_cols);


    const uword n_elem           = sample.n_elem;
    const eT*   sample_mem       = sample.memptr();
          eT*   r_mean_mem       = x.r_mean.memptr();
          eT*   min_val_mem      = x.min_val.memptr();
          eT*   max_val_mem      = x.max_val.memptr();
           T*   min_val_norm_mem = x.min_val_norm.memptr();
           T*   max_val_norm_mem = x.max_val_norm.memptr();

    for(uword i=0; i<n_elem; ++i)
      {
      const eT& val      = sample_mem[i];
      const  T  val_norm = std::norm(val);

       r_mean_mem[i] = val;
      min_val_mem[i] = val;
      max_val_mem[i] = val;

      min_val_norm_mem[i] = val_norm;
      max_val_norm_mem[i] = val_norm;
      }
    }

  x.counter++;
  }



//! @}
