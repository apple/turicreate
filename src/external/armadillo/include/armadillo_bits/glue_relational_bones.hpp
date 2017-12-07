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


//! \addtogroup glue_relational
//! @{



class glue_rel_lt
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_lt>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_lt>& X);
  };



class glue_rel_gt
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_gt>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_gt>& X);
  };



class glue_rel_lteq
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_lteq>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_lteq>& X);
  };



class glue_rel_gteq
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_gteq>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_gteq>& X);
  };



class glue_rel_eq
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_eq>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_eq>& X);
  };



class glue_rel_noteq
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_noteq>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_noteq>& X);
  };



class glue_rel_and
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_and>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_and>& X);
  };



class glue_rel_or
  {
  public:

  template<typename T1, typename T2>
  inline static void apply(Mat <uword>& out, const mtGlue<uword, T1, T2, glue_rel_or>& X);

  template<typename T1, typename T2>
  inline static void apply(Cube <uword>& out, const mtGlueCube<uword, T1, T2, glue_rel_or>& X);
  };



//! @}
