/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCHES_SPACE_SAVING_SKETCH_FLEXTYPE_HPP
#define TURI_SKETCHES_SPACE_SAVING_SKETCH_FLEXTYPE_HPP

#include <vector>
#include <ml/sketches/space_saving.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {
namespace sketches {

////////////////////////////////////////////////////////////////////////////////

/**
 * \ingroup sketching
 * Provides an efficient wrapper around the space_saving sketch customized for
 * use with flexible type. See \ref space_saving
 */
class space_saving_flextype {
 private:
  std::unique_ptr<space_saving<flex_int> > ss_integer;
  std::unique_ptr<space_saving<flexible_type> > ss_general;

  bool is_combined = false;
  double m_epsilon = 0;

 public:

  inline space_saving_flextype(const space_saving_flextype& ss)
      : ss_integer(new space_saving<flex_int>(*(ss.ss_integer)))
      , ss_general(new space_saving<flexible_type>(*(ss.ss_general)))
      , is_combined(ss.is_combined)
      , m_epsilon(ss.m_epsilon)
  {}

  space_saving_flextype(space_saving_flextype&& ss) = default;

  const space_saving_flextype& operator=(const space_saving_flextype& ss) {
    ss_integer.reset(new space_saving<flex_int>(*(ss.ss_integer)));
    ss_general.reset(new space_saving<flexible_type>(*(ss.ss_general)));
    is_combined = ss.is_combined;
    m_epsilon = ss.m_epsilon;
    return *this;
  }

  /**
   * Constructs a save saving sketch using 1 / epsilon buckets.
   * The resultant hyperloglog datastructure will 1 / epsilon memory, and
   * guarantees that all elements with occurances >= \epsilon N will be reported.
   */
  inline space_saving_flextype(double epsilon = 0.0001)
      : m_epsilon(epsilon)
  {
    ss_integer.reset(new space_saving<flex_int>(epsilon));
    ss_general.reset(new space_saving<flexible_type>(epsilon));
  }

  /**
   * Adds an item with a specified count to the sketch.
   */
  inline void add(const flexible_type& t, size_t count = 1) {
    switch(t.get_type()) {
      case flex_type_enum::INTEGER:
        ss_integer->add(t.get<flex_int>(), count);
        is_combined = false;
        break;
      case flex_type_enum::FLOAT:
        if(!std::isnan(t.get<flex_float>()))
          ss_general->add(t, count);
        break;
      case flex_type_enum::UNDEFINED:
        break;
      default:
        ss_general->add(t, count);
        break;
    }
  }

  /**
   * Returns the number of elements inserted into the sketch.
   */
  size_t size() const {
    return ss_general->size() + ss_integer->size();
  }

  /**
   * Returns all the elements tracked by the sketch as well as an
   * estimated count. The estimated can be a large overestimate.
   */
  inline std::vector<std::pair<flexible_type, size_t> > frequent_items() {
    if(!is_combined)
      _combine_integer_and_general();

    return ss_general->frequent_items();
  }

  /**
   * Returns all the elements tracked by the sketch as well as an
   * estimated count. The estimated can be a large overestimate.
   */
  inline std::vector<std::pair<flexible_type, size_t> > guaranteed_frequent_items() {
    if(!is_combined)
      _combine_integer_and_general();

    return ss_general->guaranteed_frequent_items();
  }

  /**
   * Merges a second space saving sketch into the current sketch
   */
  void combine(space_saving_flextype& other) {
    if(!is_combined)
      _combine_integer_and_general();
    if(!other.is_combined)
      other._combine_integer_and_general();

    ss_general->combine(*other.ss_general);
  }

  void clear() {
    ss_general->clear();
    ss_integer->clear();
  }

  ~space_saving_flextype() { }

 private:

  /** Combine in the integer and general hashes.
   */
  void _combine_integer_and_general() {
    ss_general->combine(*ss_integer);
    ss_integer->clear();
    is_combined = true;
  }

};

} // sketch
} // namespace turi
#endif
