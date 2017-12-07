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


//! \addtogroup mtOpCube
//! @{



struct mtOpCube_dual_aux_indicator {};


template<typename out_eT, typename T1, typename op_type>
class mtOpCube : public BaseCube<out_eT, mtOpCube<out_eT, T1, op_type> >
  {
  public:

  typedef          out_eT                       elem_type;
  typedef typename get_pod_type<out_eT>::result pod_type;

  typedef typename T1::elem_type                in_eT;

  inline explicit mtOpCube(const T1& in_m);
  inline          mtOpCube(const T1& in_m, const in_eT in_aux);
  inline          mtOpCube(const T1& in_m, const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c);
  inline          mtOpCube(const T1& in_m, const in_eT in_aux,         const uword in_aux_uword_a, const uword in_aux_uword_b, const uword in_aux_uword_c);

  inline          mtOpCube(const char junk, const T1& in_m, const out_eT in_aux);

  inline          mtOpCube(const mtOpCube_dual_aux_indicator&, const T1& in_m, const in_eT in_aux_a, const out_eT in_aux_b);

  inline         ~mtOpCube();


  arma_aligned const T1&    m;            //!< storage of reference to the operand (eg. a matrix)
  arma_aligned       in_eT  aux;          //!< storage of auxiliary data, using the element type as used by T1
  arma_aligned       out_eT aux_out_eT;   //!< storage of auxiliary data, using the element type as specified by the out_eT template parameter
  arma_aligned       uword  aux_uword_a;  //!< storage of auxiliary data, uword format
  arma_aligned       uword  aux_uword_b;  //!< storage of auxiliary data, uword format
  arma_aligned       uword  aux_uword_c;  //!< storage of auxiliary data, uword format

  };



//! @}
