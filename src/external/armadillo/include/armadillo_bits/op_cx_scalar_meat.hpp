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


//! \addtogroup op_cx_scalar
//! @{



template<typename T1>
inline
void
op_cx_scalar_times::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_times>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] * k;
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = A.at(row,col) * k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_plus::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] + k;
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = A.at(row,col) + k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_minus_pre::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = k - A[i];
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = k - A.at(row,col);  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_minus_post::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] - k;
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = A.at(row,col) - k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_div_pre::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_pre>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = k / A[i];
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = k / A.at(row,col);  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_div_post::apply
  (
        Mat< typename std::complex<typename T1::pod_type> >& out,
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_post>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const Proxy<T1> A(X.m);

  const uword n_rows = A.get_n_rows();
  const uword n_cols = A.get_n_cols();

  out.set_size(n_rows, n_cols);

  const eT  k       = X.aux_out_eT;
        eT* out_mem = out.memptr();

  if(Proxy<T1>::use_at == false)
    {
    const uword n_elem = A.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] / k;
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      *out_mem = A.at(row,col) / k;  ++out_mem;
      }
    }
  }



//
//
//



template<typename T1>
inline
void
op_cx_scalar_times::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_times>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] * k;
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = A.at(row,col,slice) * k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_plus::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] + k;
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = A.at(row,col,slice) + k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_minus_pre::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = k - A[i];
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = k - A.at(row,col,slice);  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_minus_post::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] - k;
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = A.at(row,col,slice) - k;  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_div_pre::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_pre>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = k / A[i];
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = k / A.at(row,col,slice);  ++out_mem;
      }
    }
  }



template<typename T1>
inline
void
op_cx_scalar_div_post::apply
  (
           Cube< typename std::complex<typename T1::pod_type> >& out,
  const mtOpCube<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_div_post>& X
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<typename T1::pod_type> eT;

  const ProxyCube<T1> A(X.m);

  const uword n_rows   = A.get_n_rows();
  const uword n_cols   = A.get_n_cols();
  const uword n_slices = A.get_n_slices();

  out.set_size(n_rows, n_cols, n_slices);

  const eT    k       = X.aux_out_eT;
  const uword n_elem  = out.n_elem;
        eT*   out_mem = out.memptr();

  if(ProxyCube<T1>::use_at == false)
    {
    for(uword i=0; i<n_elem; ++i)
      {
      out_mem[i] = A[i] / k;
      }
    }
  else
    {
    for(uword slice = 0; slice < n_slices; ++slice)
    for(uword col   = 0; col   < n_cols;   ++col  )
    for(uword row   = 0; row   < n_rows;   ++row  )
      {
      *out_mem = A.at(row,col,slice) / k;  ++out_mem;
      }
    }
  }



//! @}
