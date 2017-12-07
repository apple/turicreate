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


//! \addtogroup subview_each
//! @{



template<typename parent, unsigned int mode>
class subview_each_common
  {
  public:

  typedef typename parent::elem_type eT;

  const parent& P;

  inline void check_size(const Mat<typename parent::elem_type>& A) const;


  protected:

  arma_inline subview_each_common(const parent& in_P);

  arma_inline const Mat<typename parent::elem_type>& get_mat_ref_helper(const Mat    <typename parent::elem_type>& X) const;
  arma_inline const Mat<typename parent::elem_type>& get_mat_ref_helper(const subview<typename parent::elem_type>& X) const;

  arma_inline const Mat<typename parent::elem_type>& get_mat_ref() const;

  arma_cold inline const std::string incompat_size_string(const Mat<typename parent::elem_type>& A) const;


  private:

  subview_each_common();
  };




template<typename parent, unsigned int mode>
class subview_each1 : public subview_each_common<parent, mode>
  {
  protected:

  arma_inline subview_each1(const parent& in_P);


  public:

  typedef typename parent::elem_type eT;

  inline ~subview_each1();

  // deliberately returning void
  template<typename T1> inline void operator=  (const Base<eT,T1>& x);
  template<typename T1> inline void operator+= (const Base<eT,T1>& x);
  template<typename T1> inline void operator-= (const Base<eT,T1>& x);
  template<typename T1> inline void operator%= (const Base<eT,T1>& x);
  template<typename T1> inline void operator/= (const Base<eT,T1>& x);


  private:

  friend class Mat<eT>;
  friend class subview<eT>;
  };



template<typename parent, unsigned int mode, typename TB>
class subview_each2 : public subview_each_common<parent, mode>
  {
  protected:

  inline subview_each2(const parent& in_P, const Base<uword, TB>& in_indices);


  public:

  const Base<uword, TB>& base_indices;

  typedef typename parent::elem_type eT;

  inline void check_indices(const Mat<uword>& indices) const;
  inline ~subview_each2();

  // deliberately returning void
  template<typename T1> inline void operator=  (const Base<eT,T1>& x);
  template<typename T1> inline void operator+= (const Base<eT,T1>& x);
  template<typename T1> inline void operator-= (const Base<eT,T1>& x);
  template<typename T1> inline void operator%= (const Base<eT,T1>& x);
  template<typename T1> inline void operator/= (const Base<eT,T1>& x);


  private:

  friend class Mat<eT>;
  friend class subview<eT>;
  };



class subview_each1_aux
  {
  public:

  template<typename parent, unsigned int mode, typename T2>
  static inline Mat<typename parent::elem_type> operator_plus(const subview_each1<parent,mode>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename parent, unsigned int mode, typename T2>
  static inline Mat<typename parent::elem_type> operator_minus(const subview_each1<parent,mode>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename T1, typename parent, unsigned int mode>
  static inline Mat<typename parent::elem_type> operator_minus(const Base<typename parent::elem_type,T1>& X, const subview_each1<parent,mode>& Y);

  template<typename parent, unsigned int mode, typename T2>
  static inline Mat<typename parent::elem_type> operator_schur(const subview_each1<parent,mode>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename parent, unsigned int mode, typename T2>
  static inline Mat<typename parent::elem_type> operator_div(const subview_each1<parent,mode>& X,const Base<typename parent::elem_type,T2>& Y);

  template<typename T1, typename parent, unsigned int mode>
  static inline Mat<typename parent::elem_type> operator_div(const Base<typename parent::elem_type,T1>& X, const subview_each1<parent,mode>& Y);
  };



class subview_each2_aux
  {
  public:

  template<typename parent, unsigned int mode, typename TB, typename T2>
  static inline Mat<typename parent::elem_type> operator_plus(const subview_each2<parent,mode,TB>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename parent, unsigned int mode, typename TB, typename T2>
  static inline Mat<typename parent::elem_type> operator_minus(const subview_each2<parent,mode,TB>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename T1, typename parent, unsigned int mode, typename TB>
  static inline Mat<typename parent::elem_type> operator_minus(const Base<typename parent::elem_type,T1>& X, const subview_each2<parent,mode,TB>& Y);

  template<typename parent, unsigned int mode, typename TB, typename T2>
  static inline Mat<typename parent::elem_type> operator_schur(const subview_each2<parent,mode,TB>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename parent, unsigned int mode, typename TB, typename T2>
  static inline Mat<typename parent::elem_type> operator_div(const subview_each2<parent,mode,TB>& X, const Base<typename parent::elem_type,T2>& Y);

  template<typename T1, typename parent, unsigned int mode, typename TB>
  static inline Mat<typename parent::elem_type> operator_div(const Base<typename parent::elem_type,T1>& X, const subview_each2<parent,mode,TB>& Y);
  };



//! @}
