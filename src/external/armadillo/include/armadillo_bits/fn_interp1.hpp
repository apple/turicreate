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


//! \addtogroup fn_interp1
//! @{



template<typename eT>
inline
void
interp1_helper_nearest(const Mat<eT>& XG, const Mat<eT>& YG, const Mat<eT>& XI, Mat<eT>& YI, const eT extrap_val)
  {
  arma_extra_debug_sigprint();

  const eT XG_min = XG.min();
  const eT XG_max = XG.max();

  YI.copy_size(XI);

  const eT* XG_mem = XG.memptr();
  const eT* YG_mem = YG.memptr();
  const eT* XI_mem = XI.memptr();
        eT* YI_mem = YI.memptr();

  const uword NG = XG.n_elem;
  const uword NI = XI.n_elem;

  uword best_j = 0;

  for(uword i=0; i<NI; ++i)
    {
    eT best_err = Datum<eT>::inf;

    const eT XI_val = XI_mem[i];

    if((XI_val < XG_min) || (XI_val > XG_max))
      {
      YI_mem[i] = extrap_val;
      }
    else
      {
      // XG and XI are guaranteed to be sorted in ascending manner,
      // so start searching XG from last known optimum position

      for(uword j=best_j; j<NG; ++j)
        {
        const eT tmp = XG_mem[j] - XI_val;
        const eT err = (tmp >= eT(0)) ? tmp : -tmp;

        if(err >= best_err)
          {
          // error is going up, so we have found the optimum position
          break;
          }
        else
          {
          best_err = err;
          best_j   = j;   // remember the optimum position
          }
        }

      YI_mem[i] = YG_mem[best_j];
      }
    }
  }



template<typename eT>
inline
void
interp1_helper_linear(const Mat<eT>& XG, const Mat<eT>& YG, const Mat<eT>& XI, Mat<eT>& YI, const eT extrap_val)
  {
  arma_extra_debug_sigprint();

  const eT XG_min = XG.min();
  const eT XG_max = XG.max();

  YI.copy_size(XI);

  const eT* XG_mem = XG.memptr();
  const eT* YG_mem = YG.memptr();
  const eT* XI_mem = XI.memptr();
        eT* YI_mem = YI.memptr();

  const uword NG = XG.n_elem;
  const uword NI = XI.n_elem;

  uword a_best_j = 0;
  uword b_best_j = 0;

  for(uword i=0; i<NI; ++i)
    {
    const eT XI_val = XI_mem[i];

    if((XI_val < XG_min) || (XI_val > XG_max))
      {
      YI_mem[i] = extrap_val;
      }
    else
      {
      // XG and XI are guaranteed to be sorted in ascending manner,
      // so start searching XG from last known optimum position

      eT a_best_err = Datum<eT>::inf;
      eT b_best_err = Datum<eT>::inf;

      for(uword j=a_best_j; j<NG; ++j)
        {
        const eT tmp = XG_mem[j] - XI_val;
        const eT err = (tmp >= eT(0)) ? tmp : -tmp;

        if(err >= a_best_err)
          {
          break;
          }
        else
          {
          a_best_err = err;
          a_best_j   = j;
          }
        }

      if( (XG_mem[a_best_j] - XI_val) <= eT(0) )
        {
        // a_best_j is to the left of the interpolated position

        b_best_j = ( (a_best_j+1) < NG) ? (a_best_j+1) : a_best_j;
        }
      else
        {
        // a_best_j is to the right of the interpolated position

        b_best_j = (a_best_j >= 1) ? (a_best_j-1) : a_best_j;
        }

      b_best_err = std::abs( XG_mem[b_best_j] - XI_val );

      if(a_best_j > b_best_j)
        {
        std::swap(a_best_j,   b_best_j  );
        std::swap(a_best_err, b_best_err);
        }

      const eT weight = (a_best_err > eT(0)) ? (a_best_err / (a_best_err + b_best_err)) : eT(0);

      YI_mem[i] = (eT(1) - weight)*YG_mem[a_best_j] + (weight)*YG_mem[b_best_j];
      }
    }
  }



