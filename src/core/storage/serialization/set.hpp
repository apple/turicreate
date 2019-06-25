/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_SET_HPP
#define TURI_SERIALIZE_SET_HPP

#include <set>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>

namespace turi {
namespace archive_detail {
  /** serializes a set  */
  template <typename OutArcType, typename T>
  struct serialize_impl<OutArcType, std::set<T>, false > {
  static void exec(OutArcType& oarc, const std::set<T>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a set  */
  template <typename InArcType, typename T>
  struct deserialize_impl<InArcType, std::set<T>, false > {
  static void exec(InArcType& iarc, std::set<T>& vec){
    vec.clear();
    deserialize_iterator<InArcType, T>(iarc,
                                       std::inserter(vec,vec.end()));
  }
  };

} // archive_detail
} // turicreate

#endif
