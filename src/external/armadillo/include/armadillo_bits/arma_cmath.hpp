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



//! \addtogroup arma_cmath
//! @{



//
// wrappers for isfinite


template<typename eT>
arma_inline
bool
arma_isfinite(eT val)
  {
  arma_ignore(val);

  return true;
  }



template<>
arma_inline
bool
arma_isfinite(float x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isfinite(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::isfinite(x);
    }
  #elif defined(ARMA_HAVE_ISFINITE)
    {
    return (std::isfinite(x) != 0);
    }
  #else
    {
    const float y = (std::numeric_limits<float>::max)();

    const volatile float xx = x;

    return (xx == xx) && (x >= -y) && (x <= y);
    }
  #endif
  }



template<>
arma_inline
bool
arma_isfinite(double x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isfinite(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::isfinite(x);
    }
  #elif defined(ARMA_HAVE_ISFINITE)
    {
    return (std::isfinite(x) != 0);
    }
  #else
    {
    const double y = (std::numeric_limits<double>::max)();

    const volatile double xx = x;

    return (xx == xx) && (x >= -y) && (x <= y);
    }
  #endif
  }



template<typename T>
arma_inline
bool
arma_isfinite(const std::complex<T>& x)
  {
  if( (arma_isfinite(x.real()) == false) || (arma_isfinite(x.imag()) == false) )
    {
    return false;
    }
  else
    {
    return true;
    }
  }



//
// wrappers for isinf


template<typename eT>
arma_inline
bool
arma_isinf(eT val)
  {
  arma_ignore(val);

  return false;
  }



template<>
arma_inline
bool
arma_isinf(float x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isinf(x);
    }
  #elif defined(ARMA_HAVE_ISINF)
    {
    return (std::isinf(x) != 0);
    }
  #else
    {
    const float y = (std::numeric_limits<float>::max)();

    const volatile float xx = x;

    return (xx == xx) && ((x < -y) || (x > y));
    }
  #endif
  }



template<>
arma_inline
bool
arma_isinf(double x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isinf(x);
    }
  #elif defined(ARMA_HAVE_ISINF)
    {
    return (std::isinf(x) != 0);
    }
  #else
    {
    const double y = (std::numeric_limits<double>::max)();

    const volatile double xx = x;

    return (xx == xx) && ((x < -y) || (x > y));
    }
  #endif
  }



template<typename T>
arma_inline
bool
arma_isinf(const std::complex<T>& x)
  {
  return ( arma_isinf(x.real()) || arma_isinf(x.imag()) );
  }



//
// wrappers for isnan


template<typename eT>
arma_inline
bool
arma_isnan(eT val)
  {
  arma_ignore(val);

  return false;
  }



template<>
arma_inline
bool
arma_isnan(float x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isnan(x);
    }
  #elif defined(ARMA_HAVE_ISNAN)
    {
    return (std::isnan(x) != 0);
    }
  #else
    {
    const volatile float xx = x;

    return (xx != xx);
    }
  #endif
  }



template<>
arma_inline
bool
arma_isnan(double x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::isnan(x);
    }
  #elif defined(ARMA_HAVE_ISNAN)
    {
    return (std::isnan(x) != 0);
    }
  #else
    {
    const volatile double xx = x;

    return (xx != xx);
    }
  #endif
  }



template<typename T>
arma_inline
bool
arma_isnan(const std::complex<T>& x)
  {
  return ( arma_isnan(x.real()) || arma_isnan(x.imag()) );
  }



// rudimentary wrappers for log1p()

arma_inline
float
arma_log1p(const float x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::log1p(x);
    }
  #else
    {
    if((x >= float(0)) && (x < std::numeric_limits<float>::epsilon()))
      {
      return x;
      }
    else
    if((x < float(0)) && (-x < std::numeric_limits<float>::epsilon()))
      {
      return x;
      }
    else
      {
      return std::log(float(1) + x);
      }
    }
  #endif
  }



arma_inline
double
arma_log1p(const double x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::log1p(x);
    }
  #elif defined(ARMA_HAVE_LOG1P)
    {
    return log1p(x);
    }
  #else
    {
    if((x >= double(0)) && (x < std::numeric_limits<double>::epsilon()))
      {
      return x;
      }
    else
    if((x < double(0)) && (-x < std::numeric_limits<double>::epsilon()))
      {
      return x;
      }
    else
      {
      return std::log(double(1) + x);
      }
    }
  #endif
  }





//
// wrappers for trigonometric functions
//
// wherever possible, try to use C++11 or TR1 versions of the following functions:
//
// complex acos
// complex asin
// complex atan
//
// real    acosh
// real    asinh
// real    atanh
//
// complex acosh
// complex asinh
// complex atanh
//
//
// if C++11 or TR1 are not available, we have rudimentary versions of:
//
// real    acosh
// real    asinh
// real    atanh



