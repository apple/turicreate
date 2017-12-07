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


//! \addtogroup fn_randi
//! @{



template<typename obj_type>
arma_warn_unused
inline
obj_type
randi(const uword n_rows, const uword n_cols, const distr_param& param = distr_param(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename obj_type::elem_type eT;

  if(is_Col<obj_type>::value == true)
    {
    arma_debug_check( (n_cols != 1), "randi(): incompatible size" );
    }
  else
  if(is_Row<obj_type>::value == true)
    {
    arma_debug_check( (n_rows != 1), "randi(): incompatible size" );
    }

  obj_type out(n_rows, n_cols);

  int a;
  int b;

  if(param.state == 0)
    {
    a = 0;
    b = arma_rng::randi<eT>::max_val();
    }
  else
  if(param.state == 1)
    {
    a = param.a_int;
    b = param.b_int;
    }
  else
    {
    a = int(param.a_double);
    b = int(param.b_double);
    }

  arma_debug_check( (a > b), "randi(): incorrect distribution parameters: a must be less than b" );

  arma_rng::randi<eT>::fill(out.memptr(), out.n_elem, a, b);

  return out;
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
randi(const SizeMat& s, const distr_param& param = distr_param(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return randi<obj_type>(s.n_rows, s.n_cols, param);
  }



template<typename obj_type>
arma_warn_unused
inline
obj_type
randi(const uword n_elem, const distr_param& param = distr_param(), const arma_empty_class junk1 = arma_empty_class(), const typename arma_Mat_Col_Row_only<obj_type>::result* junk2 = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  if(is_Row<obj_type>::value == true)
    {
    return randi<obj_type>(1, n_elem, param);
    }
  else
    {
    return randi<obj_type>(n_elem, 1, param);
    }
  }



arma_warn_unused
inline
imat
randi(const uword n_rows, const uword n_cols, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randi<imat>(n_rows, n_cols, param);
  }



arma_warn_unused
inline
imat
randi(const SizeMat& s, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randi<imat>(s.n_rows, s.n_cols, param);
  }



arma_warn_unused
inline
ivec
randi(const uword n_elem, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randi<ivec>(n_elem, param);
  }



template<typename cube_type>
arma_warn_unused
inline
cube_type
randi(const uword n_rows, const uword n_cols, const uword n_slices, const distr_param& param = distr_param(), const typename arma_Cube_only<cube_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename cube_type::elem_type eT;

  cube_type out(n_rows, n_cols, n_slices);

  int a;
  int b;

  if(param.state == 0)
    {
    a = 0;
    b = arma_rng::randi<eT>::max_val();
    }
  else
  if(param.state == 1)
    {
    a = param.a_int;
    b = param.b_int;
    }
  else
    {
    a = int(param.a_double);
    b = int(param.b_double);
    }

  arma_debug_check( (a > b), "randi(): incorrect distribution parameters: a must be less than b" );

  arma_rng::randi<eT>::fill(out.memptr(), out.n_elem, a, b);

  return out;
  }



template<typename cube_type>
arma_warn_unused
inline
cube_type
randi(const SizeCube& s, const distr_param& param = distr_param(), const typename arma_Cube_only<cube_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return randi<cube_type>(s.n_rows, s.n_cols, s.n_slices, param);
  }



arma_warn_unused
inline
icube
randi(const uword n_rows, const uword n_cols, const uword n_slices, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randi<icube>(n_rows, n_cols, n_slices, param);
  }



arma_warn_unused
inline
icube
randi(const SizeCube& s, const distr_param& param = distr_param())
  {
  arma_extra_debug_sigprint();

  return randi<icube>(s.n_rows, s.n_cols, s.n_slices, param);
  }



//! @}
