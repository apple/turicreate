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


//! \addtogroup operator_plus
//! @{



//! unary plus operation (does nothing, but is required for completeness)
template<typename T1>
arma_inline
typename enable_if2< is_arma_type<T1>::value, const T1& >::result
operator+
(const T1& X)
  {
  arma_extra_debug_sigprint();

  return X;
  }



//! Base + scalar
template<typename T1>
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_scalar_plus> >::result
operator+
(const T1& X, const typename T1::elem_type k)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_scalar_plus>(X, k);
  }



//! scalar + Base
template<typename T1>
arma_inline
typename enable_if2< is_arma_type<T1>::value, const eOp<T1, eop_scalar_plus> >::result
operator+
(const typename T1::elem_type k, const T1& X)
  {
  arma_extra_debug_sigprint();

  return eOp<T1, eop_scalar_plus>(X, k);  // NOTE: order is swapped
  }



//! non-complex Base + complex scalar
template<typename T1>
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_cx<typename T1::elem_type>::no),
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>
  >::result
operator+
  (
  const T1&                                  X,
  const std::complex<typename T1::pod_type>& k
  )
  {
  arma_extra_debug_sigprint();

  return mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>('j', X, k);
  }



//! complex scalar + non-complex Base
template<typename T1>
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_cx<typename T1::elem_type>::no),
  const mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>
  >::result
operator+
  (
  const std::complex<typename T1::pod_type>& k,
  const T1&                                  X
  )
  {
  arma_extra_debug_sigprint();

  return mtOp<typename std::complex<typename T1::pod_type>, T1, op_cx_scalar_plus>('j', X, k);  // NOTE: order is swapped
  }



//! addition of user-accessible Armadillo objects with same element type
template<typename T1, typename T2>
arma_inline
typename
enable_if2
  <
  is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value,
  const eGlue<T1, T2, eglue_plus>
  >::result
operator+
  (
  const T1& X,
  const T2& Y
  )
  {
  arma_extra_debug_sigprint();

  return eGlue<T1, T2, eglue_plus>(X, Y);
  }



//! addition of user-accessible Armadillo objects with different element types
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && (is_same_type<typename T1::elem_type, typename T2::elem_type>::no)),
  const mtGlue<typename promote_type<typename T1::elem_type, typename T2::elem_type>::result, T1, T2, glue_mixed_plus>
  >::result
operator+
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

  return mtGlue<out_eT, T1, T2, glue_mixed_plus>( X, Y );
  }



//! addition of two sparse objects
template<typename T1, typename T2>
inline
arma_hot
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value && is_arma_sparse_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  SpGlue<T1,T2,spglue_plus>
  >::result
operator+
  (
  const T1& x,
  const T2& y
  )
  {
  arma_extra_debug_sigprint();

  return SpGlue<T1,T2,spglue_plus>(x, y);
  }



//! addition of sparse and non-sparse object
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_sparse_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  Mat<typename T1::elem_type>
  >::result
operator+
  (
  const T1& x,
  const T2& y
  )
  {
  arma_extra_debug_sigprint();

  Mat<typename T1::elem_type> result(x);

  const SpProxy<T2> pb(y);

  arma_debug_assert_same_size( result.n_rows, result.n_cols, pb.get_n_rows(), pb.get_n_cols(), "addition" );

  typename SpProxy<T2>::const_iterator_type it     = pb.begin();
  typename SpProxy<T2>::const_iterator_type it_end = pb.end();

  while(it != it_end)
    {
    result.at(it.row(), it.col()) += (*it);
    ++it;
    }

  return result;
  }



//! addition of sparse and non-sparse object
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  Mat<typename T1::elem_type>
  >::result
operator+
  (
  const T1& x,
  const T2& y
  )
  {
  arma_extra_debug_sigprint();

  // Just call the other order (these operations are commutative)
  return (y + x);
  }



template<typename parent, unsigned int mode, typename T2>
arma_inline
Mat<typename parent::elem_type>
operator+
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each1_aux::operator_plus(X, Y.get_ref());
  }



template<typename T1, typename parent, unsigned int mode>
arma_inline
Mat<typename parent::elem_type>
operator+
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each1<parent,mode>&          Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each1_aux::operator_plus(Y, X.get_ref());  // NOTE: swapped order
  }



template<typename parent, unsigned int mode, typename TB, typename T2>
arma_inline
Mat<typename parent::elem_type>
operator+
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each2_aux::operator_plus(X, Y.get_ref());
  }



template<typename T1, typename parent, unsigned int mode, typename TB>
arma_inline
Mat<typename parent::elem_type>
operator+
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each2<parent,mode,TB>&       Y
  )
  {
  arma_extra_debug_sigprint();

  return subview_each2_aux::operator_plus(Y, X.get_ref());  // NOTE: swapped order
  }



//! @}
