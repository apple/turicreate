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


template<typename obj_type, bool> struct rsv_get_elem_type_worker                  { };
template<typename obj_type>       struct rsv_get_elem_type_worker<obj_type, false> { typedef          obj_type            result; };
template<typename obj_type>       struct rsv_get_elem_type_worker<obj_type, true>  { typedef typename obj_type::elem_type result; };

template<typename obj_type> struct rsv_get_elem_type { typedef typename rsv_get_elem_type_worker<obj_type, is_Mat<obj_type>::value>::result elem_type; };


template<typename obj_type, bool> struct rsv_get_return_type1_worker                  { };
template<typename obj_type>       struct rsv_get_return_type1_worker<obj_type, false> { typedef Mat<obj_type>  result; };
template<typename obj_type>       struct rsv_get_return_type1_worker<obj_type, true>  { typedef obj_type       result; };

template<typename obj_type> struct rsv_get_return_type1 { typedef typename rsv_get_return_type1_worker<obj_type, is_Mat<obj_type>::value>::result return_type1; };


template<typename return_type1> struct rsv_get_return_type2            { };
template<typename eT>           struct rsv_get_return_type2< Mat<eT> > { typedef Mat<typename get_pod_type<eT>::result> return_type2; };
template<typename eT>           struct rsv_get_return_type2< Row<eT> > { typedef Row<typename get_pod_type<eT>::result> return_type2; };
template<typename eT>           struct rsv_get_return_type2< Col<eT> > { typedef Col<typename get_pod_type<eT>::result> return_type2; };


//! Class for keeping statistics of a continuously sampled process / signal.
//! Useful if the storage of individual samples is not necessary or desired.
//! Also useful if the number of samples is not known beforehand or exceeds
//! available memory.
template<typename obj_type>
class running_stat_vec
  {
  public:

  // voodoo for compatibility with old user code
  typedef typename rsv_get_elem_type<obj_type>::elem_type eT;

  typedef typename get_pod_type<eT>::result T;

  typedef typename rsv_get_return_type1<obj_type    >::return_type1 return_type1;
  typedef typename rsv_get_return_type2<return_type1>::return_type2 return_type2;

  inline ~running_stat_vec();
  inline  running_stat_vec(const bool in_calc_cov = false);  // TODO: investigate char* overload, eg. "calc_cov", "no_calc_cov"

  inline running_stat_vec(const running_stat_vec& in_rsv);

  inline running_stat_vec& operator=(const running_stat_vec& in_rsv);

  template<typename T1> arma_hot inline void operator() (const Base<              T, T1>& X);
  template<typename T1> arma_hot inline void operator() (const Base<std::complex<T>, T1>& X);

  inline void reset();

  inline const return_type1&  mean() const;

  inline const return_type2&  var   (const uword norm_type = 0);
  inline       return_type2   stddev(const uword norm_type = 0) const;
  inline const Mat<eT>&       cov   (const uword norm_type = 0);

  inline const return_type1& min()   const;
  inline const return_type1& max()   const;
  inline       return_type1  range() const;

  inline T count() const;

  //
  //

  private:

  const bool calc_cov;

  arma_aligned arma_counter<T> counter;

  arma_aligned return_type1 r_mean;
  arma_aligned return_type2 r_var;
  arma_aligned Mat<eT>      r_cov;

  arma_aligned return_type1 min_val;
  arma_aligned return_type1 max_val;

  arma_aligned Mat< T> min_val_norm;
  arma_aligned Mat< T> max_val_norm;

  arma_aligned return_type2 r_var_dummy;
  arma_aligned Mat<eT>      r_cov_dummy;

  arma_aligned Mat<eT> tmp1;
  arma_aligned Mat<eT> tmp2;

  friend class running_stat_vec_aux;
  };



class running_stat_vec_aux
  {
  public:

  template<typename obj_type>
  inline static void
  update_stats
    (
    running_stat_vec<obj_type>& x,
    const                  Mat<typename running_stat_vec<obj_type>::eT>& sample,
    const typename arma_not_cx<typename running_stat_vec<obj_type>::eT>::result* junk = 0
    );

  template<typename obj_type>
  inline static void
  update_stats
    (
    running_stat_vec<obj_type>& x,
    const          Mat<std::complex< typename running_stat_vec<obj_type>::T > >& sample,
    const typename       arma_not_cx<typename running_stat_vec<obj_type>::eT>::result* junk = 0
    );

  template<typename obj_type>
  inline static void
  update_stats
    (
    running_stat_vec<obj_type>& x,
    const                  Mat< typename running_stat_vec<obj_type>::T >& sample,
    const typename arma_cx_only<typename running_stat_vec<obj_type>::eT>::result* junk = 0
    );

  template<typename obj_type>
  inline static void
  update_stats
    (
    running_stat_vec<obj_type>& x,
    const                   Mat<typename running_stat_vec<obj_type>::eT>& sample,
    const typename arma_cx_only<typename running_stat_vec<obj_type>::eT>::result* junk = 0
    );
  };



//! @}
