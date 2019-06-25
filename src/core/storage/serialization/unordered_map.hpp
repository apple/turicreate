/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_UNORDERED_MAP_HPP
#define TURI_SERIALIZE_UNORDERED_MAP_HPP

#include <boost/unordered_map.hpp>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>

namespace turi {

namespace archive_detail {
  /** Serializes a map */
  template <typename OutArcType, typename T, typename U>
  struct serialize_impl<OutArcType, boost::unordered_map<T,U>, false > {
  static void exec(OutArcType& oarc,
                   const boost::unordered_map<T,U>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a map  */

  template <typename InArcType, typename T, typename U>
  struct deserialize_impl<InArcType, boost::unordered_map<T,U>, false > {
  static void exec(InArcType& iarc, boost::unordered_map<T,U>& vec){
    vec.clear();
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      std::pair<T, U> v;
      iarc >> v;
      vec[v.first] = v.second;
    }
  }
  };

} // archive_detail
} // turicreate



#if defined(__cplusplus) && __cplusplus >= 201103L
#include <unordered_map>
namespace turi {

namespace archive_detail {
  /** Serializes a map */
  template <typename OutArcType, typename T, typename U>
  struct serialize_impl<OutArcType, std::unordered_map<T,U>, false > {
  static void exec(OutArcType& oarc,
                   const std::unordered_map<T,U>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a map  */

  template <typename InArcType, typename T, typename U>
  struct deserialize_impl<InArcType, std::unordered_map<T,U>, false > {
  static void exec(InArcType& iarc, std::unordered_map<T,U>& vec){
    vec.clear();
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      std::pair<T, U> v;
      iarc >> v;
      vec[v.first] = v.second;
    }
  }
  };

} // archive_detail
} // turicreate

#endif


#endif
