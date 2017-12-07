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


//! \addtogroup mtGlue
//! @{



template<typename out_eT, typename T1, typename T2, typename glue_type>
class mtGlue : public Base<out_eT, mtGlue<out_eT, T1, T2, glue_type> >
  {
  public:

  typedef          out_eT                       elem_type;
  typedef typename get_pod_type<out_eT>::result pod_type;

  static const bool is_row = \
    (
       ((T1::is_row || T2::is_row) && is_glue_mixed_elem<glue_type>::value)
    || (T1::is_row && is_glue_mixed_times<glue_type>::value)
    || (T1::is_row && is_same_type<glue_type, glue_hist_default>::yes)
    || (T1::is_row && is_same_type<glue_type, glue_histc_default>::yes)
    );

  static const bool is_col = \
    (
       ((T1::is_col || T2::is_col) && is_glue_mixed_elem<glue_type>::value)
    || (T2::is_col && is_glue_mixed_times<glue_type>::value)
    || (T1::is_col && is_same_type<glue_type, glue_hist_default>::yes)
    || (T1::is_col && is_same_type<glue_type, glue_histc_default>::yes)
    );

  arma_inline  mtGlue(const T1& in_A, const T2& in_B);
  arma_inline  mtGlue(const T1& in_A, const T2& in_B, const uword in_aux_uword);
  arma_inline ~mtGlue();

  arma_aligned const T1&   A;         //!< first operand
  arma_aligned const T2&   B;         //!< second operand
  arma_aligned       uword aux_uword; //!< storage of auxiliary data, uword format
  };



//! @}
