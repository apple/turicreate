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


//! \addtogroup gmm_full
//! @{


namespace gmm_priv
{


template<typename eT>
inline
gmm_full<eT>::~gmm_full()
  {
  arma_extra_debug_sigprint_this(this);

  arma_type_check(( (is_same_type<eT,float>::value == false) && (is_same_type<eT,double>::value == false) ));
  }



template<typename eT>
inline
gmm_full<eT>::gmm_full()
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename eT>
inline
gmm_full<eT>::gmm_full(const gmm_full<eT>& x)
  {
  arma_extra_debug_sigprint_this(this);

  init(x);
  }



template<typename eT>
inline
gmm_full<eT>&
gmm_full<eT>::operator=(const gmm_full<eT>& x)
  {
  arma_extra_debug_sigprint();

  init(x);

  return *this;
  }



template<typename eT>
inline
gmm_full<eT>::gmm_full(const gmm_diag<eT>& x)
  {
  arma_extra_debug_sigprint_this(this);

  init(x);
  }



template<typename eT>
inline
gmm_full<eT>&
gmm_full<eT>::operator=(const gmm_diag<eT>& x)
  {
  arma_extra_debug_sigprint();

  init(x);

  return *this;
  }



template<typename eT>
inline
gmm_full<eT>::gmm_full(const uword in_n_dims, const uword in_n_gaus)
  {
  arma_extra_debug_sigprint_this(this);

  init(in_n_dims, in_n_gaus);
  }



template<typename eT>
inline
void
gmm_full<eT>::reset()
  {
  arma_extra_debug_sigprint();

  init(0, 0);
  }



template<typename eT>
inline
void
gmm_full<eT>::reset(const uword in_n_dims, const uword in_n_gaus)
  {
  arma_extra_debug_sigprint();

  init(in_n_dims, in_n_gaus);
  }



template<typename eT>
template<typename T1, typename T2, typename T3>
inline
void
gmm_full<eT>::set_params(const Base<eT,T1>& in_means_expr, const BaseCube<eT,T2>& in_fcovs_expr, const Base<eT,T3>& in_hefts_expr)
  {
  arma_extra_debug_sigprint();

  const unwrap     <T1> tmp1(in_means_expr.get_ref());
  const unwrap_cube<T2> tmp2(in_fcovs_expr.get_ref());
  const unwrap     <T3> tmp3(in_hefts_expr.get_ref());

  const Mat <eT>& in_means = tmp1.M;
  const Cube<eT>& in_fcovs = tmp2.M;
  const Mat <eT>& in_hefts = tmp3.M;

  arma_debug_check
    (
    (in_means.n_cols != in_fcovs.n_slices) || (in_means.n_rows != in_fcovs.n_rows) || (in_fcovs.n_rows != in_fcovs.n_cols) || (in_hefts.n_cols != in_means.n_cols) || (in_hefts.n_rows != 1),
    "gmm_full::set_params(): given parameters have inconsistent and/or wrong sizes"
    );

  arma_debug_check( (in_means.is_finite() == false), "gmm_full::set_params(): given means have non-finite values" );
  arma_debug_check( (in_fcovs.is_finite() == false), "gmm_full::set_params(): given fcovs have non-finite values" );
  arma_debug_check( (in_hefts.is_finite() == false), "gmm_full::set_params(): given hefts have non-finite values" );

  for(uword g=0; g < in_fcovs.n_slices; ++g)
    {
    arma_debug_check( (any(diagvec(in_fcovs.slice(g)) <= eT(0))), "gmm_full::set_params(): given fcovs have negative or zero values on diagonals" );
    }

  arma_debug_check( (any(vectorise(in_hefts) <  eT(0))), "gmm_full::set_params(): given hefts have negative values" );

  const eT s = accu(in_hefts);

  arma_debug_check( ((s < (eT(1) - Datum<eT>::eps)) || (s > (eT(1) + Datum<eT>::eps))), "gmm_full::set_params(): sum of given hefts is not 1" );

  access::rw(means) = in_means;
  access::rw(fcovs) = in_fcovs;
  access::rw(hefts) = in_hefts;

  init_constants();
  }



template<typename eT>
template<typename T1>
inline
void
gmm_full<eT>::set_means(const Base<eT,T1>& in_means_expr)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in_means_expr.get_ref());

  const Mat<eT>& in_means = tmp.M;

  arma_debug_check( (size(in_means) != size(means)), "gmm_full::set_means(): given means have incompatible size" );
  arma_debug_check( (in_means.is_finite() == false), "gmm_full::set_means(): given means have non-finite values" );

  access::rw(means) = in_means;
  }



template<typename eT>
template<typename T1>
inline
void
gmm_full<eT>::set_fcovs(const BaseCube<eT,T1>& in_fcovs_expr)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in_fcovs_expr.get_ref());

  const Cube<eT>& in_fcovs = tmp.M;

  arma_debug_check( (size(in_fcovs) != size(fcovs)), "gmm_full::set_fcovs(): given fcovs have incompatible size" );
  arma_debug_check( (in_fcovs.is_finite() == false), "gmm_full::set_fcovs(): given fcovs have non-finite values" );

  for(uword i=0; i < in_fcovs.n_slices; ++i)
    {
    arma_debug_check( (any(diagvec(in_fcovs.slice(i)) <= eT(0))), "gmm_full::set_fcovs(): given fcovs have negative or zero values on diagonals" );
    }

  access::rw(fcovs) = in_fcovs;

  init_constants();
  }



template<typename eT>
template<typename T1>
inline
void
gmm_full<eT>::set_hefts(const Base<eT,T1>& in_hefts_expr)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in_hefts_expr.get_ref());

  const Mat<eT>& in_hefts = tmp.M;

  arma_debug_check( (size(in_hefts) != size(hefts)),     "gmm_full::set_hefts(): given hefts have incompatible size" );
  arma_debug_check( (in_hefts.is_finite() == false),     "gmm_full::set_hefts(): given hefts have non-finite values" );
  arma_debug_check( (any(vectorise(in_hefts) <  eT(0))), "gmm_full::set_hefts(): given hefts have negative values"   );

  const eT s = accu(in_hefts);

  arma_debug_check( ((s < (eT(1) - Datum<eT>::eps)) || (s > (eT(1) + Datum<eT>::eps))), "gmm_full::set_hefts(): sum of given hefts is not 1" );

  // make sure all hefts are positive and non-zero

  const eT* in_hefts_mem = in_hefts.memptr();
        eT*    hefts_mem = access::rw(hefts).memptr();

  for(uword i=0; i < hefts.n_elem; ++i)
    {
    hefts_mem[i] = (std::max)( in_hefts_mem[i], std::numeric_limits<eT>::min() );
    }

  access::rw(hefts) /= accu(hefts);

  log_hefts = log(hefts);
  }



template<typename eT>
inline
uword
gmm_full<eT>::n_dims() const
  {
  return means.n_rows;
  }



template<typename eT>
inline
uword
gmm_full<eT>::n_gaus() const
  {
  return means.n_cols;
  }



template<typename eT>
inline
bool
gmm_full<eT>::load(const std::string name)
  {
  arma_extra_debug_sigprint();

  field< Mat<eT> > storage;

  bool status = storage.load(name, arma_binary);

  if( (status == false) || (storage.n_elem < 2) )
    {
    reset();
    arma_debug_warn("gmm_full::load(): problem with loading or incompatible format");
    return false;
    }

  uword count = 0;

  const Mat<eT>& storage_means = storage(count);  ++count;
  const Mat<eT>& storage_hefts = storage(count);  ++count;

  const uword N_dims = storage_means.n_rows;
  const uword N_gaus = storage_means.n_cols;

  if( (storage.n_elem != (N_gaus + 2)) || (storage_hefts.n_rows != 1) || (storage_hefts.n_cols != N_gaus) )
    {
    reset();
    arma_debug_warn("gmm_full::load(): incompatible format");
    return false;
    }

  reset(N_dims, N_gaus);

  access::rw(means) = storage_means;
  access::rw(hefts) = storage_hefts;

  for(uword g=0; g < N_gaus; ++g)
    {
    const Mat<eT>& storage_fcov = storage(count);  ++count;

    if( (storage_fcov.n_rows != N_dims) || (storage_fcov.n_cols != N_dims) )
      {
      reset();
      arma_debug_warn("gmm_full::load(): incompatible format");
      return false;
      }

    access::rw(fcovs).slice(g) = storage_fcov;
    }

  init_constants();

  return true;
  }



template<typename eT>
inline
bool
gmm_full<eT>::save(const std::string name) const
  {
  arma_extra_debug_sigprint();

  const uword N_gaus = means.n_cols;

  field< Mat<eT> > storage(2 + N_gaus);

  uword count = 0;

  storage(count) = means;  ++count;
  storage(count) = hefts;  ++count;

  for(uword g=0; g < N_gaus; ++g)
    {
    storage(count) = fcovs.slice(g);  ++count;
    }

  const bool status = storage.save(name, arma_binary);

  return status;
  }



