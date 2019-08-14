/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZABLE
#define TURI_SERIALIZABLE
#include <boost/concept/assert.hpp>
#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>
#include <sstream>
#include <core/storage/serialization/serialize.hpp>
namespace turi {

  /**
   * \brief Concept checks if a type T is serializable.
   *
   * This is a concept checking class for boost::concept and can be
   * used to enforce that a type T is \ref sec_serializable, assignable and
   * default constructible.
   *
   * \tparam T The type to test for serializability.
   */
  template <typename T>
  class Serializable : boost::Assignable<T>, boost::DefaultConstructible<T> {
   public:
    BOOST_CONCEPT_USAGE(Serializable) {
      std::stringstream strm;
      oarchive oarc(strm);
      iarchive iarc(strm);
      const T const_t = T();
      T t = T();
      // A compiler error on these lines implies that your type is not
      // serializable.  See the documentaiton on how to make
      // serializable type.
      oarc << const_t;
      iarc >> t;
    }
  };

} // namespace turi
#endif
