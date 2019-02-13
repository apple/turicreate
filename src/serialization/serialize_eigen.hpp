/**
 * Copyright (C) 2016 Turi
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 */

/**
 * Copyright (c) 2009 Carnegie Mellon University.
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.turicreate.ml.cmu.edu
 *
 */

#ifndef EIGEN_SERIALIZATION_HPP
#define EIGEN_SERIALIZATION_HPP

#include <serialization/serialization_includes.hpp>
#include <logger/assertions.hpp>
#include <typeinfo>

////////////////////////////////////////////////////////////////////////////////
// Some macros to tame the template gymnastics below

#define _EIGEN_TEMPLATE_ARGS(prefix)                                    \
  typename prefix##Scalar,                                              \
    int prefix##Rows, int prefix##Cols,                                 \
    int prefix##Options, int prefix##MaxRows, int prefix##MaxCols

#define _EIGEN_TEMPLATE_ARG_DEF(prefix)                 \
  prefix##Scalar,                                       \
    prefix##Rows, prefix##Cols,                         \
    prefix##Options, prefix##MaxRows, prefix##MaxCols


////////////////////////////////////////////////////////////////////////////////
// Forward declare the eigen classes so they don't have to be linked
// in with the serialization libraries.

namespace Eigen {

template<_EIGEN_TEMPLATE_ARGS(_)> class Matrix;

template<_EIGEN_TEMPLATE_ARGS(_)> class Array;

template <typename _Scalar, int _Flags, typename _Index> class SparseVector;
}

namespace turi { namespace archive_detail {

////////////////////////////////////////////////////////////////////////////////
// The main serializer function.  Used for both array and matrix.

template <typename InArcType,
          template <_EIGEN_TEMPLATE_ARGS(__)> class EigenContainer,
          _EIGEN_TEMPLATE_ARGS(_)>
void eigen_serialize_impl(InArcType& arc, const EigenContainer<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {

  // This code does type checking for making sure the correct types
  // are loaded.  However, it's not backwards compatible, so we'll
  // instead just do the simplest thing possible (that is backwards
  // compatible).  Thus this code is commented out for now.

  // size_t version = 1;
  // arc << version;

  // // An id of the scalar type.
  // std::string scalar_type_id = typeid(_Scalar).name();
  // arc << scalar_type_id;

  // arc << _Rows << _Cols << _Options << _MaxRows << _MaxCols;

  typedef typename EigenContainer<_EIGEN_TEMPLATE_ARG_DEF(_)>::Index index_type;

  const index_type rows = X.rows();
  const index_type cols = X.cols();

  arc << rows << cols;

  turi::serialize(arc, X.data(), rows*cols*sizeof(_Scalar));
}

////////////////////////////////////////////////////////////////////////////////
// The main deserializer function.  Used for both array and matrix.

template <typename InArcType,
          template <_EIGEN_TEMPLATE_ARGS(__)> class EigenContainer,
          _EIGEN_TEMPLATE_ARGS(_)>
void eigen_deserialize_impl(InArcType& arc, EigenContainer<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {


  // This code does type checking for making sure the correct types
  // are loaded.  However, it's not backwards compatible, so we'll
  // instead just do the simplest thing possible (that is backwards
  // compatible).  Thus this code is commented out for now.

  // size_t version;
  // arc >> version;
  // ASSERT_EQ(version, 1);

  // std::string scalar_type_id;

  // arc >> scalar_type_id;

  // ASSERT_MSG(scalar_type_id == typeid(_Scalar).name(),
  //            "Attempt to load Eigen matrix from conflicting type.");

  // int __Rows, __Cols, __Options, __MaxRows, __MaxCols;

  // arc >> __Rows >> __Cols >> __Options >> __MaxRows >> __MaxCols;

  // // Right now, only really need to check the options; the rest
  // // are going to be.
  // ASSERT_MSG(__Options == _Options,
  //            "Eigen interanl storage options not matched on load.");

  typedef typename EigenContainer<_EIGEN_TEMPLATE_ARG_DEF(_)>::Index index_type;

  index_type rows, cols;

  arc >> rows >> cols;

  X.resize(rows,cols);
  turi::deserialize(arc, X.data(), rows*cols*sizeof(_Scalar));

}

/////////////////////////////////////////////////////////////////////////////////
//
//  The matrix class

template <typename InArcType, _EIGEN_TEMPLATE_ARGS(_)>
struct deserialize_impl<InArcType, Eigen::Matrix<_EIGEN_TEMPLATE_ARG_DEF(_)>, false> {

  static void exec(InArcType& arc, Eigen::Matrix<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {
    eigen_deserialize_impl(arc, X);
  }
};

template <typename OutArcType, _EIGEN_TEMPLATE_ARGS(_)>
struct serialize_impl<OutArcType, Eigen::Matrix<_EIGEN_TEMPLATE_ARG_DEF(_)>, false> {

  static void exec(OutArcType& arc, const Eigen::Matrix<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {
    eigen_serialize_impl(arc, X);
  }
};

/////////////////////////////////////////////////////////////////////////////////
//
//  The array class

template <typename InArcType, _EIGEN_TEMPLATE_ARGS(_)>
struct deserialize_impl<InArcType, Eigen::Array<_EIGEN_TEMPLATE_ARG_DEF(_)>, false> {

  static void exec(InArcType& arc, Eigen::Array<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {
    eigen_deserialize_impl(arc, X);
  }
};

template <typename OutArcType, _EIGEN_TEMPLATE_ARGS(_)>
struct serialize_impl<OutArcType, Eigen::Array<_EIGEN_TEMPLATE_ARG_DEF(_)>, false> {

  static void exec(OutArcType& arc, const Eigen::Array<_EIGEN_TEMPLATE_ARG_DEF(_)>& X) {
    eigen_serialize_impl(arc, X);
  }
};

/////////////////////////////////////////////////////////////////////////////////
//
//  The SparseVector class

template <typename InArcType, typename _Scalar, int _Flags, typename _Index>
struct deserialize_impl<InArcType, Eigen::SparseVector<_Scalar, _Flags, _Index>, false> {

  static void exec(InArcType& arc, Eigen::SparseVector<_Scalar, _Flags, _Index>& vec) {
    size_t version;
    arc >> version;

    ASSERT_EQ(version, 1);

    size_t _size, _nnz, index;
    double value;

    arc >> _size;
    vec.resize(_size);

    arc >> _nnz;
    vec.reserve(_nnz);

    for(size_t i = 0; i < _nnz; i++) {
      arc >> index;
      arc >> value;
      vec.coeffRef(index) = value;
    }
  }
};

template <typename OutArcType, typename _Scalar, int _Flags, typename _Index>
struct serialize_impl<OutArcType, Eigen::SparseVector<_Scalar, _Flags, _Index>, false> {

  static void exec(OutArcType& arc, const Eigen::SparseVector<_Scalar, _Flags, _Index>& vec) {
    size_t version = 1;

    arc << version;

    arc << (size_t)vec.size() << (size_t)vec.nonZeros();

    for (typename Eigen::SparseVector<_Scalar, _Flags, _Index>::InnerIterator i(vec); i; ++i) {
      arc << (size_t)i.index() << (double)i.value();
    }
  }
};

}}

#undef _EIGEN_TEMPLATE_ARGS
#undef _EIGEN_TEMPLATE_ARG_DEF

#endif