template<typename eT>
inline
Col<eT>
gmm_full<eT>::generate() const
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  Col<eT> out( (N_gaus > 0) ? N_dims : uword(0)              );
  Col<eT> tmp( (N_gaus > 0) ? N_dims : uword(0), fill::randn );

  if(N_gaus > 0)
    {
    const double val = randu<double>();

    double csum    = double(0);
    uword  gaus_id = 0;

    for(uword j=0; j < N_gaus; ++j)
      {
      csum += hefts[j];

      if(val <= csum)  { gaus_id = j; break; }
      }

    out  = chol_fcovs.slice(gaus_id) * tmp;
    out += means.col(gaus_id);
    }

  return out;
  }



template<typename eT>
inline
Mat<eT>
gmm_full<eT>::generate(const uword N_vec) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  Mat<eT> out( ( (N_gaus > 0) ? N_dims : uword(0) ), N_vec              );
  Mat<eT> tmp( ( (N_gaus > 0) ? N_dims : uword(0) ), N_vec, fill::randn );

  if(N_gaus > 0)
    {
    const eT* hefts_mem = hefts.memptr();

    for(uword i=0; i < N_vec; ++i)
      {
      const double val = randu<double>();

      double csum    = double(0);
      uword  gaus_id = 0;

      for(uword j=0; j < N_gaus; ++j)
        {
        csum += hefts_mem[j];

        if(val <= csum)  { gaus_id = j; break; }
        }

      Col<eT> out_vec(out.colptr(i), N_dims, false, true);
      Col<eT> tmp_vec(tmp.colptr(i), N_dims, false, true);

      out_vec  = chol_fcovs.slice(gaus_id) * tmp_vec;
      out_vec += means.col(gaus_id);
      }
    }

  return out;
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::log_p(const T1& expr, const gmm_empty_arg& junk1, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == true))>::result* junk2) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  const uword N_dims = means.n_rows;

  const quasi_unwrap<T1> U(expr);

  arma_debug_check( (U.M.n_rows != N_dims), "gmm_full::log_p(): incompatible dimensions" );

  return internal_scalar_log_p( U.M.memptr() );
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::log_p(const T1& expr, const uword gaus_id, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == true))>::result* junk2) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk2);

  const uword N_dims = means.n_rows;

  const quasi_unwrap<T1> U(expr);

  arma_debug_check( (U.M.n_rows != N_dims),    "gmm_full::log_p(): incompatible dimensions"            );
  arma_debug_check( (gaus_id >= means.n_cols), "gmm_full::log_p(): specified gaussian is out of range" );

  return internal_scalar_log_p( U.M.memptr(), gaus_id );
  }



template<typename eT>
template<typename T1>
inline
Row<eT>
gmm_full<eT>::log_p(const T1& expr, const gmm_empty_arg& junk1, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == false))>::result* junk2) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >(expr);

    return internal_vec_log_p(X);
    }
  else
    {
    const unwrap<T1>   tmp(expr);
    const Mat<eT>& X = tmp.M;

    return internal_vec_log_p(X);
    }
  }



template<typename eT>
template<typename T1>
inline
Row<eT>
gmm_full<eT>::log_p(const T1& expr, const uword gaus_id, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == false))>::result* junk2) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk2);

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >(expr);

    return internal_vec_log_p(X, gaus_id);
    }
  else
    {
    const unwrap<T1>   tmp(expr);
    const Mat<eT>& X = tmp.M;

    return internal_vec_log_p(X, gaus_id);
    }
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::sum_log_p(const Base<eT,T1>& expr) const
  {
  arma_extra_debug_sigprint();

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >( expr.get_ref() );

    return internal_sum_log_p(X);
    }
  else
    {
    const unwrap<T1>   tmp(expr.get_ref());
    const Mat<eT>& X = tmp.M;

    return internal_sum_log_p(X);
    }
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::sum_log_p(const Base<eT,T1>& expr, const uword gaus_id) const
  {
  arma_extra_debug_sigprint();

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >( expr.get_ref() );

    return internal_sum_log_p(X, gaus_id);
    }
  else
    {
    const unwrap<T1>   tmp(expr.get_ref());
    const Mat<eT>& X = tmp.M;

    return internal_sum_log_p(X, gaus_id);
    }
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::avg_log_p(const Base<eT,T1>& expr) const
  {
  arma_extra_debug_sigprint();

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >( expr.get_ref() );

    return internal_avg_log_p(X);
    }
  else
    {
    const unwrap<T1>   tmp(expr.get_ref());
    const Mat<eT>& X = tmp.M;

    return internal_avg_log_p(X);
    }
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::avg_log_p(const Base<eT,T1>& expr, const uword gaus_id) const
  {
  arma_extra_debug_sigprint();

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >( expr.get_ref() );

    return internal_avg_log_p(X, gaus_id);
    }
  else
    {
    const unwrap<T1>   tmp(expr.get_ref());
    const Mat<eT>& X = tmp.M;

    return internal_avg_log_p(X, gaus_id);
    }
  }



template<typename eT>
template<typename T1>
inline
uword
gmm_full<eT>::assign(const T1& expr, const gmm_dist_mode& dist, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == true))>::result* junk) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  if(is_subview_col<T1>::value)
    {
    const subview_col<eT>& X = reinterpret_cast< const subview_col<eT>& >(expr);

    return internal_scalar_assign(X, dist);
    }
  else
    {
    const unwrap<T1>   tmp(expr);
    const Mat<eT>& X = tmp.M;

    return internal_scalar_assign(X, dist);
    }
  }



template<typename eT>
template<typename T1>
inline
urowvec
gmm_full<eT>::assign(const T1& expr, const gmm_dist_mode& dist, typename enable_if<((is_arma_type<T1>::value) && (resolves_to_colvector<T1>::value == false))>::result* junk) const
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  urowvec out;

  if(is_subview<T1>::value)
    {
    const subview<eT>& X = reinterpret_cast< const subview<eT>& >(expr);

    internal_vec_assign(out, X, dist);
    }
  else
    {
    const unwrap<T1>   tmp(expr);
    const Mat<eT>& X = tmp.M;

    internal_vec_assign(out, X, dist);
    }

  return out;
  }



template<typename eT>
template<typename T1>
inline
urowvec
gmm_full<eT>::raw_hist(const Base<eT,T1>& expr, const gmm_dist_mode& dist_mode) const
  {
  arma_extra_debug_sigprint();

  const unwrap<T1>   tmp(expr.get_ref());
  const Mat<eT>& X = tmp.M;

  arma_debug_check( (X.n_rows != means.n_rows), "gmm_full::raw_hist(): incompatible dimensions" );

  arma_debug_check( ((dist_mode != eucl_dist) && (dist_mode != prob_dist)), "gmm_full::raw_hist(): unsupported distance mode" );

  urowvec hist;

  internal_raw_hist(hist, X, dist_mode);

  return hist;
  }



template<typename eT>
template<typename T1>
inline
Row<eT>
gmm_full<eT>::norm_hist(const Base<eT,T1>& expr, const gmm_dist_mode& dist_mode) const
  {
  arma_extra_debug_sigprint();

  const unwrap<T1>   tmp(expr.get_ref());
  const Mat<eT>& X = tmp.M;

  arma_debug_check( (X.n_rows != means.n_rows), "gmm_full::norm_hist(): incompatible dimensions" );

  arma_debug_check( ((dist_mode != eucl_dist) && (dist_mode != prob_dist)), "gmm_full::norm_hist(): unsupported distance mode" );

  urowvec hist;

  internal_raw_hist(hist, X, dist_mode);

  const uword  hist_n_elem = hist.n_elem;
  const uword* hist_mem    = hist.memptr();

  eT acc = eT(0);
  for(uword i=0; i<hist_n_elem; ++i)  { acc += eT(hist_mem[i]); }

  if(acc == eT(0))  { acc = eT(1); }

  Row<eT> out(hist_n_elem);

  eT* out_mem = out.memptr();

  for(uword i=0; i<hist_n_elem; ++i)  { out_mem[i] = eT(hist_mem[i]) / acc; }

  return out;
  }



