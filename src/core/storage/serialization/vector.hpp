/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_VECTOR_HPP
#define TURI_SERIALIZE_VECTOR_HPP
#include <vector>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>


namespace turi {
  namespace archive_detail {
    /**
     * We re-dispatch vectors because based on the contained type,
     * it is actually possible to serialize them like a POD
     */
    template <typename OutArcType, typename ValueType, bool IsPOD>
    struct vector_serialize_impl {
      static void exec(OutArcType& oarc, const ValueType& vec) {
        // really this is an assert false. But the static assert
        // must depend on a template parameter
        BOOST_STATIC_ASSERT(sizeof(OutArcType) == 0);
        assert(false);
      };
    };
    /**
     * We re-dispatch vectors because based on the contained type,
     * it is actually possible to deserialize them like iarc POD
     */
    template <typename InArcType, typename ValueType, bool IsPOD>
    struct vector_deserialize_impl {
      static void exec(InArcType& iarc, ValueType& vec) {
        // really this is an assert false. But the static assert
        // must depend on a template parameter
        BOOST_STATIC_ASSERT(sizeof(InArcType) == 0);
        assert(false);
      };
    };

    /// If contained type is not a POD use the standard serializer
    template <typename OutArcType, typename ValueType>
    struct vector_serialize_impl<OutArcType, ValueType, false > {
      static void exec(OutArcType& oarc, const std::vector<ValueType>& vec) {
        oarc << size_t(vec.size());
        for (size_t i = 0;i < vec.size(); ++i) {
          oarc << vec[i];
        }
      }
    };

    /// Fast vector serialization if contained type is a POD
    template <typename OutArcType, typename ValueType>
    struct vector_serialize_impl<OutArcType, ValueType, true > {
      static void exec(OutArcType& oarc, const std::vector<ValueType>& vec) {
        oarc << size_t(vec.size());
        serialize(oarc, vec.data(),sizeof(ValueType)*vec.size());
      }
    };

    /// If contained type is not a POD use the standard deserializer
    template <typename InArcType, typename ValueType>
    struct vector_deserialize_impl<InArcType, ValueType, false > {
      static void exec(InArcType& iarc, std::vector<ValueType>& vec){
        size_t len;
        iarc >> len;
        vec.clear(); vec.resize(len);
        for (size_t i = 0;i < len; ++i) {
          iarc >> vec[i];
        }
      }
    };

    /// Fast vector deserialization if contained type is a POD
    template <typename InArcType, typename ValueType>
    struct vector_deserialize_impl<InArcType, ValueType, true > {
      static void exec(InArcType& iarc, std::vector<ValueType>& vec){
        size_t len;
        iarc >> len;
        vec.clear(); vec.resize(len);
        deserialize(iarc, vec.data(), sizeof(ValueType)*vec.size());
      }
    };



    /**
       Serializes a vector */
    template <typename OutArcType, typename ValueType>
    struct serialize_impl<OutArcType, std::vector<ValueType>, false > {
      static void exec(OutArcType& oarc, const std::vector<ValueType>& vec) {
        vector_serialize_impl<OutArcType, ValueType,
          gl_is_pod_or_scaler<ValueType>::value >::exec(oarc, vec);
      }
    };
    /**
       deserializes a vector */
    template <typename InArcType, typename ValueType>
    struct deserialize_impl<InArcType, std::vector<ValueType>, false > {
      static void exec(InArcType& iarc, std::vector<ValueType>& vec){
        vector_deserialize_impl<InArcType, ValueType,
          gl_is_pod_or_scaler<ValueType>::value >::exec(iarc, vec);
      }
    };
  } // archive_detail
} // namespace turi

#endif