template<typename T>
arma_inline
std::complex<T>
arma_acos(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::acos(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::acos(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("acos(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



template<typename T>
arma_inline
std::complex<T>
arma_asin(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::asin(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::asin(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("asin(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



template<typename T>
arma_inline
std::complex<T>
arma_atan(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::atan(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::atan(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("atan(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



template<typename eT>
arma_inline
eT
arma_acosh(const eT x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::acosh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::acosh(x);
    }
  #else
    {
    if(x >= eT(1))
      {
      // http://functions.wolfram.com/ElementaryFunctions/ArcCosh/02/
      return std::log( x + std::sqrt(x*x - eT(1)) );
      }
    else
      {
      if(std::numeric_limits<eT>::has_quiet_NaN)
        {
        return -(std::numeric_limits<eT>::quiet_NaN());
        }
      else
        {
        return eT(0);
        }
      }
    }
  #endif
  }



template<typename eT>
arma_inline
eT
arma_asinh(const eT x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::asinh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::asinh(x);
    }
  #else
    {
    // http://functions.wolfram.com/ElementaryFunctions/ArcSinh/02/
    return std::log( x + std::sqrt(x*x + eT(1)) );
    }
  #endif
  }



template<typename eT>
arma_inline
eT
arma_atanh(const eT x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::atanh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::atanh(x);
    }
  #else
    {
    if( (x >= eT(-1)) && (x <= eT(+1)) )
      {
      // http://functions.wolfram.com/ElementaryFunctions/ArcTanh/02/
      return std::log( ( eT(1)+x ) / ( eT(1)-x ) ) / eT(2);
      }
    else
      {
      if(std::numeric_limits<eT>::has_quiet_NaN)
        {
        return -(std::numeric_limits<eT>::quiet_NaN());
        }
      else
        {
        return eT(0);
        }
      }
    }
  #endif
  }



template<typename T>
arma_inline
std::complex<T>
arma_acosh(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::acosh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::acosh(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("acosh(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



template<typename T>
arma_inline
std::complex<T>
arma_asinh(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::asinh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::asinh(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("asinh(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



template<typename T>
arma_inline
std::complex<T>
arma_atanh(const std::complex<T>& x)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::atanh(x);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::atanh(x);
    }
  #else
    {
    arma_ignore(x);
    arma_stop_logic_error("atanh(): need C++11 compiler");

    return std::complex<T>(0);
    }
  #endif
  }



//
// wrappers for hypot(x, y) = sqrt(x^2 + y^2)


template<typename eT>
inline
eT
arma_hypot_generic(const eT x, const eT y)
  {
  #if defined(ARMA_USE_CXX11)
    {
    return std::hypot(x, y);
    }
  #elif defined(ARMA_HAVE_TR1)
    {
    return std::tr1::hypot(x, y);
    }
  #else
    {
    const eT xabs = std::abs(x);
    const eT yabs = std::abs(y);

    eT larger;
    eT ratio;

    if(xabs > yabs)
      {
      larger = xabs;
      ratio  = yabs / xabs;
      }
    else
      {
      larger = yabs;
      ratio  = xabs / yabs;
      }

    return (larger == eT(0)) ? eT(0) : (larger * std::sqrt(eT(1) + ratio * ratio));
    }
  #endif
  }



template<typename eT>
inline
eT
arma_hypot(const eT x, const eT y)
  {
  arma_ignore(x);
  arma_ignore(y);

  arma_stop_runtime_error("arma_hypot(): not implemented for integer or complex element types");

  return eT(0);
  }



template<>
arma_inline
float
arma_hypot(const float x, const float y)
  {
  return arma_hypot_generic(x,y);
  }



template<>
arma_inline
double
arma_hypot(const double x, const double y)
  {
  return arma_hypot_generic(x,y);
  }



//
// wrappers for arg()


template<typename eT>
struct arma_arg
  {
  static
  inline
  eT
  eval(const eT x)
    {
    #if defined(ARMA_USE_CXX11)
      {
      return eT( std::arg(x) );
      }
    #else
      {
      arma_ignore(x);
      arma_stop_logic_error("arg(): need C++11 compiler");

      return eT(0);
      }
    #endif
    }
  };



template<>
struct arma_arg<float>
  {
  static
  arma_inline
  float
  eval(const float x)
    {
    #if defined(ARMA_USE_CXX11)
      {
      return std::arg(x);
      }
    #else
      {
      return std::arg( std::complex<float>( x, float(0) ) );
      }
    #endif
    }
  };



template<>
struct arma_arg<double>
  {
  static
  arma_inline
  double
  eval(const double x)
    {
    #if defined(ARMA_USE_CXX11)
      {
      return std::arg(x);
      }
    #else
      {
      return std::arg( std::complex<double>( x, double(0) ) );
      }
    #endif
    }
  };



template<>
struct arma_arg< std::complex<float> >
  {
  static
  arma_inline
  float
  eval(const std::complex<float>& x)
    {
    return std::arg(x);
    }
  };



template<>
struct arma_arg< std::complex<double> >
  {
  static
  arma_inline
  double
  eval(const std::complex<double>& x)
    {
    return std::arg(x);
    }
  };



//! @}