template<typename eT>
template<typename T1>
inline
bool
gmm_full<eT>::learn
  (
  const Base<eT,T1>&   data,
  const uword          N_gaus,
  const gmm_dist_mode& dist_mode,
  const gmm_seed_mode& seed_mode,
  const uword          km_iter,
  const uword          em_iter,
  const eT             var_floor,
  const bool           print_mode
  )
  {
  arma_extra_debug_sigprint();

  const bool dist_mode_ok = (dist_mode == eucl_dist) || (dist_mode == maha_dist);

  const bool seed_mode_ok = \
       (seed_mode == keep_existing)
    || (seed_mode == static_subset)
    || (seed_mode == static_spread)
    || (seed_mode == random_subset)
    || (seed_mode == random_spread);

  arma_debug_check( (dist_mode_ok == false), "gmm_full::learn(): dist_mode must be eucl_dist or maha_dist" );
  arma_debug_check( (seed_mode_ok == false), "gmm_full::learn(): unknown seed_mode"                        );
  arma_debug_check( (var_floor < eT(0)    ), "gmm_full::learn(): variance floor is negative"               );

  const unwrap<T1>   tmp_X(data.get_ref());
  const Mat<eT>& X = tmp_X.M;

  if(X.is_empty()          )  { arma_debug_warn("gmm_full::learn(): given matrix is empty"             ); return false; }
  if(X.is_finite() == false)  { arma_debug_warn("gmm_full::learn(): given matrix has non-finite values"); return false; }

  if(N_gaus == 0)  { reset(); return true; }

  if(dist_mode == maha_dist)
    {
    mah_aux = var(X,1,1);

    const uword mah_aux_n_elem = mah_aux.n_elem;
          eT*   mah_aux_mem    = mah_aux.memptr();

    for(uword i=0; i < mah_aux_n_elem; ++i)
      {
      const eT val = mah_aux_mem[i];

      mah_aux_mem[i] = ((val != eT(0)) && arma_isfinite(val)) ? eT(1) / val : eT(1);
      }
    }


  // copy current model, in case of failure by k-means and/or EM

  const gmm_full<eT> orig = (*this);


  // initial means

  if(seed_mode == keep_existing)
    {
    if(means.is_empty()        )  { arma_debug_warn("gmm_full::learn(): no existing means"      ); return false; }
    if(X.n_rows != means.n_rows)  { arma_debug_warn("gmm_full::learn(): dimensionality mismatch"); return false; }

    // TODO: also check for number of vectors?
    }
  else
    {
    if(X.n_cols < N_gaus)  { arma_debug_warn("gmm_full::learn(): number of vectors is less than number of gaussians"); return false; }

    reset(X.n_rows, N_gaus);

    if(print_mode)  { get_cout_stream() << "gmm_full::learn(): generating initial means\n"; get_cout_stream().flush(); }

         if(dist_mode == eucl_dist)  { generate_initial_means<1>(X, seed_mode); }
    else if(dist_mode == maha_dist)  { generate_initial_means<2>(X, seed_mode); }
    }


  // k-means

  if(km_iter > 0)
    {
    const arma_ostream_state stream_state(get_cout_stream());

    bool status = false;

         if(dist_mode == eucl_dist)  { status = km_iterate<1>(X, km_iter, print_mode); }
    else if(dist_mode == maha_dist)  { status = km_iterate<2>(X, km_iter, print_mode); }

    stream_state.restore(get_cout_stream());

    if(status == false)  { arma_debug_warn("gmm_full::learn(): k-means algorithm failed; not enough data, or too many gaussians requested"); init(orig); return false; }
    }


  // initial fcovs

  const eT var_floor_actual = (eT(var_floor) > eT(0)) ? eT(var_floor) : std::numeric_limits<eT>::min();

  if(seed_mode != keep_existing)
    {
    if(print_mode)  { get_cout_stream() << "gmm_full::learn(): generating initial covariances\n"; get_cout_stream().flush(); }

         if(dist_mode == eucl_dist)  { generate_initial_params<1>(X, var_floor_actual); }
    else if(dist_mode == maha_dist)  { generate_initial_params<2>(X, var_floor_actual); }
    }


  // EM algorithm

  if(em_iter > 0)
    {
    const arma_ostream_state stream_state(get_cout_stream());

    const bool status = em_iterate(X, em_iter, var_floor_actual, print_mode);

    stream_state.restore(get_cout_stream());

    if(status == false)  { arma_debug_warn("gmm_full::learn(): EM algorithm failed"); init(orig); return false; }
    }

  mah_aux.reset();

  init_constants();

  return true;
  }



//
//
//



template<typename eT>
inline
void
gmm_full<eT>::init(const gmm_full<eT>& x)
  {
  arma_extra_debug_sigprint();

  gmm_full<eT>& t = *this;

  if(&t != &x)
    {
    access::rw(t.means) = x.means;
    access::rw(t.fcovs) = x.fcovs;
    access::rw(t.hefts) = x.hefts;

    init_constants();
    }
  }



template<typename eT>
inline
void
gmm_full<eT>::init(const gmm_diag<eT>& x)
  {
  arma_extra_debug_sigprint();

  access::rw(hefts) = x.hefts;
  access::rw(means) = x.means;

  const uword N_dims = x.means.n_rows;
  const uword N_gaus = x.means.n_cols;

  access::rw(fcovs).zeros(N_dims,N_dims,N_gaus);

  for(uword g=0; g < N_gaus; ++g)
    {
    Mat<eT>& fcov = access::rw(fcovs).slice(g);

    const eT* dcov_mem = x.dcovs.colptr(g);

    for(uword d=0; d < N_dims; ++d)
      {
      fcov.at(d,d) = dcov_mem[d];
      }
    }

  init_constants();
  }



template<typename eT>
inline
void
gmm_full<eT>::init(const uword in_n_dims, const uword in_n_gaus)
  {
  arma_extra_debug_sigprint();

  access::rw(means).zeros(in_n_dims, in_n_gaus);

  access::rw(fcovs).zeros(in_n_dims, in_n_dims, in_n_gaus);

  for(uword g=0; g < in_n_gaus; ++g)
    {
    access::rw(fcovs).slice(g).diag().ones();
    }

  access::rw(hefts).set_size(in_n_gaus);
  access::rw(hefts).fill(eT(1) / eT(in_n_gaus));

  init_constants();
  }



template<typename eT>
inline
void
gmm_full<eT>::init_constants(const bool calc_chol)
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const eT tmp = (eT(N_dims)/eT(2)) * std::log(eT(2) * Datum<eT>::pi);

  //

  inv_fcovs.copy_size(fcovs);
  log_det_etc.set_size(N_gaus);

  Mat<eT> tmp_inv;

  for(uword g=0; g < N_gaus; ++g)
    {
    const Mat<eT>&     fcov =      fcovs.slice(g);
          Mat<eT>& inv_fcov =  inv_fcovs.slice(g);

  //const bool inv_ok = auxlib::inv(tmp_inv, fcov);
    const bool inv_ok = auxlib::inv_sympd(tmp_inv, fcov);

    eT log_det_val  = eT(0);
    eT log_det_sign = eT(0);

    log_det(log_det_val, log_det_sign, fcov);

    const bool log_det_ok = ( (arma_isfinite(log_det_val)) && (log_det_sign > eT(0)) );

    if(inv_ok && log_det_ok)
      {
      inv_fcov = tmp_inv;
      }
    else
      {
      inv_fcov.zeros();

      log_det_val = eT(0);

      for(uword d=0; d < N_dims; ++d)
        {
        const eT sanitised_val = (std::max)( eT(fcov.at(d,d)), eT(std::numeric_limits<eT>::min()) );

        inv_fcov.at(d,d) = eT(1) / sanitised_val;

        log_det_val += std::log(sanitised_val);
        }
      }

    log_det_etc[g] = eT(-1) * ( tmp + eT(0.5) * log_det_val );
    }

  //

  eT* hefts_mem = access::rw(hefts).memptr();

  for(uword g=0; g < N_gaus; ++g)
    {
    hefts_mem[g] = (std::max)( hefts_mem[g], std::numeric_limits<eT>::min() );
    }

  log_hefts = log(hefts);


  if(calc_chol)
    {
    chol_fcovs.copy_size(fcovs);

    Mat<eT> tmp_chol;

    for(uword g=0; g < N_gaus; ++g)
      {
      const Mat<eT>& fcov      =      fcovs.slice(g);
            Mat<eT>& chol_fcov = chol_fcovs.slice(g);

      const uword chol_layout = 1;  // indicates "lower"

      const bool chol_ok = auxlib::chol(tmp_chol, fcov, chol_layout);

      if(chol_ok)
        {
        chol_fcov = tmp_chol;
        }
      else
        {
        chol_fcov.zeros();

        for(uword d=0; d < N_dims; ++d)
          {
          const eT sanitised_val = (std::max)( eT(fcov.at(d,d)), eT(std::numeric_limits<eT>::min()) );

          chol_fcov.at(d,d) = std::sqrt(sanitised_val);
          }
        }
      }
    }
  }



template<typename eT>
inline
umat
gmm_full<eT>::internal_gen_boundaries(const uword N) const
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_OPENMP)
    const uword n_threads_avail = uword(omp_get_max_threads());
    const uword n_threads       = (n_threads_avail > 0) ? ( (n_threads_avail <= N) ? n_threads_avail : 1 ) : 1;
  #else
    static const uword n_threads = 1;
  #endif

  // get_cout_stream() << "gmm_full::internal_gen_boundaries(): n_threads: " << n_threads << '\n';

  umat boundaries(2, n_threads);

  if(N > 0)
    {
    const uword chunk_size = N / n_threads;

    uword count = 0;

    for(uword t=0; t<n_threads; t++)
      {
      boundaries.at(0,t) = count;

      count += chunk_size;

      boundaries.at(1,t) = count-1;
      }

    boundaries.at(1,n_threads-1) = N - 1;
    }
  else
    {
    boundaries.zeros();
    }

  // get_cout_stream() << "gmm_full::internal_gen_boundaries(): boundaries: " << '\n' << boundaries << '\n';

  return boundaries;
  }