template<typename eT>
inline
void
interp1_helper(const Mat<eT>& X, const Mat<eT>& Y, const Mat<eT>& XI, Mat<eT>& YI, const uword sig, const eT extrap_val)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ((X.is_vec() == false) || (Y.is_vec() == false) || (XI.is_vec() == false)), "interp1(): currently only vectors are supported" );

  arma_debug_check( (X.n_elem != Y.n_elem), "interp1(): X and Y must have the same number of elements" );

  arma_debug_check( (X.n_elem < 2), "interp1(): X must have at least two unique elements" );

  // sig = 10: nearest neighbour
  // sig = 11: nearest neighbour, assume monotonic increase in X and XI
  //
  // sig = 20: linear
  // sig = 21: linear, assume monotonic increase in X and XI

  if(sig == 11)  { interp1_helper_nearest(X, Y, XI, YI, extrap_val); return; }
  if(sig == 21)  { interp1_helper_linear (X, Y, XI, YI, extrap_val); return; }

  uvec X_indices;

  try { X_indices = find_unique(X,false); } catch(...) { }

  // NOTE: find_unique(X,false) provides indices of elements sorted in ascending order
  // NOTE: find_unique(X,false) will reset X_indices if X has NaN

  const uword N_subset = X_indices.n_elem;

  arma_debug_check( (N_subset < 2), "interp1(): X must have at least two unique elements" );

  Mat<eT> X_sanitised(N_subset,1);
  Mat<eT> Y_sanitised(N_subset,1);

  eT* X_sanitised_mem = X_sanitised.memptr();
  eT* Y_sanitised_mem = Y_sanitised.memptr();

  const eT* X_mem = X.memptr();
  const eT* Y_mem = Y.memptr();

  const uword* X_indices_mem = X_indices.memptr();

  for(uword i=0; i<N_subset; ++i)
    {
    const uword j = X_indices_mem[i];

    X_sanitised_mem[i] = X_mem[j];
    Y_sanitised_mem[i] = Y_mem[j];
    }


  Mat<eT> XI_tmp;
  uvec    XI_indices;

  const bool XI_is_sorted = XI.is_sorted();

  if(XI_is_sorted == false)
    {
    XI_indices = sort_index(XI);

    const uword N = XI.n_elem;

    XI_tmp.copy_size(XI);

    const uword* XI_indices_mem = XI_indices.memptr();

    const eT* XI_mem     = XI.memptr();
          eT* XI_tmp_mem = XI_tmp.memptr();

    for(uword i=0; i<N; ++i)
      {
      XI_tmp_mem[i] = XI_mem[ XI_indices_mem[i] ];
      }
    }

  const Mat<eT>& XI_sorted = (XI_is_sorted) ? XI : XI_tmp;


       if(sig == 10)  { interp1_helper_nearest(X_sanitised, Y_sanitised, XI_sorted, YI, extrap_val); }
  else if(sig == 20)  { interp1_helper_linear (X_sanitised, Y_sanitised, XI_sorted, YI, extrap_val); }


  if( (XI_is_sorted == false) && (YI.n_elem > 0) )
    {
    Mat<eT> YI_unsorted;

    YI_unsorted.copy_size(YI);

    const eT* YI_mem          = YI.memptr();
          eT* YI_unsorted_mem = YI_unsorted.memptr();

    const uword  N              = XI_sorted.n_elem;
    const uword* XI_indices_mem = XI_indices.memptr();

    for(uword i=0; i<N; ++i)
      {
      YI_unsorted_mem[ XI_indices_mem[i] ] = YI_mem[i];
      }

    YI.steal_mem(YI_unsorted);
    }
  }



template<typename T1, typename T2, typename T3>
inline
typename
enable_if2
  <
  is_real<typename T1::elem_type>::value,
  void
  >::result
interp1
  (
  const Base<typename T1::elem_type, T1>& X,
  const Base<typename T1::elem_type, T2>& Y,
  const Base<typename T1::elem_type, T3>& XI,
         Mat<typename T1::elem_type>&     YI,
  const char*                             method     = "linear",
  const typename T1::elem_type            extrap_val = Datum<typename T1::elem_type>::nan
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  uword sig = 0;

  if(method    != NULL   )
  if(method[0] != char(0))
  if(method[1] != char(0))
    {
    const char c1 = method[0];
    const char c2 = method[1];

         if(c1 == 'n')  { sig = 10; }  // nearest neighbour
    else if(c1 == 'l')  { sig = 20; }  // linear
    else
      {
      if( (c1 == '*') && (c2 == 'n') )  { sig = 11; }  // nearest neighour, assume monotonic increase in X and XI
      if( (c1 == '*') && (c2 == 'l') )  { sig = 21; }  // linear, assume monotonic increase in X and XI
      }
    }

  arma_debug_check( (sig == 0), "interp1(): unsupported interpolation type" );

  const quasi_unwrap<T1>  X_tmp( X.get_ref());
  const quasi_unwrap<T2>  Y_tmp( Y.get_ref());
  const quasi_unwrap<T3> XI_tmp(XI.get_ref());

  if( X_tmp.is_alias(YI) || Y_tmp.is_alias(YI) || XI_tmp.is_alias(YI) )
    {
    Mat<eT> tmp;

    interp1_helper(X_tmp.M, Y_tmp.M, XI_tmp.M, tmp, sig, extrap_val);

    YI.steal_mem(tmp);
    }
  else
    {
    interp1_helper(X_tmp.M, Y_tmp.M, XI_tmp.M, YI, sig, extrap_val);
    }
  }



//! @}
