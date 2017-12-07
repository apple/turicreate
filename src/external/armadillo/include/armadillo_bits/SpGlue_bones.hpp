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


//! \addtogroup SpGlue
//! @{



template<typename T1, typename T2, typename spglue_type>
class SpGlue : public SpBase<typename T1::elem_type, SpGlue<T1, T2, spglue_type> >
  {
  public:

  typedef typename T1::elem_type                   elem_type;
  typedef typename get_pod_type<elem_type>::result pod_type;

  static const bool is_row = ( (T1::is_row || T2::is_row) && is_spglue_elem<spglue_type>::value ) || ( (is_spglue_times<spglue_type>::value || is_spglue_times2<spglue_type>::value) ? T1::is_row : false );
  static const bool is_col = ( (T1::is_col || T2::is_col) && is_spglue_elem<spglue_type>::value ) || ( (is_spglue_times<spglue_type>::value || is_spglue_times2<spglue_type>::value) ? T2::is_col : false );

  arma_inline  SpGlue(const T1& in_A, const T2& in_B);
  arma_inline  SpGlue(const T1& in_A, const T2& in_B, const elem_type in_aux);
  arma_inline ~SpGlue();

  const T1&       A;    //!< first operand
  const T2&       B;    //!< second operand
        elem_type aux;
  };



//! @}
