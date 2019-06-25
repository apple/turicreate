/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_UNORDERED_SET_HPP
#define TURI_SERIALIZE_UNORDERED_SET_HPP

#include <boost/unordered_set.hpp>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>

namespace turi {
namespace archive_detail {
  /** serializes a set  */
  template <typename OutArcType, typename T>
  struct serialize_impl<OutArcType, boost::unordered_set<T>, false > {
  static void exec(OutArcType& oarc, const boost::unordered_set<T>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a set  */
  template <typename InArcType, typename T>
  struct deserialize_impl<InArcType, boost::unordered_set<T>, false > {
  static void exec(InArcType& iarc, boost::unordered_set<T>& vec){
    vec.clear();
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      T v;
      iarc >> v;
      vec.insert(v);
    }
  }
  };

} // archive_detail
} // turicreate




// check if C++11 exists. If it does, also support the std::unordered_set
#if defined(__cplusplus) && __cplusplus >= 201103L
#include <unordered_set>

namespace turi {
namespace archive_detail {
  /** serializes a set  */
  template <typename OutArcType, typename T>
  struct serialize_impl<OutArcType, std::unordered_set<T>, false > {
  static void exec(OutArcType& oarc, const std::unordered_set<T>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a set  */
  template <typename InArcType, typename T>
  struct deserialize_impl<InArcType, std::unordered_set<T>, false > {
  static void exec(InArcType& iarc, std::unordered_set<T>& vec){
    vec.clear();
    // get the number of elements to deserialize
    size_t length = 0;
    iarc >> length;
    // iterate through and send to the output iterator
    for (size_t x = 0; x < length ; ++x){
      T v;
      iarc >> v;
      vec.insert(v);
    }
  }
  };

} // archive_detail
} // turicreate


#endif




#endif
