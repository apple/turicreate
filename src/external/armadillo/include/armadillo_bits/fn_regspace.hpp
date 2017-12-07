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


//! \addtogroup fn_regspace
//! @{



template<typename eT>
inline
void
internal_regspace_default_delta
  (
  Mat<eT>& x,
  const typename Mat<eT>::pod_type start,
  const typename Mat<eT>::pod_type end
  )
  {
  arma_extra_debug_sigprint();

  typedef typename Mat<eT>::pod_type T;

  const bool ascend = (start <= end);

  const uword N = uword(1) + uword((ascend) ? (end-start) : (start-end));

  x.set_size(N);

  eT* x_mem = x.memptr();

  if(ascend)
    {
    for(uword i=0; i < N; ++i)  { x_mem[i] = eT(start + T(i)); }
    }
  else
    {
    for(uword i=0; i < N; ++i)  { x_mem[i] = eT(start - T(i)); }
    }
  }



template<typename eT, typename sT>
inline
typename enable_if2< (is_signed<sT>::value == true), void >::result
internal_regspace_var_delta
  (
  Mat<eT>& x,
  const typename Mat<eT>::pod_type start,
  const sT                         delta,
  const typename Mat<eT>::pod_type end
  )
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("internal_regspace_var_delta(): signed version");

  typedef typename Mat<eT>::pod_type T;

  if( ((start < end) && (delta < sT(0))) || ((start > end) && (delta > sT(0))) || (delta == sT(0)) )  { return; }

  const bool ascend = (start <= end);

  const T inc = (delta < sT(0)) ? T(-delta) : T(delta);

  const T M = ((ascend) ? T(end-start) : T(start-end)) / T(inc);

  const uword N = uword(1) + ( (is_non_integral<T>::value) ? uword(std::floor(double(M))) : uword(M) );

  x.set_size(N);

  eT* x_mem = x.memptr();

  if(ascend)
    {
    for(uword i=0; i < N; ++i) { x_mem[i] = eT( start + T(i*inc) ); }
    }
  else
    {
    for(uword i=0; i < N; ++i) { x_mem[i] = eT( start - T(i*inc) ); }
    }
  }



template<typename eT, typename uT>
inline
typename enable_if2< (is_signed<uT>::value == false), void >::result
internal_regspace_var_delta
  (
  Mat<eT>& x,
  const typename Mat<eT>::pod_type start,
  const          uT                delta,
  const typename Mat<eT>::pod_type end
  )
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("internal_regspace_var_delta(): unsigned version");

  typedef typename Mat<eT>::pod_type T;

  if( ((start > end) && (delta > uT(0))) || (delta == uT(0)) )  { return; }

  const bool ascend = (start <= end);

  const T inc = T(delta);

  const T M = ((ascend) ? T(end-start) : T(start-end)) / T(inc);

  const uword N = uword(1) + ( (is_non_integral<T>::value) ? uword(std::floor(double(M))) : uword(M) );

  x.set_size(N);

  eT* x_mem = x.memptr();

  if(ascend)
    {
    for(uword i=0; i < N; ++i) { x_mem[i] = eT( start + T(i*inc) ); }
    }
  else
    {
    for(uword i=0; i < N; ++i) { x_mem[i] = eT( start - T(i*inc) ); }
    }
  }



template<typename vec_type, typename sT>
inline
typename enable_if2< is_Mat<vec_type>::value && (is_signed<sT>::value == true), vec_type >::result
regspace
  (
  const typename vec_type::pod_type start,
  const          sT                 delta,
  const typename vec_type::pod_type end
  )
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("regspace(): signed version");

  vec_type x;

  if( ((delta == sT(+1)) && (start <= end)) || ((delta == sT(-1)) && (start > end)) )
    {
    internal_regspace_default_delta(x, start, end);
    }
  else
    {
    internal_regspace_var_delta(x, start, delta, end);
    }

  if(x.n_elem == 0)
    {
    if(is_Mat_only<vec_type>::value)  { x.set_size(1,0); }
    }

  return x;
  }



template<typename vec_type, typename uT>
inline
typename enable_if2< is_Mat<vec_type>::value && (is_signed<uT>::value == false), vec_type >::result
regspace
  (
  const typename vec_type::pod_type start,
  const          uT                 delta,
  const typename vec_type::pod_type end
  )
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("regspace(): unsigned version");

  vec_type x;

  if( (delta == uT(+1)) && (start <= end) )
    {
    internal_regspace_default_delta(x, start, end);
    }
  else
    {
    internal_regspace_var_delta(x, start, delta, end);
    }

  if(x.n_elem == 0)
    {
    if(is_Mat_only<vec_type>::value)  { x.set_size(1,0); }
    }

  return x;
  }



template<typename vec_type>
arma_warn_unused
inline
typename
enable_if2
  <
  is_Mat<vec_type>::value,
  vec_type
  >::result
regspace
  (
  const typename vec_type::pod_type start,
  const typename vec_type::pod_type end
  )
  {
  arma_extra_debug_sigprint();

  vec_type x;

  internal_regspace_default_delta(x, start, end);

  if(x.n_elem == 0)
    {
    if(is_Mat_only<vec_type>::value)  { x.set_size(1,0); }
    }

  return x;
  }



arma_warn_unused
inline
vec
regspace(const double start, const double delta, const double end)
  {
  arma_extra_debug_sigprint();

  return regspace<vec>(start, delta, end);
  }



arma_warn_unused
inline
vec
regspace(const double start, const double end)
  {
  arma_extra_debug_sigprint();

  return regspace<vec>(start, end);
  }



//! @}