template<typename eT>
inline
eT
gmm_full<eT>::internal_scalar_log_p(const eT* x) const
  {
  arma_extra_debug_sigprint();

  const eT* log_hefts_mem = log_hefts.mem;

  const uword N_gaus = means.n_cols;

  if(N_gaus > 0)
    {
    eT log_sum = internal_scalar_log_p(x, 0) + log_hefts_mem[0];

    for(uword g=1; g < N_gaus; ++g)
      {
      const eT log_val = internal_scalar_log_p(x, g) + log_hefts_mem[g];

      log_sum = log_add_exp(log_sum, log_val);
      }

    return log_sum;
    }
  else
    {
    return -Datum<eT>::inf;
    }
  }



template<typename eT>
inline
eT
gmm_full<eT>::internal_scalar_log_p(const eT* x, const uword g) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims   = means.n_rows;
  const eT*   mean_mem = means.colptr(g);

  eT outer_acc = eT(0);

  const eT* inv_fcov_coldata = inv_fcovs.slice(g).memptr();

  for(uword i=0; i < N_dims; ++i)
    {
    eT inner_acc = eT(0);

    for(uword j=0; j < N_dims; ++j)
      {
      inner_acc += (x[j] - mean_mem[j]) * inv_fcov_coldata[j];
      }

    inv_fcov_coldata += N_dims;

    outer_acc += inner_acc * (x[i] - mean_mem[i]);
    }

  return eT(-0.5)*outer_acc + log_det_etc.mem[g];
  }



template<typename eT>
template<typename T1>
inline
Row<eT>
gmm_full<eT>::internal_vec_log_p(const T1& X) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims    = means.n_rows;
  const uword N_samples = X.n_cols;

  arma_debug_check( (X.n_rows != N_dims), "gmm_full::log_p(): incompatible dimensions" );

  Row<eT> out(N_samples);

  if(N_samples > 0)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      const umat boundaries = internal_gen_boundaries(N_samples);

      const uword n_threads = boundaries.n_cols;

      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        eT* out_mem = out.memptr();

        for(uword i=start_index; i <= end_index; ++i)
          {
          out_mem[i] = internal_scalar_log_p( X.colptr(i) );
          }
        }
      }
    #else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i < N_samples; ++i)
        {
        out_mem[i] = internal_scalar_log_p( X.colptr(i) );
        }
      }
    #endif
    }

  return out;
  }



template<typename eT>
template<typename T1>
inline
Row<eT>
gmm_full<eT>::internal_vec_log_p(const T1& X, const uword gaus_id) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims    = means.n_rows;
  const uword N_samples = X.n_cols;

  arma_debug_check( (X.n_rows != N_dims),       "gmm_full::log_p(): incompatible dimensions"            );
  arma_debug_check( (gaus_id  >= means.n_cols), "gmm_full::log_p(): specified gaussian is out of range" );

  Row<eT> out(N_samples);

  if(N_samples > 0)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      const umat boundaries = internal_gen_boundaries(N_samples);

      const uword n_threads = boundaries.n_cols;

      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        eT* out_mem = out.memptr();

        for(uword i=start_index; i <= end_index; ++i)
          {
          out_mem[i] = internal_scalar_log_p( X.colptr(i), gaus_id );
          }
        }
      }
    #else
      {
      eT* out_mem = out.memptr();

      for(uword i=0; i < N_samples; ++i)
        {
        out_mem[i] = internal_scalar_log_p( X.colptr(i), gaus_id );
        }
      }
    #endif
    }

  return out;
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::internal_sum_log_p(const T1& X) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (X.n_rows != means.n_rows), "gmm_full::sum_log_p(): incompatible dimensions" );

  const uword N = X.n_cols;

  if(N == 0)  { return (-Datum<eT>::inf); }


  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(N);

    const uword n_threads = boundaries.n_cols;

    Col<eT> t_accs(n_threads, fill::zeros);

    #pragma omp parallel for schedule(static)
    for(uword t=0; t < n_threads; ++t)
      {
      const uword start_index = boundaries.at(0,t);
      const uword   end_index = boundaries.at(1,t);

      eT t_acc = eT(0);

      for(uword i=start_index; i <= end_index; ++i)
        {
        t_acc += internal_scalar_log_p( X.colptr(i) );
        }

      t_accs[t] = t_acc;
      }

    return eT(accu(t_accs));
    }
  #else
    {
    eT acc = eT(0);

    for(uword i=0; i<N; ++i)
      {
      acc += internal_scalar_log_p( X.colptr(i) );
      }

    return acc;
    }
  #endif
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::internal_sum_log_p(const T1& X, const uword gaus_id) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (X.n_rows != means.n_rows), "gmm_full::sum_log_p(): incompatible dimensions"            );
  arma_debug_check( (gaus_id  >= means.n_cols), "gmm_full::sum_log_p(): specified gaussian is out of range" );

  const uword N = X.n_cols;

  if(N == 0)  { return (-Datum<eT>::inf); }


  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(N);

    const uword n_threads = boundaries.n_cols;

    Col<eT> t_accs(n_threads, fill::zeros);

    #pragma omp parallel for schedule(static)
    for(uword t=0; t < n_threads; ++t)
      {
      const uword start_index = boundaries.at(0,t);
      const uword   end_index = boundaries.at(1,t);

      eT t_acc = eT(0);

      for(uword i=start_index; i <= end_index; ++i)
        {
        t_acc += internal_scalar_log_p( X.colptr(i), gaus_id );
        }

      t_accs[t] = t_acc;
      }

    return eT(accu(t_accs));
    }
  #else
    {
    eT acc = eT(0);

    for(uword i=0; i<N; ++i)
      {
      acc += internal_scalar_log_p( X.colptr(i), gaus_id );
      }

    return acc;
    }
  #endif
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::internal_avg_log_p(const T1& X) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims    = means.n_rows;
  const uword N_samples = X.n_cols;

  arma_debug_check( (X.n_rows != N_dims), "gmm_full::avg_log_p(): incompatible dimensions" );

  if(N_samples == 0)  { return (-Datum<eT>::inf); }


  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(N_samples);

    const uword n_threads = boundaries.n_cols;

    field< running_mean_scalar<eT> > t_running_means(n_threads);


    #pragma omp parallel for schedule(static)
    for(uword t=0; t < n_threads; ++t)
      {
      const uword start_index = boundaries.at(0,t);
      const uword   end_index = boundaries.at(1,t);

      running_mean_scalar<eT>& current_running_mean = t_running_means[t];

      for(uword i=start_index; i <= end_index; ++i)
        {
        current_running_mean( internal_scalar_log_p( X.colptr(i) ) );
        }
      }


    eT avg = eT(0);

    for(uword t=0; t < n_threads; ++t)
      {
      running_mean_scalar<eT>& current_running_mean = t_running_means[t];

      const eT w = eT(current_running_mean.count()) / eT(N_samples);

      avg += w * current_running_mean.mean();
      }

    return avg;
    }
  #else
    {
    running_mean_scalar<eT> running_mean;

    for(uword i=0; i < N_samples; ++i)
      {
      running_mean( internal_scalar_log_p( X.colptr(i) ) );
      }

    return running_mean.mean();
    }
  #endif
  }



template<typename eT>
template<typename T1>
inline
eT
gmm_full<eT>::internal_avg_log_p(const T1& X, const uword gaus_id) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims    = means.n_rows;
  const uword N_samples = X.n_cols;

  arma_debug_check( (X.n_rows != N_dims),       "gmm_full::avg_log_p(): incompatible dimensions"            );
  arma_debug_check( (gaus_id  >= means.n_cols), "gmm_full::avg_log_p(): specified gaussian is out of range" );

  if(N_samples == 0)  { return (-Datum<eT>::inf); }


  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(N_samples);

    const uword n_threads = boundaries.n_cols;

    field< running_mean_scalar<eT> > t_running_means(n_threads);


    #pragma omp parallel for schedule(static)
    for(uword t=0; t < n_threads; ++t)
      {
      const uword start_index = boundaries.at(0,t);
      const uword   end_index = boundaries.at(1,t);

      running_mean_scalar<eT>& current_running_mean = t_running_means[t];

      for(uword i=start_index; i <= end_index; ++i)
        {
        current_running_mean( internal_scalar_log_p( X.colptr(i), gaus_id) );
        }
      }


    eT avg = eT(0);

    for(uword t=0; t < n_threads; ++t)
      {
      running_mean_scalar<eT>& current_running_mean = t_running_means[t];

      const eT w = eT(current_running_mean.count()) / eT(N_samples);

      avg += w * current_running_mean.mean();
      }

    return avg;
    }
  #else
    {
    running_mean_scalar<eT> running_mean;

    for(uword i=0; i<N_samples; ++i)
      {
      running_mean( internal_scalar_log_p( X.colptr(i), gaus_id ) );
      }

    return running_mean.mean();
    }
  #endif
  }



