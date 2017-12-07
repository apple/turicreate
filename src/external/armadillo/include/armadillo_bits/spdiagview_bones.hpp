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


//! \addtogroup spdiagview
//! @{


//! Class for storing data required to extract and set the diagonals of a sparse matrix
template<typename eT>
class spdiagview : public SpBase<eT, spdiagview<eT> >
  {
  public:

  typedef eT                                elem_type;
  typedef typename get_pod_type<eT>::result pod_type;

  arma_aligned const SpMat<eT>& m;

  static const bool is_row = false;
  static const bool is_col = true;

  const uword row_offset;
  const uword col_offset;

  const uword n_rows;     // equal to n_elem
  const uword n_elem;

  static const uword n_cols = 1;


  protected:

  arma_inline spdiagview(const SpMat<eT>& in_m, const uword in_row_offset, const uword in_col_offset, const uword len);


  public:

  inline ~spdiagview();

  inline void operator=(const spdiagview& x);

  inline void operator+=(const eT val);
  inline void operator-=(const eT val);
  inline void operator*=(const eT val);
  inline void operator/=(const eT val);

  template<typename T1> inline void operator= (const Base<eT,T1>& x);
  template<typename T1> inline void operator+=(const Base<eT,T1>& x);
  template<typename T1> inline void operator-=(const Base<eT,T1>& x);
  template<typename T1> inline void operator%=(const Base<eT,T1>& x);
  template<typename T1> inline void operator/=(const Base<eT,T1>& x);

  template<typename T1> inline void operator= (const SpBase<eT,T1>& x);
  template<typename T1> inline void operator+=(const SpBase<eT,T1>& x);
  template<typename T1> inline void operator-=(const SpBase<eT,T1>& x);
  template<typename T1> inline void operator%=(const SpBase<eT,T1>& x);
  template<typename T1> inline void operator/=(const SpBase<eT,T1>& x);

  inline MapMat_elem<eT> operator[](const uword ii);
  inline eT              operator[](const uword ii) const;

  inline MapMat_elem<eT> at(const uword ii);
  inline eT              at(const uword ii) const;

  inline MapMat_elem<eT> operator()(const uword ii);
  inline eT              operator()(const uword ii) const;

  inline MapMat_elem<eT> at(const uword in_n_row, const uword);
  inline eT              at(const uword in_n_row, const uword) const;

  inline MapMat_elem<eT> operator()(const uword in_n_row, const uword in_n_col);
  inline eT              operator()(const uword in_n_row, const uword in_n_col) const;


  inline void fill(const eT val);
  inline void zeros();
  inline void ones();
  inline void randu();
  inline void randn();


  inline static void extract(SpMat<eT>& out, const spdiagview& in);
  inline static void extract(  Mat<eT>& out, const spdiagview& in);


  private:

  friend class SpMat<eT>;
  spdiagview();
  };


//! @}
