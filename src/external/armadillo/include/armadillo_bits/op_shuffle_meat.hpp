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



//! \addtogroup op_shuffle
//! @{



template<typename eT>
inline
void
op_shuffle::apply_direct(Mat<eT>& out, const Mat<eT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  if(X.is_empty()) { out.copy_size(X); return; }

  const uword N = (dim == 0) ? X.n_rows : X.n_cols;


  // see op_sort_index_bones.hpp for the definition of arma_sort_index_packet
  // and the associated comparison functor
  std::vector< arma_sort_index_packet<int> > packet_vec(N);

  for(uword i=0; i<N; ++i)
    {
    packet_vec[i].val   = int(arma_rng::randi<int>());
    packet_vec[i].index = i;
    }

  arma_sort_index_helper_ascend<int> comparator;

  std::sort( packet_vec.begin(), packet_vec.end(), comparator );

  const bool is_alias = (&out == &X);

  if(X.is_vec() == false)
    {
    if(is_alias == false)
      {
      arma_extra_debug_print("op_shuffle::apply(): matrix");

      out.copy_size(X);

      if(dim == 0)
        {
        for(uword i=0; i<N; ++i) { out.row(i) = X.row(packet_vec[i].index); }
        }
      else
        {
        for(uword i=0; i<N; ++i) { out.col(i) = X.col(packet_vec[i].index); }
        }
      }
    else  // in-place shuffle
      {
      arma_extra_debug_print("op_shuffle::apply(): in-place matrix");

      // reuse the val member variable of packet_vec
      // to indicate whether a particular row or column
      // has already been shuffled

      for(uword i=0; i<N; ++i)
        {
        packet_vec[i].val = 0;
        }

      if(dim == 0)
        {
        for(uword i=0; i<N; ++i)
          {
          if(packet_vec[i].val == 0)
            {
            const uword j = packet_vec[i].index;

            out.swap_rows(i, j);

            packet_vec[j].val = 1;
            }
          }
        }
      else
        {
        for(uword i=0; i<N; ++i)
          {
          if(packet_vec[i].val == 0)
            {
            const uword j = packet_vec[i].index;

            out.swap_cols(i, j);

            packet_vec[j].val = 1;
            }
          }
        }
      }
    }
  else  // we're dealing with a vector
    {
    if(is_alias == false)
      {
      arma_extra_debug_print("op_shuffle::apply(): vector");

      out.copy_size(X);

      if(dim == 0)
        {
        if(X.n_rows > 1)  // i.e. column vector
          {
          for(uword i=0; i<N; ++i) { out[i] = X[ packet_vec[i].index ]; }
          }
        else
          {
          out = X;
          }
        }
      else
        {
        if(X.n_cols > 1)  // i.e. row vector
          {
          for(uword i=0; i<N; ++i) { out[i] = X[ packet_vec[i].index ]; }
          }
        else
          {
          out = X;
          }
        }
      }
    else  // in-place shuffle
      {
      arma_extra_debug_print("op_shuffle::apply(): in-place vector");

      // reuse the val member variable of packet_vec
      // to indicate whether a particular row or column
      // has already been shuffled

      for(uword i=0; i<N; ++i)
        {
        packet_vec[i].val = 0;
        }

      if(dim == 0)
        {
        if(X.n_rows > 1)  // i.e. column vector
          {
          for(uword i=0; i<N; ++i)
            {
            if(packet_vec[i].val == 0)
              {
              const uword j = packet_vec[i].index;

              std::swap(out[i], out[j]);

              packet_vec[j].val = 1;
              }
            }
          }
        }
      else
        {
        if(X.n_cols > 1)  // i.e. row vector
          {
          for(uword i=0; i<N; ++i)
            {
            if(packet_vec[i].val == 0)
              {
              const uword j = packet_vec[i].index;

              std::swap(out[i], out[j]);

              packet_vec[j].val = 1;
              }
            }
          }
        }
      }
    }

  }



template<typename T1>
inline
void
op_shuffle::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_shuffle>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> U(in.m);

  const uword dim = in.aux_uword_a;

  arma_debug_check( (dim > 1), "shuffle(): parameter 'dim' must be 0 or 1" );

  op_shuffle::apply_direct(out, U.M, dim);
  }



template<typename T1>
inline
void
op_shuffle_default::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_shuffle_default>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> U(in.m);

  const uword dim = (T1::is_row) ? 1 : 0;

  op_shuffle::apply_direct(out, U.M, dim);
  }



//! @}
