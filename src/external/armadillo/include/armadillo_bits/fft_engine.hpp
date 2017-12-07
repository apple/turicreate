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
//
// ------------------------------------------------------------------------
//
// This file includes portions of Kiss FFT software,
// licensed under the following conditions.
//
// Copyright (c) 2003-2010 Mark Borgerding
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the author nor the names of any contributors may be used to
//   endorse or promote products derived from this software without specific
//   prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ------------------------------------------------------------------------


//! \addtogroup fft_engine
//! @{


template<typename cx_type, uword fixed_N, bool> struct store {};

template<typename cx_type, uword fixed_N>
struct store<cx_type, fixed_N, true>
  {
  static const uword N = fixed_N;

  arma_aligned cx_type coeffs_array[fixed_N];

  inline store()      {}
  inline store(uword) {}

  arma_inline       cx_type* coeffs_ptr()       { return &coeffs_array[0]; }
  arma_inline const cx_type* coeffs_ptr() const { return &coeffs_array[0]; }
  };



template<typename cx_type, uword fixed_N>
struct store<cx_type, fixed_N, false>
  {
  const uword N;

  podarray<cx_type> coeffs_array;

  inline store()           : N(0)    {}
  inline store(uword in_N) : N(in_N) { coeffs_array.set_size(N); }

  arma_inline       cx_type* coeffs_ptr()       { return coeffs_array.memptr(); }
  arma_inline const cx_type* coeffs_ptr() const { return coeffs_array.memptr(); }
  };



