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


//! \addtogroup fn_join
//! @{



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_join_cols>
  >::result
join_cols(const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_join_cols>(A, B);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_join_cols>
  >::result
join_vert(const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_join_cols>(A, B);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_join_rows>
  >::result
join_rows(const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_join_rows>(A, B);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_join_rows>
  >::result
join_horiz(const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_join_rows>(A, B);
  }



//
// for cubes

template<typename T1, typename T2>
arma_warn_unused
inline
const GlueCube<T1, T2, glue_join_slices>
join_slices(const BaseCube<typename T1::elem_type,T1>& A, const BaseCube<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return GlueCube<T1, T2, glue_join_slices>(A.get_ref(), B.get_ref());
  }



template<typename T1, typename T2>
arma_warn_unused
inline
Cube<typename T1::elem_type>
join_slices(const Base<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1> UA(A.get_ref());
  const unwrap<T2> UB(B.get_ref());

  arma_debug_assert_same_size(UA.M.n_rows, UA.M.n_cols, UB.M.n_rows, UB.M.n_cols, "join_slices(): incompatible dimensions");

  Cube<eT> out(UA.M.n_rows, UA.M.n_cols, 2);

  arrayops::copy(out.slice_memptr(0), UA.M.memptr(), UA.M.n_elem);
  arrayops::copy(out.slice_memptr(1), UB.M.memptr(), UB.M.n_elem);

  return out;
  }



template<typename T1, typename T2>
arma_warn_unused
inline
Cube<typename T1::elem_type>
join_slices(const Base<typename T1::elem_type,T1>& A, const BaseCube<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1> U(A.get_ref());

  const Cube<eT> M(const_cast<eT*>(U.M.memptr()), U.M.n_rows, U.M.n_cols, 1, false);

  return join_slices(M,B);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
Cube<typename T1::elem_type>
join_slices(const BaseCube<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T2> U(B.get_ref());

  const Cube<eT> M(const_cast<eT*>(U.M.memptr()), U.M.n_rows, U.M.n_cols, 1, false);

  return join_slices(A,M);
  }



//
// for sparse matrices

template<typename T1, typename T2>
arma_warn_unused
inline
const SpGlue<T1, T2, spglue_join_cols>
join_cols(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1, T2, spglue_join_cols>(A.get_ref(), B.get_ref());
  }



template<typename T1, typename T2>
arma_warn_unused
inline
const SpGlue<T1, T2, spglue_join_cols>
join_vert(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1, T2, spglue_join_cols>(A.get_ref(), B.get_ref());
  }



template<typename T1, typename T2>
arma_warn_unused
inline
const SpGlue<T1, T2, spglue_join_rows>
join_rows(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1, T2, spglue_join_rows>(A.get_ref(), B.get_ref());
  }



template<typename T1, typename T2>
arma_warn_unused
inline
const SpGlue<T1, T2, spglue_join_rows>
join_horiz(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1, T2, spglue_join_rows>(A.get_ref(), B.get_ref());
  }



//! @}
