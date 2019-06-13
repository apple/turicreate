/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZATION_CONDITIONAL_SERIALIZE_HPP
#define TURI_SERIALIZATION_CONDITIONAL_SERIALIZE_HPP
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iarchive.hpp>
namespace turi {

template <typename T>
struct conditional_serialize {
  bool hasval;
  T val;

  conditional_serialize(): hasval(false) { }
  conditional_serialize(T& val): hasval(true), val(val) { }

  conditional_serialize(const conditional_serialize& cs): hasval(cs.hasval), val(cs.val) { }
  conditional_serialize& operator=(const conditional_serialize& cs) {
    hasval = cs.hasval;
    val = cs.val;
    return (*this);
  }
  void save(oarchive& oarc) const {
    oarc << hasval;
    if (hasval) oarc << val;
  }

  void load(iarchive& iarc) {
    iarc >> hasval;
    if (hasval) iarc >> val;
  }
};

};

#endif