template<typename cx_type, bool inverse, uword fixed_N = 0>
class fft_engine : public store<cx_type, fixed_N, (fixed_N > 0)>
  {
  public:

  typedef typename get_pod_type<cx_type>::result T;

  using store<cx_type, fixed_N, (fixed_N > 0)>::N;
  using store<cx_type, fixed_N, (fixed_N > 0)>::coeffs_ptr;

  podarray<uword>   residue;
  podarray<uword>   radix;

  podarray<cx_type> tmp_array;


  template<bool fill>
  inline
  uword
  calc_radix()
    {
    uword i = 0;

    for(uword n = N, r=4; n >= 2; ++i)
      {
      while( (n % r) > 0 )
        {
        switch(r)
          {
          case 2:  r  = 3; break;
          case 4:  r  = 2; break;
          default: r += 2; break;
          }

        if(r*r > n) { r = n; }
        }

      n /= r;

      if(fill)
        {
        residue[i] = n;
          radix[i] = r;
        }
      }

    return i;
    }



  inline
  fft_engine(const uword in_N)
    : store< cx_type, fixed_N, (fixed_N > 0) >(in_N)
    {
    arma_extra_debug_sigprint();

    const uword len = calc_radix<false>();

    residue.set_size(len);
      radix.set_size(len);

    calc_radix<true>();


    // calculate the constant coefficients

    cx_type* coeffs = coeffs_ptr();

    const T k = T( (inverse) ? +2 : -2 ) * std::acos( T(-1) ) / T(N);

    for(uword i=0; i < N; ++i)  { coeffs[i] = std::exp( cx_type(T(0), i*k) ); }
    }



  arma_hot
  inline
  void
  butterfly_2(cx_type* Y, const uword stride, const uword m)
    {
    arma_extra_debug_sigprint();

    const cx_type* coeffs = coeffs_ptr();

    for(uword i=0; i < m; ++i)
      {
      const cx_type t = Y[i+m] * coeffs[i*stride];

      Y[i+m] =  Y[i] - t;
      Y[i  ] += t;
      }
    }



  arma_hot
  inline
  void
  butterfly_3(cx_type* Y, const uword stride, const uword m)
    {
    arma_extra_debug_sigprint();

    arma_aligned cx_type tmp[5];

    cx_type* coeffs1 = coeffs_ptr();
    cx_type* coeffs2 = coeffs1;

    const T coeff_sm_imag = coeffs1[stride*m].imag();

    const uword n = m*2;

    // TODO: rearrange the indices within tmp[] into a more sane order

    for(uword i = m; i > 0; --i)
      {
      tmp[1] = Y[m] * (*coeffs1);
      tmp[2] = Y[n] * (*coeffs2);

      tmp[0]  = tmp[1] - tmp[2];
      tmp[0] *= coeff_sm_imag;

      tmp[3] = tmp[1] + tmp[2];

      Y[m] = cx_type( (Y[0].real() - (T(0.5)*tmp[3].real())), (Y[0].imag() - (T(0.5)*tmp[3].imag())) );

      Y[0] += tmp[3];


      Y[n] = cx_type( (Y[m].real() + tmp[0].imag()), (Y[m].imag() - tmp[0].real()) );

      Y[m] += cx_type( -tmp[0].imag(), tmp[0].real() );

      Y++;

      coeffs1 += stride;
      coeffs2 += stride*2;
      }
    }



  arma_hot
  inline
  void
  butterfly_4(cx_type* Y, const uword stride, const uword m)
    {
    arma_extra_debug_sigprint();

    arma_aligned cx_type tmp[7];

    const cx_type* coeffs = coeffs_ptr();

    const uword m2 = m*2;
    const uword m3 = m*3;

    // TODO: rearrange the indices within tmp[] into a more sane order

    for(uword i=0; i < m; ++i)
      {
      tmp[0] = Y[i + m ] * coeffs[i*stride  ];
      tmp[2] = Y[i + m3] * coeffs[i*stride*3];
      tmp[3] = tmp[0] + tmp[2];

      //tmp[4] = tmp[0] - tmp[2];
      //tmp[4] = (inverse) ? cx_type( -(tmp[4].imag()), tmp[4].real() ) : cx_type( tmp[4].imag(), -tmp[4].real() );

      tmp[4] = (inverse)
                 ? cx_type( (tmp[2].imag() - tmp[0].imag()), (tmp[0].real() - tmp[2].real()) )
                 : cx_type( (tmp[0].imag() - tmp[2].imag()), (tmp[2].real() - tmp[0].real()) );

      tmp[1] = Y[i + m2] * coeffs[i*stride*2];
      tmp[5] = Y[i] - tmp[1];


      Y[i     ] += tmp[1];
      Y[i + m2]  = Y[i] - tmp[3];
      Y[i     ] += tmp[3];
      Y[i + m ]  = tmp[5] + tmp[4];
      Y[i + m3]  = tmp[5] - tmp[4];
      }
    }



  inline
  arma_hot
  void
  butterfly_5(cx_type* Y, const uword stride, const uword m)
    {
    arma_extra_debug_sigprint();

    arma_aligned cx_type tmp[13];

    const cx_type* coeffs = coeffs_ptr();

    const T a_real = coeffs[stride*1*m].real();
    const T a_imag = coeffs[stride*1*m].imag();

    const T b_real = coeffs[stride*2*m].real();
    const T b_imag = coeffs[stride*2*m].imag();

    cx_type* Y0 = Y;
    cx_type* Y1 = Y + 1*m;
    cx_type* Y2 = Y + 2*m;
    cx_type* Y3 = Y + 3*m;
    cx_type* Y4 = Y + 4*m;

    for(uword i=0; i < m; ++i)
      {
      tmp[0] = (*Y0);

      tmp[1] = (*Y1) * coeffs[stride*1*i];
      tmp[2] = (*Y2) * coeffs[stride*2*i];
      tmp[3] = (*Y3) * coeffs[stride*3*i];
      tmp[4] = (*Y4) * coeffs[stride*4*i];

      tmp[7]  = tmp[1] + tmp[4];
      tmp[8]  = tmp[2] + tmp[3];
      tmp[9]  = tmp[2] - tmp[3];
      tmp[10] = tmp[1] - tmp[4];

      (*Y0) += tmp[7];
      (*Y0) += tmp[8];

      tmp[5] = tmp[0] + cx_type( ( (tmp[7].real() * a_real) + (tmp[8].real() * b_real) ), ( (tmp[7].imag() * a_real) + (tmp[8].imag() * b_real) ) );

      tmp[6] =  cx_type( ( (tmp[10].imag() * a_imag) + (tmp[9].imag() * b_imag) ), ( -(tmp[10].real() * a_imag) - (tmp[9].real() * b_imag) ) );

      (*Y1) = tmp[5] - tmp[6];
      (*Y4) = tmp[5] + tmp[6];

      tmp[11] = tmp[0] +  cx_type( ( (tmp[7].real() * b_real) + (tmp[8].real() * a_real) ), ( (tmp[7].imag() * b_real) + (tmp[8].imag() * a_real) ) );

      tmp[12] = cx_type( ( -(tmp[10].imag() * b_imag) + (tmp[9].imag() * a_imag) ), (  (tmp[10].real() * b_imag) - (tmp[9].real() * a_imag) ) );

      (*Y2) = tmp[11] + tmp[12];
      (*Y3) = tmp[11] - tmp[12];

      Y0++;
      Y1++;
      Y2++;
      Y3++;
      Y4++;
      }
    }



  arma_hot
  inline
  void
  butterfly_N(cx_type* Y, const uword stride, const uword m, const uword r)
    {
    arma_extra_debug_sigprint();

    const cx_type* coeffs = coeffs_ptr();

    tmp_array.set_min_size(r);
    cx_type* tmp = tmp_array.memptr();

    for(uword u=0; u < m; ++u)
      {
      uword k = u;

      for(uword v=0; v < r; ++v)
        {
        tmp[v] = Y[k];
        k += m;
        }

      k = u;

      for(uword v=0; v < r; ++v)
        {
        Y[k] = tmp[0];

        uword j = 0;

        for(uword w=1; w < r; ++w)
          {
          j += stride * k;

          if(j >= N) { j -= N; }

          Y[k] += tmp[w] * coeffs[j];
          }

        k += m;
        }
      }
    }



  inline
  void
  run(cx_type* Y, const cx_type* X, const uword stage = 0, const uword stride = 1)
    {
    arma_extra_debug_sigprint();

    const uword m = residue[stage];
    const uword r =   radix[stage];

    const cx_type *Y_end = Y + r*m;

    if(m == 1)
      {
      for(cx_type* Yi = Y; Yi != Y_end; Yi++, X += stride)  {  (*Yi) = (*X);  }
      }
    else
      {
      const uword next_stage  = stage + 1;
      const uword next_stride = stride * r;

      for(cx_type* Yi = Y; Yi != Y_end; Yi += m, X += stride)  { run(Yi, X, next_stage, next_stride); }
      }

    switch(r)
      {
      case 2:  butterfly_2(Y, stride, m   );  break;
      case 3:  butterfly_3(Y, stride, m   );  break;
      case 4:  butterfly_4(Y, stride, m   );  break;
      case 5:  butterfly_5(Y, stride, m   );  break;
      default: butterfly_N(Y, stride, m, r);  break;
      }
    }


  };


//! @}
