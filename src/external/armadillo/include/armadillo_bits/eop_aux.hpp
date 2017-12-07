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


//! \addtogroup eop_aux
//! @{



//! use of the SFINAE approach to work around compiler limitations
//! http://en.wikipedia.org/wiki/SFINAE

class eop_aux
  {
  public:

  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    acos  (const eT x) { return eT( std::acos(double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    asin  (const eT x) { return eT( std::asin(double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    atan  (const eT x) { return eT( std::atan(double(x)) ); }

  template<typename eT> arma_inline static typename arma_real_only<eT>::result        acos  (const eT x) { return std::acos(x); }
  template<typename eT> arma_inline static typename arma_real_only<eT>::result        asin  (const eT x) { return std::asin(x); }
  template<typename eT> arma_inline static typename arma_real_only<eT>::result        atan  (const eT x) { return std::atan(x); }

  template<typename eT> arma_inline static typename arma_cx_only<eT>::result          acos  (const eT x) { return arma_acos(x); }
  template<typename eT> arma_inline static typename arma_cx_only<eT>::result          asin  (const eT x) { return arma_asin(x); }
  template<typename eT> arma_inline static typename arma_cx_only<eT>::result          atan  (const eT x) { return arma_atan(x); }

  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    acosh (const eT x) { return eT( arma_acosh(double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    asinh (const eT x) { return eT( arma_asinh(double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result    atanh (const eT x) { return eT( arma_atanh(double(x)) ); }

  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result acosh (const eT x) { return arma_acosh(x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result asinh (const eT x) { return arma_asinh(x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result atanh (const eT x) { return arma_atanh(x); }

  template<typename eT> arma_inline static typename arma_not_cx<eT>::result conj(const eT               x) { return x;            }
  template<typename  T> arma_inline static          std::complex<T>         conj(const std::complex<T>& x) { return std::conj(x); }

  template<typename eT> arma_inline static typename arma_integral_only<eT>::result sqrt  (const eT x) { return eT( std::sqrt (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result log10 (const eT x) { return eT( std::log10(double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result log   (const eT x) { return eT( std::log  (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result exp   (const eT x) { return eT( std::exp  (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result cos   (const eT x) { return eT( std::cos  (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result sin   (const eT x) { return eT( std::sin  (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result tan   (const eT x) { return eT( std::tan  (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result cosh  (const eT x) { return eT( std::cosh (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result sinh  (const eT x) { return eT( std::sinh (double(x)) ); }
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result tanh  (const eT x) { return eT( std::tanh (double(x)) ); }

  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result sqrt  (const eT x) { return std::sqrt (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result log10 (const eT x) { return std::log10(x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result log   (const eT x) { return std::log  (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result exp   (const eT x) { return std::exp  (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result cos   (const eT x) { return std::cos  (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result sin   (const eT x) { return std::sin  (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result tan   (const eT x) { return std::tan  (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result cosh  (const eT x) { return std::cosh (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result sinh  (const eT x) { return std::sinh (x); }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result tanh  (const eT x) { return std::tanh (x); }

  template<typename eT> arma_inline static typename arma_unsigned_integral_only<eT>::result neg (const eT x) { return  x; }
  template<typename eT> arma_inline static typename            arma_signed_only<eT>::result neg (const eT x) { return -x; }

  template<typename eT> arma_inline static typename arma_integral_only<eT>::result floor (const eT  x) { return x;                                                }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result floor (const eT  x) { return std::floor(x);                                    }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result floor (const eT& x) { return eT( std::floor(x.real()), std::floor(x.imag()) ); }

  template<typename eT> arma_inline static typename arma_integral_only<eT>::result ceil  (const eT  x) { return x;                                                }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result ceil  (const eT  x) { return std::ceil(x);                                     }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result ceil  (const eT& x) { return eT( std::ceil(x.real()), std::ceil(x.imag()) );   }


  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result round (const eT  x) { return x;                                                        }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result round (const eT  x) { return std::round(x);                                            }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result round (const eT& x) { return eT( std::round(x.real()), std::round(x.imag()) );         }
  #else
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result round (const eT  x) { return x;                                                        }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result round (const eT  x) { return (x >= eT(0)) ? std::floor(x+0.5) : std::ceil(x-0.5);      }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result round (const eT& x) { return eT( eop_aux::round(x.real()), eop_aux::round(x.imag()) ); }
  #endif


  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result trunc (const eT  x) { return x;                                                        }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result trunc (const eT  x) { return std::trunc(x);                                            }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result trunc (const eT& x) { return eT( std::trunc(x.real()), std::trunc(x.imag()) );         }
  #else
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result trunc (const eT  x) { return x;                                                        }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result trunc (const eT  x) { return (x >= eT(0)) ? std::floor(x) : std::ceil(x);              }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result trunc (const eT& x) { return eT( eop_aux::trunc(x.real()), eop_aux::trunc(x.imag()) ); }
  #endif


  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename   arma_integral_only<eT>::result log2 (const eT  x) { return eT( std::log(double(x))/ double(0.69314718055994530942) );                            }
  template<typename eT> arma_inline static typename       arma_real_only<eT>::result log2 (const eT  x) { return std::log2(x);                                                                         }
  template<typename eT> arma_inline static typename         arma_cx_only<eT>::result log2 (const eT& x) { typedef typename get_pod_type<eT>::result T; return std::log(x) / T(0.69314718055994530942); }
  #else
  template<typename eT> arma_inline static typename   arma_integral_only<eT>::result log2 (const eT  x) { return eT( std::log(double(x))/ double(0.69314718055994530942) );                            }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result log2 (const eT  x) { typedef typename get_pod_type<eT>::result T; return std::log(x) / T(0.69314718055994530942); }
  #endif


  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename   arma_integral_only<eT>::result exp2 (const eT  x) { return eT( std::pow(double(2), double(x)) );                            }
  template<typename eT> arma_inline static typename       arma_real_only<eT>::result exp2 (const eT  x) { return std::exp2(x);                                                    }
  template<typename eT> arma_inline static typename         arma_cx_only<eT>::result exp2 (const eT& x) { typedef typename get_pod_type<eT>::result T; return std::pow( T(2), x); }
  #else
  template<typename eT> arma_inline static typename   arma_integral_only<eT>::result exp2 (const eT  x) { return eT( std::pow(double(2), double(x)) );                            }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result exp2 (const eT  x) { typedef typename get_pod_type<eT>::result T; return std::pow( T(2), x); }
  #endif


  template<typename eT> arma_inline static typename   arma_integral_only<eT>::result exp10 (const eT x) { return eT( std::pow(double(10), double(x)) );                            }
  template<typename eT> arma_inline static typename arma_real_or_cx_only<eT>::result exp10 (const eT x) { typedef typename get_pod_type<eT>::result T; return std::pow( T(10), x); }

  template<typename eT> arma_inline static typename arma_unsigned_integral_only<eT>::result arma_abs (const eT               x) { return x;           }
  template<typename eT> arma_inline static typename   arma_signed_integral_only<eT>::result arma_abs (const eT               x) { return std::abs(x); }
  template<typename eT> arma_inline static typename              arma_real_only<eT>::result arma_abs (const eT               x) { return std::abs(x); }
  template<typename  T> arma_inline static typename              arma_real_only< T>::result arma_abs (const std::complex<T>& x) { return std::abs(x); }

  template<typename eT> arma_inline static typename arma_unsigned_integral_only<eT>::result sign (const eT  x) { return (x > eT(0)) ? eT(+1) : eT(0);                                                                      }
  template<typename eT> arma_inline static typename   arma_signed_integral_only<eT>::result sign (const eT  x) { return (x > eT(0)) ? eT(+1) : ( (x < eT(0)) ? eT(-1) : eT(0) );                                           }
  template<typename eT> arma_inline static typename              arma_real_only<eT>::result sign (const eT  x) { return (x > eT(0)) ? eT(+1) : ( (x < eT(0)) ? eT(-1) : eT(0) );                                           }
  template<typename eT> arma_inline static typename                arma_cx_only<eT>::result sign (const eT& x) { typedef typename eT::value_type T; return (x.real() != T(0) && x.imag() != T(0)) ? (x / std::abs(x)) : x; }

  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result erf (const eT  x) { return eT( std::erf(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result erf (const eT  x) { return std::erf(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result erf (const eT& x) { arma_ignore(x); return eT(0);     }
  #elif defined(ARMA_HAVE_TR1)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result erf (const eT  x) { return eT( std::tr1::erf(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result erf (const eT  x) { return std::tr1::erf(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result erf (const eT& x) { arma_ignore(x); return eT(0);          }
  #else
  template<typename eT> arma_inline static                                      eT erf (const eT  x) { arma_ignore(x); arma_stop_logic_error("erf(): need C++11 compiler"); return eT(0); }
  #endif

  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result erfc (const eT  x) { return eT( std::erfc(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result erfc (const eT  x) { return std::erfc(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result erfc (const eT& x) { arma_ignore(x); return eT(0);      }
  #elif defined(ARMA_HAVE_TR1)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result erfc (const eT  x) { return eT( std::tr1::erfc(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result erfc (const eT  x) { return std::tr1::erfc(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result erfc (const eT& x) { arma_ignore(x); return eT(0);           }
  #else
  template<typename eT> arma_inline static                                      eT erfc (const eT  x) { arma_ignore(x); arma_stop_logic_error("erfc(): need C++11 compiler"); return eT(0); }
  #endif

  #if defined(ARMA_USE_CXX11)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result lgamma (const eT  x) { return eT( std::lgamma(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result lgamma (const eT  x) { return std::lgamma(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result lgamma (const eT& x) { arma_ignore(x); return eT(0);        }
  #elif defined(ARMA_HAVE_TR1)
  template<typename eT> arma_inline static typename arma_integral_only<eT>::result lgamma (const eT  x) { return eT( std::tr1::lgamma(double(x)) ); }
  template<typename eT> arma_inline static typename     arma_real_only<eT>::result lgamma (const eT  x) { return std::tr1::lgamma(x);               }
  template<typename eT> arma_inline static typename       arma_cx_only<eT>::result lgamma (const eT& x) { arma_ignore(x); return eT(0);             }
  #else
  template<typename eT> arma_inline static                                      eT lgamma (const eT  x) { arma_ignore(x); arma_stop_logic_error("lgamma(): need C++11 compiler"); return eT(0); }
  #endif


  template<typename T1, typename T2> arma_inline static typename   arma_integral_only<T1>::result pow (const T1 base, const T2 exponent) { return T1( std::pow( double(base), double(exponent) ) ); }
  template<typename T1, typename T2> arma_inline static typename arma_real_or_cx_only<T1>::result pow (const T1 base, const T2 exponent) { return std::pow(base, exponent);                         }


  template<typename eT>
  arma_inline
  static
  typename arma_integral_only<eT>::result
  direct_eps(const eT)
    {
    return eT(0);
    }


  template<typename eT>
  inline
  static
  typename arma_real_only<eT>::result
  direct_eps(const eT x)
    {
    //arma_extra_debug_sigprint();

    // acording to IEEE Standard for Floating-Point Arithmetic (IEEE 754)
    // the mantissa length for double is 53 bits = std::numeric_limits<double>::digits
    // the mantissa length for float  is 24 bits = std::numeric_limits<float >::digits

    //return std::pow( std::numeric_limits<eT>::radix, (std::floor(std::log10(std::abs(x))/std::log10(std::numeric_limits<eT>::radix))-(std::numeric_limits<eT>::digits-1)) );

    const eT radix_eT     = eT(std::numeric_limits<eT>::radix);
    const eT digits_m1_eT = eT(std::numeric_limits<eT>::digits - 1);

    // return std::pow( radix_eT, eT(std::floor(std::log10(std::abs(x))/std::log10(radix_eT)) - digits_m1_eT) );
    return eop_aux::pow( radix_eT, eT(std::floor(std::log10(std::abs(x))/std::log10(radix_eT)) - digits_m1_eT) );
    }


  template<typename T>
  inline
  static
  typename arma_real_only<T>::result
  direct_eps(const std::complex<T>& x)
    {
    //arma_extra_debug_sigprint();

    //return std::pow( std::numeric_limits<T>::radix, (std::floor(std::log10(std::abs(x))/std::log10(std::numeric_limits<T>::radix))-(std::numeric_limits<T>::digits-1)) );

    const T radix_T     = T(std::numeric_limits<T>::radix);
    const T digits_m1_T = T(std::numeric_limits<T>::digits - 1);

    return std::pow( radix_T, T(std::floor(std::log10(std::abs(x))/std::log10(radix_T)) - digits_m1_T) );
    }
  };



//! @}
