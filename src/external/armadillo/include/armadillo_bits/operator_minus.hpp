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


//! \addtogroup operator_minus
//! @{



//! unary -
template<typename T1>
arma_inline
typename
enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_neg> >::result
operator-
(const T1& X)
  {
  arma_extra_debug_sigprint();

  return eOp<T1,eop_neg>(X);
  }



//! Base - scalar
template<typename T1>
arma_inline
typename
enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_scalar_minus_post> >::result
operator-
  (
  const T1&                    X,
  const typename T1::elem_type k
  )
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_scalar_minus_post>(X, k);
  }



//! scalar - Base
template<typename T1>
arma_inline
typename
enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_scalar_minus_pre> >::result
operator-
  (
  const typename T1::elem_type k,
  const T1&                    X
  )
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_scalar_minus_pre>(X, k);
  }



//! complex scalar - non-complex Base
template<typename T1>
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_cx<typename T1::elem_type>::no),
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>
  >::result
operator-
  (
  const std::complex<typename T1::pod_type>& k,
  const T1&                                  X
  )
  {
  arma_extra_debug_sigprint();

  return mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_pre>('j', X, k);
  }



//! non-complex Base - complex scalar
template<typename T1>
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_cx<typename T1::elem_type>::no),
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>
  >::result
operator-
  (
  const T1&                                  X,
  const std::complex<typename T1::pod_type>& k
  )
  {
  arma_extra_debug_sigprint();

  return mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_minus_post>('j', X, k);
  }



//! subtraction of Base objects with same element type
template<typename T1, typename T2>
arma_inline
typename
enable_if2
  <
  is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value,
  const eGlue<T1, T2, eglue_minus>
  >::result
operator-
  (
  const T1& X,
  const T2& Y
  )
  {
  arma_extra_debug_sigprint();

  return eGlue<T1, T2, eglue_minus>(X, Y);
  }



//! subtraction of Base objects with different element types
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_same_type<typename T1::elem_type, typename T2::elem_type>::no)),
  const mtGlue<typename promote_type<typename T1::elem_type, typename T2::elem_type>::result, T1, T2, glue_mixed_minus>
  >::result
operator-
  (
  const T1& X,
  const T2& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT1;
  typedef typename T2::elem_type eT2;

  typedef typename promote_type<eT1,eT2>::result out_eT;

  promote_type<eT1,eT2>::check();

  return mtGlue<out_eT, T1, T2, glue_mixed_minus>( X, Y );
  }



//! unary "-" for sparse objects
template<typename T1>
inline
typename
enable_if2
  <
  is_arma_sparse_type<T1>::value && is_signed<typename T1::elem_type>::value,
  SpOp<T1,spop_scalar_times>
  >::result
operator-
(const T1& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  return SpOp<T1,spop_scalar_times>(X, eT(-1));
  }



//! subtraction of two sparse objects
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value && is_arma_sparse_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const SpGlue<T1,T2,spglue_minus>
  >::result
operator-
  (
  const T1& X,
  const T2& Y
  )
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1,T2,spglue_minus>(X,Y);
  }



//! subtraction of one sparse and one dense object
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  Mat<typename T1::elem_type>
  >::result
operator-
  (
  const T1& x,
  const T2& y
  )
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> pa(x);

  Mat<typename T1::elem_type> result(-y);

  arma_debug_assert_same_size( pa.get_n_rows(), pa.get_n_cols(), result.n_rows, result.n_cols, "subtraction" );

  typename SpProxy<T1>::const_iterator_type it     = pa.begin();
  typename SpProxy<T1>::const_iterator_type it_end = pa.end();

  while(it != it_end)
    {
    result.at(it.row(), it.col()) += (*it);
    ++it;
    }

  return result;
  }



//! subtraction of one dense and one sparse object
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_sparse_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  Mat<typename T1::elem_type>
  >::result
operator-
  (
  const T1& x,
  const T2& y
  )
  {
  arma_extra_debug_sigprint();

  Mat<typename T1::elem_type> result(x);

  const SpProxy<T2> pb(y.get_ref());

  arma_debug_assert_same_size( result.n_rows, result.n_cols, pb.get_n_rows(), pb.get_n_cols(), "subtraction" );

  typename SpProxy<T2>::const_iterator_type it     = pb.begin();
  typename SpProxy<T2>::const_iterator_type it_end = pb.end();

  while(it != it_end)
    {
    result.at(it.row(), it.col()) -= (*it);
    ++it;
    }

  return result;
  }



template<typename parent, unsigned int mode, typename T2>
arma_inline
Mat<typename parent::elem_type>
operator-
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each1_aux::operator_minus(X, Y.get_ref());
  }



template<typename T1, typename parent, unsigned int mode>
arma_inline
Mat<typename parent::elem_type>
operator-
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each1<parent,mode>&          Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each1_aux::operator_minus(X.get_ref(), Y);
  }



template<typename parent, unsigned int mode, typename TB, typename T2>
arma_inline
Mat<typename parent::elem_type>
operator-
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each2_aux::operator_minus(X, Y.get_ref());
  }



template<typename T1, typename parent, unsigned int mode, typename TB>
arma_inline
Mat<typename parent::elem_type>
operator-
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each2<parent,mode,TB>&       Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each2_aux::operator_minus(X.get_ref(), Y);
  }



//! @}
