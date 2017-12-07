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


//! \addtogroup fn_randg
//! @{



template<typename obj_type>
arma_warn_unused
inline
obj_type
randg(const uword n_rows, const uword n_cols, const distr_param& param = distr_param(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  #if defined(ARMA_USE_CXX11)
    {
    if(is_Col<obj_type>::value == true)
      {
      arma_debug_check( (n_cols != 1), "randg(): incompatible size" );
      }
    else
    if(is_Row<obj_type>::value == true)
      {
      arma_debug_check( (n_rows != 1), "randg(): incompatible size" );
      }

    obj_type out(n_rows, n_cols);

    double a;
    double b;

    if(param.state == 0)
      {
      a = double(1);
      b = double(1);
      }
    else
    if(param.state == 1)
      {
      a = double(param.a_int);
      b = double(param.b_int);
      }
    else
      {
      a = param.a_double;
      b = param.b_double;
      }

    arma_debug_check( ((a <= double(0)) || (b <= double(0))), "randg(): a and b must be greater than zero" );

    #if defined(ARMA_USE_EXTERN_CXX11_RNG)
      {
      arma_rng_cxx11_instance.randg_fill(out.memptr(), out.n_elem, a, b);
      }
    #else
      {
      arma_rng_cxx11 local_arma_rng_cxx11_instance;

      typedef typename arma_rng_cxx11::seed_type seed_type;

      local_arma_rng_cxx11_instance.set_seed( seed_type(arma_rng::randi<seed_type>()) );

      local_arma_rng_cxx11_instance.randg_fill(out.memptr(), out.n_elem, a, b);
      }
    #endif

    return out;
    }
  #else
    {
    arma_ignore(n_rows);
    arma_ignore(n_cols);
    arma_ignore(param);

    arma_stop_logic_error("randg(): C++11 compiler required");

    return obj_type();
    }
  #endif
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
randg(const SizeMat& s, const distr_param& param = distr_param(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return randg<obj_type>(s.n_rows, s.n_cols, param);
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
randg(const uword n_elem, const distr_param& param = distr_param(), const arma_empty_class junk1 = arma_empty_class(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk2 = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  if(is_Row<obj_type>::value == true)
    {
    return randg<obj_type>(1, n_elem, param);
    }
  else
    {
    return randg<obj_type>(n_elem, 1, param);
    }
  }



arma_warn_unused
inline
mat
randg(const uword n_rows, const uword n_cols, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randg<mat>(n_rows, n_cols, param);
  }



arma_warn_unused
inline
mat
randg(const SizeMat& s, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randg<mat>(s.n_rows, s.n_cols, param);
  }



arma_warn_unused
inline
vec
randg(const uword n_elem, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randg<vec>(n_elem, param);
  }



template<typename cube_type>
arma_warn_unused
inline
cube_type
randg(const uword n_rows, const uword n_cols, const uword n_slices, const distr_param& param = distr_param(), const typename arma_Cube_only<cube_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  #if defined(ARMA_USE_CXX11)
    {
    cube_type out(n_rows, n_cols, n_slices);

    double a;
    double b;

    if(param.state == 0)
      {
      a = double(1);
      b = double(1);
      }
    else
    if(param.state == 1)
      {
      a = double(param.a_int);
      b = double(param.b_int);
      }
    else
      {
      a = param.a_double;
      b = param.b_double;
      }

    arma_debug_check( ((a <= double(0)) || (b <= double(0))), "randg(): a and b must be greater than zero" );

    #if defined(ARMA_USE_EXTERN_CXX11_RNG)
      {
      arma_rng_cxx11_instance.randg_fill(out.memptr(), out.n_elem, a, b);
      }
    #else
      {
      arma_rng_cxx11 local_arma_rng_cxx11_instance;

      typedef typename arma_rng_cxx11::seed_type seed_type;

      local_arma_rng_cxx11_instance.set_seed( seed_type(arma_rng::randi<seed_type>()) );

      local_arma_rng_cxx11_instance.randg_fill(out.memptr(), out.n_elem, a, b);
      }
    #endif

    return out;
    }
  #else
    {
    arma_ignore(n_rows);
    arma_ignore(n_cols);
    arma_ignore(n_slices);
    arma_ignore(param);

    arma_stop_logic_error("randg(): C++11 compiler required");

    return cube_type();
    }
  #endif
  }



template<typename cube_type>
arma_warn_unused
inline
cube_type
randg(const SizeCube& s, const distr_param& param = distr_param(), const typename arma_Cube_only<cube_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return randg<cube_type>(s.n_rows, s.n_cols, s.n_slices, param);
  }



arma_warn_unused
inline
cube
randg(const uword n_rows, const uword n_cols, const uword n_slices, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randg<cube>(n_rows, n_cols, n_slices, param);
  }



arma_warn_unused
inline
cube
randg(const SizeCube& s, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randg<cube>(s.n_rows, s.n_cols, s.n_slices, param);
  }



//! @}
