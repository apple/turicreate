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


//! \addtogroup glue_hist
//! @{


template<typename eT>
inline
void
glue_hist::apply_noalias(Mat<uword>& out, const Mat<eT>& X, const Mat<eT>& C, const uword dim)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ((C.is_vec() == false) && (C.is_empty() == false)), "hist(): parameter 'centers' must be a vector" );

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  const uword C_n_elem = C.n_elem;

  if( C_n_elem == 0 )  { out.reset(); return; }

  const eT* C_mem    = C.memptr();
  const eT  center_0 = C_mem[0];

  if(dim == 0)
    {
    out.zeros(C_n_elem, X_n_cols);

    for(uword col=0; col < X_n_cols; ++col)
      {
      const eT*    X_coldata   = X.colptr(col);
            uword* out_coldata = out.colptr(col);

      for(uword row=0; row < X_n_rows; ++row)
        {
        const eT val = X_coldata[row];

        if(arma_isfinite(val))
          {
          eT    opt_dist  = (center_0 >= val) ? (center_0 - val) : (val - center_0);
          uword opt_index = 0;

          for(uword j=1; j < C_n_elem; ++j)
            {
            const eT center = C_mem[j];
            const eT dist   = (center >= val) ? (center - val) : (val - center);

            if(dist < opt_dist)
              {
              opt_dist  = dist;
              opt_index = j;
              }
            else
              {
              break;
              }
            }

          out_coldata[opt_index]++;
          }
        else
          {
          // -inf
          if(val < eT(0)) { out_coldata[0]++; }

          // +inf
          if(val > eT(0)) { out_coldata[C_n_elem-1]++; }

          // ignore NaN
          }
        }
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(X_n_rows, C_n_elem);

    if(X_n_rows == 1)
      {
      const uword  X_n_elem = X.n_elem;
      const eT*    X_mem    = X.memptr();
            uword* out_mem  = out.memptr();

      for(uword i=0; i < X_n_elem; ++i)
        {
        const eT val = X_mem[i];

        if(is_finite(val))
          {
          eT    opt_dist  = (val >= center_0) ? (val - center_0) : (center_0 - val);
          uword opt_index = 0;

          for(uword j=1; j < C_n_elem; ++j)
            {
            const eT center = C_mem[j];
            const eT dist   = (val >= center) ? (val - center) : (center - val);

            if(dist < opt_dist)
              {
              opt_dist  = dist;
              opt_index = j;
              }
            else
              {
              break;
              }
            }

          out_mem[opt_index]++;
          }
        else
          {
          // -inf
          if(val < eT(0)) { out_mem[0]++; }

          // +inf
          if(val > eT(0)) { out_mem[C_n_elem-1]++; }

          // ignore NaN
          }
        }
      }
    else
      {
      for(uword row=0; row < X_n_rows; ++row)
        {
        for(uword col=0; col < X_n_cols; ++col)
          {
          const eT val = X.at(row,col);

          if(arma_isfinite(val))
            {
            eT    opt_dist  = (center_0 >= val) ? (center_0 - val) : (val - center_0);
            uword opt_index = 0;

            for(uword j=1; j < C_n_elem; ++j)
              {
              const eT center = C_mem[j];
              const eT dist   = (center >= val) ? (center - val) : (val - center);

              if(dist < opt_dist)
                {
                opt_dist  = dist;
                opt_index = j;
                }
              else
                {
                break;
                }
              }

            out.at(row,opt_index)++;
            }
          else
            {
            // -inf
            if(val < eT(0)) { out.at(row,0)++; }

            // +inf
            if(val > eT(0)) { out.at(row,C_n_elem-1)++; }

            // ignore NaN
            }
          }
        }
      }
    }
  }



template<typename T1, typename T2>
inline
void
glue_hist::apply(Mat<uword>& out, const mtGlue<uword,T1,T2,glue_hist>& expr)
  {
  arma_extra_debug_sigprint();

  const uword dim = expr.aux_uword;

  arma_debug_check( (dim > 1), "hist(): parameter 'dim' must be 0 or 1" );

  const quasi_unwrap<T1> UA(expr.A);
  const quasi_unwrap<T2> UB(expr.B);

  if(UA.is_alias(out) || UB.is_alias(out))
    {
    Mat<uword> tmp;

    glue_hist::apply_noalias(tmp, UA.M, UB.M, dim);

    out.steal_mem(tmp);
    }
  else
    {
    glue_hist::apply_noalias(out, UA.M, UB.M, dim);
    }
  }



template<typename T1, typename T2>
inline
void
glue_hist_default::apply(Mat<uword>& out, const mtGlue<uword,T1,T2,glue_hist_default>& expr)
  {
  arma_extra_debug_sigprint();

  const quasi_unwrap<T1> UA(expr.A);
  const quasi_unwrap<T2> UB(expr.B);

  //const uword dim = ( (T1::is_row) || ((UA.M.vec_state == 0) && (UA.M.n_elem <= 1) && (out.vec_state == 2)) ) ? 1 : 0;
  const uword dim = (T1::is_row) ? 1 : 0;

  if(UA.is_alias(out) || UB.is_alias(out))
    {
    Mat<uword> tmp;

    glue_hist::apply_noalias(tmp, UA.M, UB.M, dim);

    out.steal_mem(tmp);
    }
  else
    {
    glue_hist::apply_noalias(out, UA.M, UB.M, dim);
    }
  }


//! @}