template<typename eT>
template<typename T1>
inline
uword
gmm_full<eT>::internal_scalar_assign(const T1& X, const gmm_dist_mode& dist_mode) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  arma_debug_check( (X.n_rows != N_dims), "gmm_full::assign(): incompatible dimensions" );
  arma_debug_check( (N_gaus == 0),        "gmm_full::assign(): model has no means"      );

  const eT* X_mem = X.colptr(0);

  if(dist_mode == eucl_dist)
    {
    eT    best_dist = Datum<eT>::inf;
    uword best_g    = 0;

    for(uword g=0; g < N_gaus; ++g)
      {
      const eT tmp_dist = distance<eT,1>::eval(N_dims, X_mem, means.colptr(g), X_mem);

      if(tmp_dist <= best_dist)
        {
        best_dist = tmp_dist;
        best_g    = g;
        }
      }

    return best_g;
    }
  else
  if(dist_mode == prob_dist)
    {
    const eT* log_hefts_mem = log_hefts.memptr();

    eT    best_p = -Datum<eT>::inf;
    uword best_g = 0;

    for(uword g=0; g < N_gaus; ++g)
      {
      const eT tmp_p = internal_scalar_log_p(X_mem, g) + log_hefts_mem[g];

      if(tmp_p >= best_p)
        {
        best_p = tmp_p;
        best_g = g;
        }
      }

    return best_g;
    }
  else
    {
    arma_debug_check(true, "gmm_full::assign(): unsupported distance mode");
    }

  return uword(0);
  }



template<typename eT>
template<typename T1>
inline
void
gmm_full<eT>::internal_vec_assign(urowvec& out, const T1& X, const gmm_dist_mode& dist_mode) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  arma_debug_check( (X.n_rows != N_dims), "gmm_full::assign(): incompatible dimensions" );

  const uword X_n_cols = (N_gaus > 0) ? X.n_cols : 0;

  out.set_size(1,X_n_cols);

  uword* out_mem = out.memptr();

  if(dist_mode == eucl_dist)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      #pragma omp parallel for schedule(static)
      for(uword i=0; i<X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT    best_dist = Datum<eT>::inf;
        uword best_g    = 0;

        for(uword g=0; g<N_gaus; ++g)
          {
          const eT tmp_dist = distance<eT,1>::eval(N_dims, X_colptr, means.colptr(g), X_colptr);

          if(tmp_dist <= best_dist)  { best_dist = tmp_dist; best_g = g; }
          }

        out_mem[i] = best_g;
        }
      }
    #else
      {
      for(uword i=0; i<X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT    best_dist = Datum<eT>::inf;
        uword best_g    = 0;

        for(uword g=0; g<N_gaus; ++g)
          {
          const eT tmp_dist = distance<eT,1>::eval(N_dims, X_colptr, means.colptr(g), X_colptr);

          if(tmp_dist <= best_dist)  { best_dist = tmp_dist; best_g = g; }
          }

        out_mem[i] = best_g;
        }
      }
    #endif
    }
  else
  if(dist_mode == prob_dist)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      const umat boundaries = internal_gen_boundaries(X_n_cols);

      const uword n_threads = boundaries.n_cols;

      const eT* log_hefts_mem = log_hefts.memptr();

      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        for(uword i=start_index; i <= end_index; ++i)
          {
          const eT* X_colptr = X.colptr(i);

          eT    best_p = -Datum<eT>::inf;
          uword best_g = 0;

          for(uword g=0; g<N_gaus; ++g)
            {
            const eT tmp_p = internal_scalar_log_p(X_colptr, g) + log_hefts_mem[g];

            if(tmp_p >= best_p)  { best_p = tmp_p; best_g = g; }
            }

          out_mem[i] = best_g;
          }
        }
      }
    #else
      {
      const eT* log_hefts_mem = log_hefts.memptr();

      for(uword i=0; i<X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT    best_p = -Datum<eT>::inf;
        uword best_g = 0;

        for(uword g=0; g<N_gaus; ++g)
          {
          const eT tmp_p = internal_scalar_log_p(X_colptr, g) + log_hefts_mem[g];

          if(tmp_p >= best_p)  { best_p = tmp_p; best_g = g; }
          }

        out_mem[i] = best_g;
        }
      }
    #endif
    }
  else
    {
    arma_debug_check(true, "gmm_full::assign(): unsupported distance mode");
    }
  }




template<typename eT>
inline
void
gmm_full<eT>::internal_raw_hist(urowvec& hist, const Mat<eT>& X, const gmm_dist_mode& dist_mode) const
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const uword X_n_cols = X.n_cols;

  hist.zeros(N_gaus);

  if(N_gaus == 0)  { return; }

  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(X_n_cols);

    const uword n_threads = boundaries.n_cols;

    field<urowvec> thread_hist(n_threads);

    for(uword t=0; t < n_threads; ++t)  { thread_hist(t).zeros(N_gaus); }


    if(dist_mode == eucl_dist)
      {
      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        uword* thread_hist_mem = thread_hist(t).memptr();

        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        for(uword i=start_index; i <= end_index; ++i)
          {
          const eT* X_colptr = X.colptr(i);

          eT    best_dist = Datum<eT>::inf;
          uword best_g    = 0;

          for(uword g=0; g < N_gaus; ++g)
            {
            const eT tmp_dist = distance<eT,1>::eval(N_dims, X_colptr, means.colptr(g), X_colptr);

            if(tmp_dist <= best_dist)  { best_dist = tmp_dist; best_g = g; }
            }

          thread_hist_mem[best_g]++;
          }
        }
      }
    else
    if(dist_mode == prob_dist)
      {
      const eT* log_hefts_mem = log_hefts.memptr();

      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        uword* thread_hist_mem = thread_hist(t).memptr();

        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        for(uword i=start_index; i <= end_index; ++i)
          {
          const eT* X_colptr = X.colptr(i);

          eT    best_p = -Datum<eT>::inf;
          uword best_g = 0;

          for(uword g=0; g < N_gaus; ++g)
            {
            const eT tmp_p = internal_scalar_log_p(X_colptr, g) + log_hefts_mem[g];

            if(tmp_p >= best_p)  { best_p = tmp_p; best_g = g; }
            }

          thread_hist_mem[best_g]++;
          }
        }
      }

    // reduction
    for(uword t=0; t < n_threads; ++t)
      {
      hist += thread_hist(t);
      }
    }
  #else
    {
    uword* hist_mem = hist.memptr();

    if(dist_mode == eucl_dist)
      {
      for(uword i=0; i<X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT    best_dist = Datum<eT>::inf;
        uword best_g    = 0;

        for(uword g=0; g < N_gaus; ++g)
          {
          const eT tmp_dist = distance<eT,1>::eval(N_dims, X_colptr, means.colptr(g), X_colptr);

          if(tmp_dist <= best_dist)  { best_dist = tmp_dist; best_g = g; }
          }

        hist_mem[best_g]++;
        }
      }
    else
    if(dist_mode == prob_dist)
      {
      const eT* log_hefts_mem = log_hefts.memptr();

      for(uword i=0; i<X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT    best_p = -Datum<eT>::inf;
        uword best_g = 0;

        for(uword g=0; g < N_gaus; ++g)
          {
          const eT tmp_p = internal_scalar_log_p(X_colptr, g) + log_hefts_mem[g];

          if(tmp_p >= best_p)  { best_p = tmp_p; best_g = g; }
          }

        hist_mem[best_g]++;
        }
      }
    }
  #endif
  }



template<typename eT>
template<uword dist_id>
inline
void
gmm_full<eT>::generate_initial_means(const Mat<eT>& X, const gmm_seed_mode& seed_mode)
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  if( (seed_mode == static_subset) || (seed_mode == random_subset) )
    {
    uvec initial_indices;

         if(seed_mode == static_subset)  { initial_indices = linspace<uvec>(0, X.n_cols-1, N_gaus);                   }
    else if(seed_mode == random_subset)  { initial_indices = uvec(sort_index(randu<vec>(X.n_cols))).rows(0,N_gaus-1); }

    // not using randi() here as on some primitive systems it produces vectors with non-unique values

    // initial_indices.print("initial_indices:");

    access::rw(means) = X.cols(initial_indices);
    }
  else
  if( (seed_mode == static_spread) || (seed_mode == random_spread) )
    {
    // going through all of the samples can be extremely time consuming;
    // instead, if there are enough samples, randomly choose samples with probability 0.1

    const bool  use_sampling = ((X.n_cols/uword(100)) > N_gaus);
    const uword step         = (use_sampling) ? uword(10) : uword(1);

    uword start_index = 0;

         if(seed_mode == static_spread)  { start_index = X.n_cols / 2;                                         }
    else if(seed_mode == random_spread)  { start_index = as_scalar(randi<uvec>(1, distr_param(0,X.n_cols-1))); }

    access::rw(means).col(0) = X.unsafe_col(start_index);

    const eT* mah_aux_mem = mah_aux.memptr();

    running_stat<double> rs;

    for(uword g=1; g < N_gaus; ++g)
      {
      eT    max_dist = eT(0);
      uword best_i   = uword(0);
      uword start_i  = uword(0);

      if(use_sampling)
        {
        uword start_i_proposed = uword(0);

        if(seed_mode == static_spread)  { start_i_proposed = g % uword(10);                               }
        if(seed_mode == random_spread)  { start_i_proposed = as_scalar(randi<uvec>(1, distr_param(0,9))); }

        if(start_i_proposed < X.n_cols)  { start_i = start_i_proposed; }
        }


      for(uword i=start_i; i < X.n_cols; i += step)
        {
        rs.reset();

        const eT* X_colptr = X.colptr(i);

        bool ignore_i = false;

        // find the average distance between sample i and the means so far
        for(uword h = 0; h < g; ++h)
          {
          const eT dist = distance<eT,dist_id>::eval(N_dims, X_colptr, means.colptr(h), mah_aux_mem);

          // ignore sample already selected as a mean
          if(dist == eT(0))  { ignore_i = true; break; }
          else               { rs(dist);               }
          }

        if( (rs.mean() >= max_dist) && (ignore_i == false))
          {
          max_dist = eT(rs.mean()); best_i = i;
          }
        }

      // set the mean to the sample that is the furthest away from the means so far
      access::rw(means).col(g) = X.unsafe_col(best_i);
      }
    }

  // get_cout_stream() << "generate_initial_means():" << '\n';
  // means.print();
  }



