/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_LIST_HPP
#define TURI_SERIALIZE_LIST_HPP

#include <list>

#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>


namespace turi {
namespace archive_detail {
  /** serializes a list  */
  template <typename OutArcType, typename T>
  struct serialize_impl<OutArcType, std::list<T>, false > {
  static void exec(OutArcType& oarc, const std::list<T>& vec){
    serialize_iterator(oarc,vec.begin(),vec.end(), vec.size());
  }
  };

  /** deserializes a list  */
  template <typename InArcType, typename T>
  struct deserialize_impl<InArcType, std::list<T>, false > {
  static void exec(InArcType& iarc, std::list<T>& vec){
    vec.clear();
    deserialize_iterator<InArcType, T>(iarc, std::inserter(vec,vec.end()));
  }
  };
} // archive_detail
} // turicreate
#endif
