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


//! \addtogroup eop_core
//! @{



template<typename eop_type>
class eop_core
  {
  public:

  // matrices

  template<typename outT, typename T1> arma_hot inline static void apply(outT& out, const eOp<T1, eop_type>& x);

  template<typename T1> arma_hot inline static void apply_inplace_plus (Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_minus(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_schur(Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_div  (Mat<typename T1::elem_type>& out, const eOp<T1, eop_type>& x);


  // cubes

  template<typename T1> arma_hot inline static void apply(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x);

  template<typename T1> arma_hot inline static void apply_inplace_plus (Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_minus(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_schur(Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x);
  template<typename T1> arma_hot inline static void apply_inplace_div  (Cube<typename T1::elem_type>& out, const eOpCube<T1, eop_type>& x);


  // common

  template<typename eT> arma_hot arma_inline static eT process(const eT val, const eT k);
  };


struct eop_use_mp_true  { static const bool use_mp = true;  };
struct eop_use_mp_false { static const bool use_mp = false; };


class eop_neg               : public eop_core<eop_neg>               , public eop_use_mp_false {};
class eop_scalar_plus       : public eop_core<eop_scalar_plus>       , public eop_use_mp_false {};
class eop_scalar_minus_pre  : public eop_core<eop_scalar_minus_pre>  , public eop_use_mp_false {};
class eop_scalar_minus_post : public eop_core<eop_scalar_minus_post> , public eop_use_mp_false {};
class eop_scalar_times      : public eop_core<eop_scalar_times>      , public eop_use_mp_false {};
class eop_scalar_div_pre    : public eop_core<eop_scalar_div_pre>    , public eop_use_mp_false {};
class eop_scalar_div_post   : public eop_core<eop_scalar_div_post>   , public eop_use_mp_false {};
class eop_square            : public eop_core<eop_square>            , public eop_use_mp_false {};
class eop_sqrt              : public eop_core<eop_sqrt>              , public eop_use_mp_true  {};
class eop_pow               : public eop_core<eop_pow>               , public eop_use_mp_false {};  // for pow(), use_mp is selectively enabled in eop_core_meat.hpp
class eop_log               : public eop_core<eop_log>               , public eop_use_mp_true  {};
class eop_log2              : public eop_core<eop_log2>              , public eop_use_mp_true  {};
class eop_log10             : public eop_core<eop_log10>             , public eop_use_mp_true  {};
class eop_trunc_log         : public eop_core<eop_trunc_log>         , public eop_use_mp_true  {};
class eop_exp               : public eop_core<eop_exp>               , public eop_use_mp_true  {};
class eop_exp2              : public eop_core<eop_exp2>              , public eop_use_mp_true  {};
class eop_exp10             : public eop_core<eop_exp10>             , public eop_use_mp_true  {};
class eop_trunc_exp         : public eop_core<eop_trunc_exp>         , public eop_use_mp_true  {};
class eop_cos               : public eop_core<eop_cos>               , public eop_use_mp_true  {};
class eop_sin               : public eop_core<eop_sin>               , public eop_use_mp_true  {};
class eop_tan               : public eop_core<eop_tan>               , public eop_use_mp_true  {};
class eop_acos              : public eop_core<eop_acos>              , public eop_use_mp_true  {};
class eop_asin              : public eop_core<eop_asin>              , public eop_use_mp_true  {};
class eop_atan              : public eop_core<eop_atan>              , public eop_use_mp_true  {};
class eop_cosh              : public eop_core<eop_cosh>              , public eop_use_mp_true  {};
class eop_sinh              : public eop_core<eop_sinh>              , public eop_use_mp_true  {};
class eop_tanh              : public eop_core<eop_tanh>              , public eop_use_mp_true  {};
class eop_acosh             : public eop_core<eop_acosh>             , public eop_use_mp_true  {};
class eop_asinh             : public eop_core<eop_asinh>             , public eop_use_mp_true  {};
class eop_atanh             : public eop_core<eop_atanh>             , public eop_use_mp_true  {};
class eop_eps               : public eop_core<eop_eps>               , public eop_use_mp_true  {};
class eop_abs               : public eop_core<eop_abs>               , public eop_use_mp_false {};
class eop_arg               : public eop_core<eop_arg>               , public eop_use_mp_false {};
class eop_conj              : public eop_core<eop_conj>              , public eop_use_mp_false {};
class eop_floor             : public eop_core<eop_floor>             , public eop_use_mp_false {};
class eop_ceil              : public eop_core<eop_ceil>              , public eop_use_mp_false {};
class eop_round             : public eop_core<eop_round>             , public eop_use_mp_false {};
class eop_trunc             : public eop_core<eop_trunc>             , public eop_use_mp_false {};
class eop_sign              : public eop_core<eop_sign>              , public eop_use_mp_false {};
class eop_erf               : public eop_core<eop_erf>               , public eop_use_mp_true  {};
class eop_erfc              : public eop_core<eop_erfc>              , public eop_use_mp_true  {};
class eop_lgamma            : public eop_core<eop_lgamma>            , public eop_use_mp_true  {};



// the classes below are currently not used; reserved for potential future use
class eop_log_approx {};
class eop_exp_approx {};
class eop_approx_log {};
class eop_approx_exp {};



//! @}