template<typename eT>
template<uword dist_id>
inline
void
gmm_full<eT>::generate_initial_params(const Mat<eT>& X, const eT var_floor)
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const eT* mah_aux_mem = mah_aux.memptr();

  const uword X_n_cols = X.n_cols;

  if(X_n_cols == 0)  { return; }

  // as the covariances are calculated via accumulators,
  // the means also need to be calculated via accumulators to ensure numerical consistency

  Mat<eT> acc_means(N_dims, N_gaus, fill::zeros);
  Mat<eT> acc_dcovs(N_dims, N_gaus, fill::zeros);

  Row<uword> acc_hefts(N_gaus, fill::zeros);

  uword* acc_hefts_mem = acc_hefts.memptr();

  #if defined(ARMA_USE_OPENMP)
    {
    const umat boundaries = internal_gen_boundaries(X_n_cols);

    const uword n_threads = boundaries.n_cols;

    field< Mat<eT>    > t_acc_means(n_threads);
    field< Mat<eT>    > t_acc_dcovs(n_threads);
    field< Row<uword> > t_acc_hefts(n_threads);

    for(uword t=0; t < n_threads; ++t)
      {
      t_acc_means(t).zeros(N_dims, N_gaus);
      t_acc_dcovs(t).zeros(N_dims, N_gaus);
      t_acc_hefts(t).zeros(N_gaus);
      }

    #pragma omp parallel for schedule(static)
    for(uword t=0; t < n_threads; ++t)
      {
      uword* t_acc_hefts_mem = t_acc_hefts(t).memptr();

      const uword start_index = boundaries.at(0,t);
      const uword   end_index = boundaries.at(1,t);

      for(uword i=start_index; i <= end_index; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT     min_dist = Datum<eT>::inf;
        uword  best_g   = 0;

        for(uword g=0; g<N_gaus; ++g)
          {
          const eT dist = distance<eT,dist_id>::eval(N_dims, X_colptr, means.colptr(g), mah_aux_mem);

          if(dist < min_dist)  { min_dist = dist;  best_g = g; }
          }

        eT* t_acc_mean = t_acc_means(t).colptr(best_g);
        eT* t_acc_dcov = t_acc_dcovs(t).colptr(best_g);

        for(uword d=0; d<N_dims; ++d)
          {
          const eT x_d = X_colptr[d];

          t_acc_mean[d] += x_d;
          t_acc_dcov[d] += x_d*x_d;
          }

        t_acc_hefts_mem[best_g]++;
        }
      }

    // reduction
    acc_means = t_acc_means(0);
    acc_dcovs = t_acc_dcovs(0);
    acc_hefts = t_acc_hefts(0);

    for(uword t=1; t < n_threads; ++t)
      {
      acc_means += t_acc_means(t);
      acc_dcovs += t_acc_dcovs(t);
      acc_hefts += t_acc_hefts(t);
      }
    }
  #else
    {
    for(uword i=0; i<X_n_cols; ++i)
      {
      const eT* X_colptr = X.colptr(i);

      eT     min_dist = Datum<eT>::inf;
      uword  best_g   = 0;

      for(uword g=0; g<N_gaus; ++g)
        {
        const eT dist = distance<eT,dist_id>::eval(N_dims, X_colptr, means.colptr(g), mah_aux_mem);

        if(dist < min_dist)  { min_dist = dist;  best_g = g; }
        }

      eT* acc_mean = acc_means.colptr(best_g);
      eT* acc_dcov = acc_dcovs.colptr(best_g);

      for(uword d=0; d<N_dims; ++d)
        {
        const eT x_d = X_colptr[d];

        acc_mean[d] += x_d;
        acc_dcov[d] += x_d*x_d;
        }

      acc_hefts_mem[best_g]++;
      }
    }
  #endif

  eT* hefts_mem = access::rw(hefts).memptr();

  for(uword g=0; g<N_gaus; ++g)
    {
    const eT*   acc_mean = acc_means.colptr(g);
    const eT*   acc_dcov = acc_dcovs.colptr(g);
    const uword acc_heft = acc_hefts_mem[g];

    eT* mean = access::rw(means).colptr(g);

    Mat<eT>& fcov = access::rw(fcovs).slice(g);
    fcov.zeros();

    for(uword d=0; d<N_dims; ++d)
      {
      const eT tmp = acc_mean[d] / eT(acc_heft);

      mean[d]      = (acc_heft >= 1) ? tmp : eT(0);
      fcov.at(d,d) = (acc_heft >= 2) ? eT((acc_dcov[d] / eT(acc_heft)) - (tmp*tmp)) : eT(var_floor);
      }

    hefts_mem[g] = eT(acc_heft) / eT(X_n_cols);
    }

  em_fix_params(var_floor);
  }



