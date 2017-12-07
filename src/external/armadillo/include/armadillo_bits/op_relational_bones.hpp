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


//! \addtogroup op_relational
//! @{



class op_rel_lt_pre
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lt_pre>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lt_pre>& X);
  };



class op_rel_lt_post
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lt_post>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lt_post>& X);
  };



class op_rel_gt_pre
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gt_pre>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gt_pre>& X);
  };



class op_rel_gt_post
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gt_post>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gt_post>& X);
  };



class op_rel_lteq_pre
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lteq_pre>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lteq_pre>& X);
  };



class op_rel_lteq_post
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_lteq_post>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_lteq_post>& X);
  };



class op_rel_gteq_pre
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gteq_pre>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gteq_pre>& X);
  };



class op_rel_gteq_post
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_gteq_post>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_gteq_post>& X);
  };



class op_rel_eq
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_eq>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_eq>& X);
  };



class op_rel_noteq
  {
  public:

  template<typename T1>
  inline static void apply(Mat<uword>& out, const mtOp<uword, T1, op_rel_noteq>& X);

  template<typename T1>
  inline static void apply(Cube<uword>& out, const mtOpCube<uword, T1, op_rel_noteq>& X);
  };



//! @}
