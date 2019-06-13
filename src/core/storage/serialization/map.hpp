/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_MAP_HPP
#define TURI_SERIALIZE_MAP_HPP

#include <map>
#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>

namespace turi {

namespace archive_detail {
  /** Serializes a map */
  template <typename OutArcType, typename T, typename U>
  struct serialize_impl<OutArcType, std::map<T,U>, false > {
  static void exec(OutArcType& oarc, const std::map<T,U>& vec){
    serialize_iterator(oarc,
                       vec.begin(), vec.end(), vec.size());
  }
  };

  /** deserializes a map  */

  template <typename InArcType, typename T, typename U>
  struct deserialize_impl<InArcType, std::map<T,U>, false > {
  static void exec(InArcType& iarc, std::map<T,U>& vec){
    vec.clear();
    deserialize_iterator<InArcType,
                         std::pair<T,U> >(iarc,
                                          std::inserter(vec,vec.end()));
  }
  };

} // archive_detail
} // turicreate
#endif