//! multi-threaded implementation of k-means, inspired by MapReduce
template<typename eT>
template<uword dist_id>
inline
bool
gmm_full<eT>::km_iterate(const Mat<eT>& X, const uword max_iter, const bool verbose)
  {
  arma_extra_debug_sigprint();

  if(verbose)
    {
    get_cout_stream().unsetf(ios::showbase);
    get_cout_stream().unsetf(ios::uppercase);
    get_cout_stream().unsetf(ios::showpos);
    get_cout_stream().unsetf(ios::scientific);

    get_cout_stream().setf(ios::right);
    get_cout_stream().setf(ios::fixed);
    }

  const uword X_n_cols = X.n_cols;

  if(X_n_cols == 0)  { return true; }

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const eT* mah_aux_mem = mah_aux.memptr();

  Mat<eT>    acc_means(N_dims, N_gaus, fill::zeros);
  Row<uword> acc_hefts(N_gaus, fill::zeros);
  Row<uword> last_indx(N_gaus, fill::zeros);

  Mat<eT> new_means = means;
  Mat<eT> old_means = means;

  running_mean_scalar<eT> rs_delta;

  #if defined(ARMA_USE_OPENMP)
    const umat boundaries = internal_gen_boundaries(X_n_cols);
    const uword n_threads = boundaries.n_cols;

    field< Mat<eT>    > t_acc_means(n_threads);
    field< Row<uword> > t_acc_hefts(n_threads);
    field< Row<uword> > t_last_indx(n_threads);
  #else
    const uword n_threads = 1;
  #endif

  if(verbose)  { get_cout_stream() << "gmm_full::learn(): k-means: n_threads: " << n_threads << '\n';  get_cout_stream().flush(); }

  for(uword iter=1; iter <= max_iter; ++iter)
    {
    #if defined(ARMA_USE_OPENMP)
      {
      for(uword t=0; t < n_threads; ++t)
        {
        t_acc_means(t).zeros(N_dims, N_gaus);
        t_acc_hefts(t).zeros(N_gaus);
        t_last_indx(t).zeros(N_gaus);
        }

      #pragma omp parallel for schedule(static)
      for(uword t=0; t < n_threads; ++t)
        {
        Mat<eT>& t_acc_means_t   = t_acc_means(t);
        uword*   t_acc_hefts_mem = t_acc_hefts(t).memptr();
        uword*   t_last_indx_mem = t_last_indx(t).memptr();

        const uword start_index = boundaries.at(0,t);
        const uword   end_index = boundaries.at(1,t);

        for(uword i=start_index; i <= end_index; ++i)
          {
          const eT* X_colptr = X.colptr(i);

          eT     min_dist = Datum<eT>::inf;
          uword  best_g   = 0;

          for(uword g=0; g<N_gaus; ++g)
            {
            const eT dist = distance<eT,dist_id>::eval(N_dims, X_colptr, old_means.colptr(g), mah_aux_mem);

            if(dist < min_dist)  { min_dist = dist;  best_g = g; }
            }

          eT* t_acc_mean = t_acc_means_t.colptr(best_g);

          for(uword d=0; d<N_dims; ++d)  { t_acc_mean[d] += X_colptr[d]; }

          t_acc_hefts_mem[best_g]++;
          t_last_indx_mem[best_g] = i;
          }
        }

      // reduction

      acc_means = t_acc_means(0);
      acc_hefts = t_acc_hefts(0);

      for(uword t=1; t < n_threads; ++t)
        {
        acc_means += t_acc_means(t);
        acc_hefts += t_acc_hefts(t);
        }

      for(uword g=0; g < N_gaus;    ++g)
      for(uword t=0; t < n_threads; ++t)
        {
        if( t_acc_hefts(t)(g) >= 1 )  { last_indx(g) = t_last_indx(t)(g); }
        }
      }
    #else
      {
      uword* acc_hefts_mem = acc_hefts.memptr();
      uword* last_indx_mem = last_indx.memptr();

      for(uword i=0; i < X_n_cols; ++i)
        {
        const eT* X_colptr = X.colptr(i);

        eT     min_dist = Datum<eT>::inf;
        uword  best_g   = 0;

        for(uword g=0; g<N_gaus; ++g)
          {
          const eT dist = distance<eT,dist_id>::eval(N_dims, X_colptr, old_means.colptr(g), mah_aux_mem);

          if(dist < min_dist)  { min_dist = dist;  best_g = g; }
          }

        eT* acc_mean = acc_means.colptr(best_g);

        for(uword d=0; d<N_dims; ++d)  { acc_mean[d] += X_colptr[d]; }

        acc_hefts_mem[best_g]++;
        last_indx_mem[best_g] = i;
        }
      }
    #endif

    // generate new means

    uword* acc_hefts_mem = acc_hefts.memptr();

    for(uword g=0; g < N_gaus; ++g)
      {
      const eT*   acc_mean = acc_means.colptr(g);
      const uword acc_heft = acc_hefts_mem[g];

      eT* new_mean = access::rw(new_means).colptr(g);

      for(uword d=0; d<N_dims; ++d)
        {
        new_mean[d] = (acc_heft >= 1) ? (acc_mean[d] / eT(acc_heft)) : eT(0);
        }
      }


    // heuristics to resurrect dead means

    const uvec dead_gs = find(acc_hefts == uword(0));

    if(dead_gs.n_elem > 0)
      {
      if(verbose)  { get_cout_stream() << "gmm_full::learn(): k-means: recovering from dead means\n"; get_cout_stream().flush(); }

      uword* last_indx_mem = last_indx.memptr();

      const uvec live_gs = sort( find(acc_hefts >= uword(2)), "descend" );

      if(live_gs.n_elem == 0)  { return false; }

      uword live_gs_count  = 0;

      for(uword dead_gs_count = 0; dead_gs_count < dead_gs.n_elem; ++dead_gs_count)
        {
        const uword dead_g_id = dead_gs(dead_gs_count);

        uword proposed_i = 0;

        if(live_gs_count < live_gs.n_elem)
          {
          const uword live_g_id = live_gs(live_gs_count);  ++live_gs_count;

          if(live_g_id == dead_g_id)  { return false; }

          // recover by using a sample from a known good mean
          proposed_i = last_indx_mem[live_g_id];
          }
        else
          {
          // recover by using a randomly seleced sample (last resort)
          proposed_i = as_scalar(randi<uvec>(1, distr_param(0,X_n_cols-1)));
          }

        if(proposed_i >= X_n_cols)  { return false; }

        new_means.col(dead_g_id) = X.col(proposed_i);
        }
      }

    rs_delta.reset();

    for(uword g=0; g < N_gaus; ++g)
      {
      rs_delta( distance<eT,dist_id>::eval(N_dims, old_means.colptr(g), new_means.colptr(g), mah_aux_mem) );
      }

    if(verbose)
      {
      get_cout_stream() << "gmm_full::learn(): k-means: iteration: ";
      get_cout_stream().unsetf(ios::scientific);
      get_cout_stream().setf(ios::fixed);
      get_cout_stream().width(std::streamsize(4));
      get_cout_stream() << iter;
      get_cout_stream() << "   delta: ";
      get_cout_stream().unsetf(ios::fixed);
      //get_cout_stream().setf(ios::scientific);
      get_cout_stream() << rs_delta.mean() << '\n';
      get_cout_stream().flush();
      }

    arma::swap(old_means, new_means);

    if(rs_delta.mean() <= Datum<eT>::eps)  { break; }
    }

  access::rw(means) = old_means;

  if(means.is_finite() == false)  { return false; }

  return true;
  }



//! multi-threaded implementation of Expectation-Maximisation, inspired by MapReduce
template<typename eT>
inline
bool
gmm_full<eT>::em_iterate(const Mat<eT>& X, const uword max_iter, const eT var_floor, const bool verbose)
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  if(verbose)
    {
    get_cout_stream().unsetf(ios::showbase);
    get_cout_stream().unsetf(ios::uppercase);
    get_cout_stream().unsetf(ios::showpos);
    get_cout_stream().unsetf(ios::scientific);

    get_cout_stream().setf(ios::right);
    get_cout_stream().setf(ios::fixed);
    }

  const umat boundaries = internal_gen_boundaries(X.n_cols);

  const uword n_threads = boundaries.n_cols;

  field<  Mat<eT> > t_acc_means(n_threads);
  field< Cube<eT> > t_acc_fcovs(n_threads);

  field< Col<eT> > t_acc_norm_lhoods(n_threads);
  field< Col<eT> > t_gaus_log_lhoods(n_threads);

  Col<eT>          t_progress_log_lhood(n_threads);

  for(uword t=0; t<n_threads; t++)
    {
    t_acc_means[t].set_size(N_dims, N_gaus);
    t_acc_fcovs[t].set_size(N_dims, N_dims, N_gaus);

    t_acc_norm_lhoods[t].set_size(N_gaus);
    t_gaus_log_lhoods[t].set_size(N_gaus);
    }


  if(verbose)
    {
    get_cout_stream() << "gmm_full::learn(): EM: n_threads: " << n_threads  << '\n';
    }

  eT old_avg_log_p = -Datum<eT>::inf;

  const bool calc_chol = false;

  for(uword iter=1; iter <= max_iter; ++iter)
    {
    init_constants(calc_chol);

    em_update_params(X, boundaries, t_acc_means, t_acc_fcovs, t_acc_norm_lhoods, t_gaus_log_lhoods, t_progress_log_lhood, var_floor);

    em_fix_params(var_floor);

    const eT new_avg_log_p = accu(t_progress_log_lhood) / eT(t_progress_log_lhood.n_elem);

    if(verbose)
      {
      get_cout_stream() << "gmm_full::learn(): EM: iteration: ";
      get_cout_stream().unsetf(ios::scientific);
      get_cout_stream().setf(ios::fixed);
      get_cout_stream().width(std::streamsize(4));
      get_cout_stream() << iter;
      get_cout_stream() << "   avg_log_p: ";
      get_cout_stream().unsetf(ios::fixed);
      //get_cout_stream().setf(ios::scientific);
      get_cout_stream() << new_avg_log_p << '\n';
      get_cout_stream().flush();
      }

    if(arma_isfinite(new_avg_log_p) == false)  { return false; }

    if(std::abs(old_avg_log_p - new_avg_log_p) <= Datum<eT>::eps)  { break; }


    old_avg_log_p = new_avg_log_p;
    }


  for(uword g=0; g < N_gaus; ++g)
    {
    const Mat<eT>& fcov = fcovs.slice(g);

    if(any(vectorise(fcov.diag()) <= eT(0)))  { return false; }
    }

  if(means.is_finite() == false)  { return false; }
  if(fcovs.is_finite() == false)  { return false; }
  if(hefts.is_finite() == false)  { return false; }

  return true;
  }




