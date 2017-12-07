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


//! \addtogroup fn_conv_to
//! @{



//! conversion from Armadillo Base and BaseCube objects to scalars
//! (kept only for compatibility with old code; use as_scalar() instead for Base objects like Mat)
template<typename out_eT>
class conv_to
  {
  public:

  template<typename in_eT, typename T1>
  inline static out_eT from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static out_eT from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static out_eT from(const BaseCube<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static out_eT from(const BaseCube<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
out_eT
conv_to<out_eT>::from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_type_check(( is_supported_elem_type<out_eT>::value == false ));

  const Proxy<T1> P(in.get_ref());

  arma_debug_check( (P.get_n_elem() != 1), "conv_to(): given object doesn't have exactly one element" );

  return out_eT(Proxy<T1>::use_at ? P.at(0,0) : P[0]);
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
out_eT
conv_to<out_eT>::from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_type_check(( is_supported_elem_type<out_eT>::value == false ));

  const Proxy<T1> P(in.get_ref());

  arma_debug_check( (P.get_n_elem() != 1), "conv_to(): given object doesn't have exactly one element" );

  out_eT out;

  arrayops::convert_cx_scalar(out, (Proxy<T1>::use_at ? P.at(0,0) : P[0]));

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
out_eT
conv_to<out_eT>::from(const BaseCube<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_type_check(( is_supported_elem_type<out_eT>::value == false ));

  const ProxyCube<T1> P(in.get_ref());

  arma_debug_check( (P.get_n_elem() != 1), "conv_to(): given object doesn't have exactly one element" );

  return out_eT(ProxyCube<T1>::use_at ? P.at(0,0,0) : P[0]);
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
out_eT
conv_to<out_eT>::from(const BaseCube<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_type_check(( is_supported_elem_type<out_eT>::value == false ));

  const ProxyCube<T1> P(in.get_ref());

  arma_debug_check( (P.get_n_elem() != 1), "conv_to(): given object doesn't have exactly one element" );

  out_eT out;

  arrayops::convert_cx_scalar(out, (ProxyCube<T1>::use_at ? P.at(0,0,0) : P[0]));

  return out;
  }



//! conversion to Armadillo matrices from Armadillo Base objects, as well as from std::vector
template<typename out_eT>
class conv_to< Mat<out_eT> >
  {
  public:

  template<typename in_eT, typename T1>
  inline static Mat<out_eT> from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static Mat<out_eT> from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);

  template<typename T1>
  inline static Mat<out_eT> from(const SpBase<out_eT, T1>& in);



  template<typename in_eT>
  inline static Mat<out_eT> from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT>
  inline static Mat<out_eT> from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Mat<out_eT>
conv_to< Mat<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  Mat<out_eT> out(X.n_rows, X.n_cols);

  arrayops::convert( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Mat<out_eT>
conv_to< Mat<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  Mat<out_eT> out(X.n_rows, X.n_cols);

  arrayops::convert_cx( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename T1>
arma_warn_unused
inline
Mat<out_eT>
conv_to< Mat<out_eT> >::from(const SpBase<out_eT, T1>& in)
  {
  arma_extra_debug_sigprint();

  return Mat<out_eT>(in.get_ref());
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Mat<out_eT>
conv_to< Mat<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Mat<out_eT> out(N, 1);

  if(N > 0)
    {
    arrayops::convert( out.memptr(), &(in[0]), N );
    }

  return out;
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Mat<out_eT>
conv_to< Mat<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Mat<out_eT> out(N, 1);

  if(N > 0)
    {
    arrayops::convert_cx( out.memptr(), &(in[0]), N );
    }

  return out;
  }



//! conversion to Armadillo row vectors from Armadillo Base objects, as well as from std::vector
template<typename out_eT>
class conv_to< Row<out_eT> >
  {
  public:

  template<typename in_eT, typename T1>
  inline static Row<out_eT> from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static Row<out_eT> from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);



  template<typename in_eT>
  inline static Row<out_eT> from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT>
  inline static Row<out_eT> from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Row<out_eT>
conv_to< Row<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  Row<out_eT> out(X.n_elem);

  arrayops::convert( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Row<out_eT>
conv_to< Row<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  Row<out_eT> out(X.n_rows, X.n_cols);

  arrayops::convert_cx( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Row<out_eT>
conv_to< Row<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Row<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert( out.memptr(), &(in[0]), N );
    }

  return out;
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Row<out_eT>
conv_to< Row<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Row<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert_cx( out.memptr(), &(in[0]), N );
    }

  return out;
  }



//! conversion to Armadillo column vectors from Armadillo Base objects, as well as from std::vector
template<typename out_eT>
class conv_to< Col<out_eT> >
  {
  public:

  template<typename in_eT, typename T1>
  inline static Col<out_eT> from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static Col<out_eT> from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);



  template<typename in_eT>
  inline static Col<out_eT> from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT>
  inline static Col<out_eT> from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Col<out_eT>
conv_to< Col<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  Col<out_eT> out(X.n_elem);

  arrayops::convert( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Col<out_eT>
conv_to< Col<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  Col<out_eT> out(X.n_rows, X.n_cols);

  arrayops::convert_cx( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Col<out_eT>
conv_to< Col<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Col<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert( out.memptr(), &(in[0]), N );
    }

  return out;
  }



template<typename out_eT>
template<typename in_eT>
arma_warn_unused
inline
Col<out_eT>
conv_to< Col<out_eT> >::from(const std::vector<in_eT>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword N = uword( in.size() );

  Col<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert_cx( out.memptr(), &(in[0]), N );
    }

  return out;
  }



template<typename out_eT>
class conv_to< SpMat<out_eT> >
  {
  public:

  template<typename T1>
  inline static SpMat<out_eT> from(const Base<out_eT, T1>& in);
  };



template<typename out_eT>
template<typename T1>
arma_warn_unused
inline
SpMat<out_eT>
conv_to< SpMat<out_eT> >::from(const Base<out_eT, T1>& in)
  {
  arma_extra_debug_sigprint();

  return SpMat<out_eT>(in.get_ref());
  }



//! conversion to Armadillo cubes from Armadillo BaseCube objects
template<typename out_eT>
class conv_to< Cube<out_eT> >
  {
  public:

  template<typename in_eT, typename T1>
  inline static Cube<out_eT> from(const BaseCube<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static Cube<out_eT> from(const BaseCube<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Cube<out_eT>
conv_to< Cube<out_eT> >::from(const BaseCube<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const unwrap_cube<T1>  tmp( in.get_ref() );
  const Cube<in_eT>& X = tmp.M;

  Cube<out_eT> out(X.n_rows, X.n_cols, X.n_slices);

  arrayops::convert( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
Cube<out_eT>
conv_to< Cube<out_eT> >::from(const BaseCube<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const unwrap_cube<T1>  tmp( in.get_ref() );
  const Cube<in_eT>& X = tmp.M;

  Cube<out_eT> out(X.n_rows, X.n_cols, X.n_slices);

  arrayops::convert_cx( out.memptr(), X.memptr(), X.n_elem );

  return out;
  }



//! conversion to std::vector from Armadillo Base objects
template<typename out_eT>
class conv_to< std::vector<out_eT> >
  {
  public:

  template<typename in_eT, typename T1>
  inline static std::vector<out_eT> from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk = 0);

  template<typename in_eT, typename T1>
  inline static std::vector<out_eT> from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk = 0);
  };



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
std::vector<out_eT>
conv_to< std::vector<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_not_cx<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  const uword N = X.n_elem;

  std::vector<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert( &(out[0]), X.memptr(), N );
    }

  return out;
  }



template<typename out_eT>
template<typename in_eT, typename T1>
arma_warn_unused
inline
std::vector<out_eT>
conv_to< std::vector<out_eT> >::from(const Base<in_eT, T1>& in, const typename arma_cx_only<in_eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const quasi_unwrap<T1> tmp(in.get_ref());
  const Mat<in_eT>& X  = tmp.M;

  arma_debug_check( ( (X.is_vec() == false) && (X.is_empty() == false) ), "conv_to(): given object can't be interpreted as a vector" );

  const uword N = X.n_elem;

  std::vector<out_eT> out(N);

  if(N > 0)
    {
    arrayops::convert_cx( &(out[0]), X.memptr(), N );
    }

  return out;
  }



//! @}
