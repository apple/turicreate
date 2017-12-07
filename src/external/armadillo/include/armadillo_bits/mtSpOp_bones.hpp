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


//! \addtogroup mtSpOp
//! @{

// Class for delayed multi-type sparse operations.  These are operations where
// the resulting type is different than the stored type.



template<typename out_eT, typename T1, typename op_type>
class mtSpOp : public SpBase<out_eT, mtSpOp<out_eT, T1, op_type> >
  {
  public:

  typedef          out_eT                       elem_type;
  typedef typename get_pod_type<out_eT>::result pod_type;

  typedef typename T1::elem_type                in_eT;

  static const bool is_row = false;
  static const bool is_col = false;

  inline explicit mtSpOp(const T1& in_m);
  inline          mtSpOp(const T1& in_m, const uword aux_uword_a, const uword aux_uword_b);

  inline          ~mtSpOp();

  arma_aligned const T1&    m;
  arma_aligned       uword  aux_uword_a;
  arma_aligned       uword  aux_uword_b;
  };



//! @}