template<typename eT>
inline
void
gmm_full<eT>::em_update_params
  (
  const Mat<eT>&           X,
  const umat&              boundaries,
        field<  Mat<eT> >& t_acc_means,
        field< Cube<eT> >& t_acc_fcovs,
        field<  Col<eT> >& t_acc_norm_lhoods,
        field<  Col<eT> >& t_gaus_log_lhoods,
        Col<eT>&           t_progress_log_lhood,
  const eT                 var_floor
  )
  {
  arma_extra_debug_sigprint();

  const uword n_threads = boundaries.n_cols;


  // em_generate_acc() is the "map" operation, which produces partial accumulators for means, diagonal covariances and hefts

  #if defined(ARMA_USE_OPENMP)
    {
    #pragma omp parallel for schedule(static)
    for(uword t=0; t<n_threads; t++)
      {
       Mat<eT>& acc_means          = t_acc_means[t];
      Cube<eT>& acc_fcovs          = t_acc_fcovs[t];
       Col<eT>& acc_norm_lhoods    = t_acc_norm_lhoods[t];
       Col<eT>& gaus_log_lhoods    = t_gaus_log_lhoods[t];
       eT&      progress_log_lhood = t_progress_log_lhood[t];

      em_generate_acc(X, boundaries.at(0,t), boundaries.at(1,t), acc_means, acc_fcovs, acc_norm_lhoods, gaus_log_lhoods, progress_log_lhood);
      }
    }
  #else
    {
    em_generate_acc(X, boundaries.at(0,0), boundaries.at(1,0), t_acc_means[0], t_acc_fcovs[0], t_acc_norm_lhoods[0], t_gaus_log_lhoods[0], t_progress_log_lhood[0]);
    }
  #endif

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

   Mat<eT>& final_acc_means = t_acc_means[0];
  Cube<eT>& final_acc_fcovs = t_acc_fcovs[0];

  Col<eT>& final_acc_norm_lhoods = t_acc_norm_lhoods[0];


  // the "reduce" operation, which combines the partial accumulators produced by the separate threads

  for(uword t=1; t<n_threads; t++)
    {
    final_acc_means += t_acc_means[t];
    final_acc_fcovs += t_acc_fcovs[t];

    final_acc_norm_lhoods += t_acc_norm_lhoods[t];
    }


  eT* hefts_mem = access::rw(hefts).memptr();

  Mat<eT> mean_outer(N_dims, N_dims);


  //// update each component without sanity checking
  //for(uword g=0; g < N_gaus; ++g)
  //  {
  //  const eT acc_norm_lhood = (std::max)( final_acc_norm_lhoods[g], std::numeric_limits<eT>::min() );
  //
  //    hefts_mem[g] = acc_norm_lhood / eT(X.n_cols);
  //
  //    eT*     mean_mem = access::rw(means).colptr(g);
  //    eT* acc_mean_mem = final_acc_means.colptr(g);
  //
  //    for(uword d=0; d < N_dims; ++d)
  //      {
  //      mean_mem[d] = acc_mean_mem[d] / acc_norm_lhood;
  //      }
  //
  //    const Col<eT> mean(mean_mem, N_dims, false, true);
  //
  //    mean_outer = mean * mean.t();
  //
  //    Mat<eT>&     fcov = access::rw(fcovs).slice(g);
  //    Mat<eT>& acc_fcov = final_acc_fcovs.slice(g);
  //
  //    fcov = acc_fcov / acc_norm_lhood - mean_outer;
  //    }


  // conditionally update each component; if only a subset of the hefts was updated, em_fix_params() will sanitise them
  for(uword g=0; g < N_gaus; ++g)
    {
    const eT acc_norm_lhood = (std::max)( final_acc_norm_lhoods[g], std::numeric_limits<eT>::min() );

    if(arma_isfinite(acc_norm_lhood) == false)  { continue; }

    eT* acc_mean_mem = final_acc_means.colptr(g);

    for(uword d=0; d < N_dims; ++d)
      {
      acc_mean_mem[d] /= acc_norm_lhood;
      }

    const Col<eT> new_mean(acc_mean_mem, N_dims, false, true);

    mean_outer = new_mean * new_mean.t();

    Mat<eT>& acc_fcov = final_acc_fcovs.slice(g);

    acc_fcov /= acc_norm_lhood;
    acc_fcov -= mean_outer;

    for(uword d=0; d < N_dims; ++d)
      {
      eT& val = acc_fcov.at(d,d);

      if(val < var_floor)  { val = var_floor; }
      }

    if(acc_fcov.is_finite() == false)  { continue; }

    eT log_det_val  = eT(0);
    eT log_det_sign = eT(0);

    log_det(log_det_val, log_det_sign, acc_fcov);

    const bool log_det_ok = ( (arma_isfinite(log_det_val)) && (log_det_sign > eT(0)) );

    const bool inv_ok = (log_det_ok) ? bool(auxlib::inv_sympd(mean_outer, acc_fcov)) : bool(false);  // mean_outer is used as a junk matrix

    if(log_det_ok && inv_ok)
      {
      hefts_mem[g] = acc_norm_lhood / eT(X.n_cols);

      eT* mean_mem = access::rw(means).colptr(g);

      for(uword d=0; d < N_dims; ++d)
        {
        mean_mem[d] = acc_mean_mem[d];
        }

      Mat<eT>& fcov = access::rw(fcovs).slice(g);

      fcov = acc_fcov;
      }
    }
  }



template<typename eT>
inline
void
gmm_full<eT>::em_generate_acc
  (
  const  Mat<eT>& X,
  const  uword    start_index,
  const  uword      end_index,
         Mat<eT>& acc_means,
        Cube<eT>& acc_fcovs,
         Col<eT>& acc_norm_lhoods,
         Col<eT>& gaus_log_lhoods,
         eT&      progress_log_lhood
  )
  const
  {
  arma_extra_debug_sigprint();

  progress_log_lhood = eT(0);

  acc_means.zeros();
  acc_fcovs.zeros();

  acc_norm_lhoods.zeros();
  gaus_log_lhoods.zeros();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const eT* log_hefts_mem       = log_hefts.memptr();
        eT* gaus_log_lhoods_mem = gaus_log_lhoods.memptr();


  for(uword i=start_index; i <= end_index; i++)
    {
    const eT* x = X.colptr(i);

    for(uword g=0; g < N_gaus; ++g)
      {
      gaus_log_lhoods_mem[g] = internal_scalar_log_p(x, g) + log_hefts_mem[g];
      }

    eT log_lhood_sum = gaus_log_lhoods_mem[0];

    for(uword g=1; g < N_gaus; ++g)
      {
      log_lhood_sum = log_add_exp(log_lhood_sum, gaus_log_lhoods_mem[g]);
      }

    progress_log_lhood += log_lhood_sum;

    for(uword g=0; g < N_gaus; ++g)
      {
      const eT norm_lhood = std::exp(gaus_log_lhoods_mem[g] - log_lhood_sum);

      acc_norm_lhoods[g] += norm_lhood;

      eT* acc_mean_mem = acc_means.colptr(g);

      for(uword d=0; d < N_dims; ++d)
        {
        acc_mean_mem[d] += x[d] * norm_lhood;
        }

      Mat<eT>& acc_fcov = access::rw(acc_fcovs).slice(g);

      // specialised version of acc_fcov += norm_lhood * (xx * xx.t());

      for(uword d=0; d < N_dims; ++d)
        {
        const uword dp1 = d+1;

        const eT xd = x[d];

        eT* acc_fcov_col_d = acc_fcov.colptr(d) + d;
        eT* acc_fcov_row_d = &(acc_fcov.at(d,dp1));

        (*acc_fcov_col_d) += norm_lhood * (xd * xd);  acc_fcov_col_d++;

        for(uword e=dp1; e < N_dims; ++e)
          {
          const eT val = norm_lhood * (xd * x[e]);

          (*acc_fcov_col_d) += val;  acc_fcov_col_d++;
          (*acc_fcov_row_d) += val;  acc_fcov_row_d += N_dims;
          }
        }
      }
    }

  progress_log_lhood /= eT((end_index - start_index) + 1);
  }



template<typename eT>
inline
void
gmm_full<eT>::em_fix_params(const eT var_floor)
  {
  arma_extra_debug_sigprint();

  const uword N_dims = means.n_rows;
  const uword N_gaus = means.n_cols;

  const eT var_ceiling = std::numeric_limits<eT>::max();

  for(uword g=0; g < N_gaus; ++g)
    {
    Mat<eT>& fcov = access::rw(fcovs).slice(g);

    for(uword d=0; d < N_dims; ++d)
      {
      eT& var_val = fcov.at(d,d);

           if(var_val < var_floor  )  { var_val = var_floor;   }
      else if(var_val > var_ceiling)  { var_val = var_ceiling; }
      else if(arma_isnan(var_val)  )  { var_val = eT(1);       }
      }
    }


  eT* hefts_mem = access::rw(hefts).memptr();

  for(uword g1=0; g1 < N_gaus; ++g1)
    {
    if(hefts_mem[g1] > eT(0))
      {
      const eT* means_colptr_g1 = means.colptr(g1);

      for(uword g2=(g1+1); g2 < N_gaus; ++g2)
        {
        if( (hefts_mem[g2] > eT(0)) && (std::abs(hefts_mem[g1] - hefts_mem[g2]) <= std::numeric_limits<eT>::epsilon()) )
          {
          const eT dist = distance<eT,1>::eval(N_dims, means_colptr_g1, means.colptr(g2), means_colptr_g1);

          if(dist == eT(0)) { hefts_mem[g2] = eT(0); }
          }
        }
      }
    }

  const eT heft_floor   = std::numeric_limits<eT>::min();
  const eT heft_initial = eT(1) / eT(N_gaus);

  for(uword i=0; i < N_gaus; ++i)
    {
    eT& heft_val = hefts_mem[i];

         if(heft_val < heft_floor)  { heft_val = heft_floor;   }
    else if(heft_val > eT(1)     )  { heft_val = eT(1);        }
    else if(arma_isnan(heft_val) )  { heft_val = heft_initial; }
    }

  const eT heft_sum = accu(hefts);

  if((heft_sum < (eT(1) - Datum<eT>::eps)) || (heft_sum > (eT(1) + Datum<eT>::eps)))  { access::rw(hefts) /= heft_sum; }
  }



} // namespace gmm_priv


//! @}
