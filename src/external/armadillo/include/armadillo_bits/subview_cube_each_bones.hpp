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


//! \addtogroup subview_cube_each
//! @{



template<typename eT>
class subview_cube_each_common
  {
  public:

  const Cube<eT>& P;

  inline void check_size(const Mat<eT>& A) const;


  protected:

  arma_inline subview_cube_each_common(const Cube<eT>& in_p);

  arma_cold inline const std::string incompat_size_string(const Mat<eT>& A) const;


  private:

  subview_cube_each_common();
  };




template<typename eT>
class subview_cube_each1 : public subview_cube_each_common<eT>
  {
  protected:

  arma_inline subview_cube_each1(const Cube<eT>& in_p);


  public:

  inline ~subview_cube_each1();

  // deliberately returning void
  template<typename T1> inline void operator=  (const Base<eT,T1>& x);
  template<typename T1> inline void operator+= (const Base<eT,T1>& x);
  template<typename T1> inline void operator-= (const Base<eT,T1>& x);
  template<typename T1> inline void operator%= (const Base<eT,T1>& x);
  template<typename T1> inline void operator/= (const Base<eT,T1>& x);
  template<typename T1> inline void operator*= (const Base<eT,T1>& x);


  private:

  friend class Cube<eT>;
  };



template<typename eT, typename TB>
class subview_cube_each2 : public subview_cube_each_common<eT>
  {
  protected:

  inline subview_cube_each2(const Cube<eT>& in_p, const Base<uword, TB>& in_indices);


  public:

  const Base<uword, TB>& base_indices;

  inline void check_indices(const Mat<uword>& indices) const;
  inline ~subview_cube_each2();

  // deliberately returning void
  template<typename T1> inline void operator=  (const Base<eT,T1>& x);
  template<typename T1> inline void operator+= (const Base<eT,T1>& x);
  template<typename T1> inline void operator-= (const Base<eT,T1>& x);
  template<typename T1> inline void operator%= (const Base<eT,T1>& x);
  template<typename T1> inline void operator/= (const Base<eT,T1>& x);


  private:

  friend class Cube<eT>;
  };



class subview_cube_each1_aux
  {
  public:

  template<typename eT, typename T2>
  static inline Cube<eT> operator_plus(const subview_cube_each1<eT>& X, const Base<eT,T2>& Y);

  template<typename eT, typename T2>
  static inline Cube<eT> operator_minus(const subview_cube_each1<eT>& X, const Base<eT,T2>& Y);

  template<typename T1, typename eT>
  static inline Cube<eT> operator_minus(const Base<eT,T1>& X, const subview_cube_each1<eT>& Y);

  template<typename eT, typename T2>
  static inline Cube<eT> operator_schur(const subview_cube_each1<eT>& X, const Base<eT,T2>& Y);

  template<typename eT, typename T2>
  static inline Cube<eT> operator_div(const subview_cube_each1<eT>& X,const Base<eT,T2>& Y);

  template<typename T1, typename eT>
  static inline Cube<eT> operator_div(const Base<eT,T1>& X, const subview_cube_each1<eT>& Y);

  template<typename eT, typename T2>
  static inline Cube<eT> operator_times(const subview_cube_each1<eT>& X,const Base<eT,T2>& Y);

  template<typename T1, typename eT>
  static inline Cube<eT> operator_times(const Base<eT,T1>& X, const subview_cube_each1<eT>& Y);
  };



class subview_cube_each2_aux
  {
  public:

  template<typename eT, typename TB, typename T2>
  static inline Cube<eT> operator_plus(const subview_cube_each2<eT,TB>& X, const Base<eT,T2>& Y);

  template<typename eT, typename TB, typename T2>
  static inline Cube<eT> operator_minus(const subview_cube_each2<eT,TB>& X, const Base<eT,T2>& Y);

  template<typename T1, typename eT, typename TB>
  static inline Cube<eT> operator_minus(const Base<eT,T1>& X, const subview_cube_each2<eT,TB>& Y);

  template<typename eT, typename TB, typename T2>
  static inline Cube<eT> operator_schur(const subview_cube_each2<eT,TB>& X, const Base<eT,T2>& Y);

  template<typename eT, typename TB, typename T2>
  static inline Cube<eT> operator_div(const subview_cube_each2<eT,TB>& X, const Base<eT,T2>& Y);

  template<typename T1, typename eT, typename TB>
  static inline Cube<eT> operator_div(const Base<eT,T1>& X, const subview_cube_each2<eT,TB>& Y);

  // TODO: operator_times
  };



//! @}
