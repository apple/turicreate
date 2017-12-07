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


//! \addtogroup upgrade_val
//! @{



//! upgrade_val is used to ensure an operation such as multiplication is possible between two types.
//! values are upgraded only where necessary.

template<typename T1, typename T2>
struct upgrade_val
  {
  typedef typename promote_type<T1,T2>::result T1_result;
  typedef typename promote_type<T1,T2>::result T2_result;

  arma_inline
  static
  typename promote_type<T1,T2>::result
  apply(const T1 x)
    {
    typedef typename promote_type<T1,T2>::result out_type;
    return out_type(x);
    }

  arma_inline
  static
  typename promote_type<T1,T2>::result
  apply(const T2 x)
    {
    typedef typename promote_type<T1,T2>::result out_type;
    return out_type(x);
    }

  };


// template<>
template<typename T>
struct upgrade_val<T,T>
  {
  typedef T T1_result;
  typedef T T2_result;

  arma_inline static const T& apply(const T& x) { return x; }
  };


//! upgrade a type to allow multiplication with a complex type
//! e.g. the int in "int * complex<double>" is upgraded to a double
// template<>
template<typename T, typename T2>
struct upgrade_val< std::complex<T>, T2 >
  {
  typedef std::complex<T> T1_result;
  typedef T               T2_result;

  arma_inline static const std::complex<T>& apply(const std::complex<T>& x) { return x;    }
  arma_inline static       T                apply(const T2 x)               { return T(x); }
  };


// template<>
template<typename T1, typename T>
struct upgrade_val< T1, std::complex<T> >
  {
  typedef T               T1_result;
  typedef std::complex<T> T2_result;

  arma_inline static       T                apply(const T1 x)               { return T(x); }
  arma_inline static const std::complex<T>& apply(const std::complex<T>& x) { return x;    }
  };


//! ensure we don't lose precision when multiplying a complex number with a higher precision real number
template<>
struct upgrade_val< std::complex<float>, double >
  {
  typedef std::complex<double> T1_result;
  typedef double               T2_result;

  arma_inline static const std::complex<double> apply(const std::complex<float>& x) { return std::complex<double>(x); }
  arma_inline static       double               apply(const double x)               { return x; }
  };


template<>
struct upgrade_val< double, std::complex<float> >
  {
  typedef double              T1_result;
  typedef std::complex<float> T2_result;

  arma_inline static       double               apply(const double x)               { return x; }
  arma_inline static const std::complex<double> apply(const std::complex<float>& x) { return std::complex<double>(x); }
  };


//! ensure we don't lose precision when multiplying complex numbers with different underlying types
template<>
struct upgrade_val< std::complex<float>, std::complex<double> >
  {
  typedef std::complex<double> T1_result;
  typedef std::complex<double> T2_result;

  arma_inline static const std::complex<double>  apply(const std::complex<float>&  x) { return std::complex<double>(x); }
  arma_inline static const std::complex<double>& apply(const std::complex<double>& x) { return x; }
  };


template<>
struct upgrade_val< std::complex<double>, std::complex<float> >
  {
  typedef std::complex<double> T1_result;
  typedef std::complex<double> T2_result;

  arma_inline static const std::complex<double>& apply(const std::complex<double>& x) { return x; }
  arma_inline static const std::complex<double>  apply(const std::complex<float>&  x) { return std::complex<double>(x); }
  };


//! work around limitations in the complex class (at least as present in gcc 4.1 & 4.3)
template<>
struct upgrade_val< std::complex<double>, float >
  {
  typedef std::complex<double> T1_result;
  typedef double               T2_result;

  arma_inline static const std::complex<double>& apply(const std::complex<double>& x) { return x; }
  arma_inline static       double                apply(const float x)                 { return double(x); }
  };


template<>
struct upgrade_val< float, std::complex<double> >
  {
  typedef double               T1_result;
  typedef std::complex<double> T2_result;

  arma_inline static       double                apply(const float x)                 { return double(x); }
  arma_inline static const std::complex<double>& apply(const std::complex<double>& x) { return x; }
  };



//! @}
