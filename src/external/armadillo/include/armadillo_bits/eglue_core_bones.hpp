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


//! \addtogroup eglue_core
//! @{



template<typename eglue_type>
struct eglue_core
  {

  // matrices

  template<typename outT, typename T1, typename T2> arma_hot inline static void apply(outT& out, const eGlue<T1, T2, eglue_type>& x);

  template<typename T1, typename T2> arma_hot inline static void apply_inplace_plus (Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_minus(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_schur(Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_div  (Mat<typename T1::elem_type>& out, const eGlue<T1, T2, eglue_type>& x);


  // cubes

  template<typename T1, typename T2> arma_hot inline static void apply(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x);

  template<typename T1, typename T2> arma_hot inline static void apply_inplace_plus (Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_minus(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_schur(Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x);
  template<typename T1, typename T2> arma_hot inline static void apply_inplace_div  (Cube<typename T1::elem_type>& out, const eGlueCube<T1, T2, eglue_type>& x);
  };



class eglue_plus : public eglue_core<eglue_plus>
  {
  public:

  inline static const char* text() { return "addition"; }
  };



class eglue_minus : public eglue_core<eglue_minus>
  {
  public:

  inline static const char* text() { return "subtraction"; }
  };



class eglue_div : public eglue_core<eglue_div>
  {
  public:

  inline static const char* text() { return "element-wise division"; }
  };



class eglue_schur : public eglue_core<eglue_schur>
  {
  public:

  inline static const char* text() { return "element-wise multiplication"; }
  };



//! @}
