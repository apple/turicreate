/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SARRAY_BUILDER_HPP
#define TURI_UNITY_SARRAY_BUILDER_HPP

#include <vector>
#include <core/storage/sframe_data/sarray.hpp>
#include <boost/circular_buffer.hpp>
#include <model_server/lib/api/unity_sarray_builder_interface.hpp>

namespace turi {

// forward declarations
template <typename T>
class sarray;

/**
 * Provides a Python interface to incrementally build an SArray.
 *
 * Unlike most other unity objects, this is not a wrapper of another
 * "sarray_builder" class, but provides the implementation. This is because it
 * is a slightly embellished wrapper around the SArray's output iterator, so
 * there is no further functionality that needs to be available for the C++
 * side.
 *
 * The unity_sarray_builder is designed to append values until \ref close is
 * called, which returns the SArray. No "reopening" is allowed, and no
 * operations in that instance of unity_sarray_builder will work after close is
 * called.
 */
class unity_sarray_builder: public unity_sarray_builder_base {
 public:
  /**
   * Default constructor. Does nothing
   */
  unity_sarray_builder() {}

  /**
   * Initialize the unity_sarray_buidler.
   *
   * This essentially opens the output iterator for writing.
   *
   */
  void init(size_t num_segments, size_t history_size, flex_type_enum dtype);

  /**
   * Add a single flexible_type value to the SArray.
   *
   * The segment number allows the user to use the parallel interface provided
   * by the underlying output_iterator.
   *
   * Throws if:
   *  - init hasn't been called or close has been called
   *  - segment number is invalid
   *  - the type of \p val differs from the type given in \ref init
   *
   */
  void append(const flexible_type &val, size_t segment);

  /**
   * A wrapper around \ref append which adds multiple flexible_types to SArray.
   *
   * Throws if:
   *  - init hasn't been called or close has been called
   *  - segment number is invalid
   *  - the type of any values in \p vals differs from
   *    the type given in \ref init
   */
  void append_multiple(const std::vector<flexible_type> &vals, size_t segment);

  /**
   * Return the current type of the SArray.
   */
  flex_type_enum get_type();

  /**
   * Return the last \p num_elems elements appended.
   */
  std::vector<flexible_type> read_history(size_t num_elems, size_t segment);

  /**
   * Finalize SArray and return it.
   */
  std::shared_ptr<unity_sarray_base> close();

  unity_sarray_builder(const unity_sarray_builder&) = delete;
  unity_sarray_builder& operator=(const unity_sarray_builder&) = delete;
 private:
  /// Methods

  /// Variables
  bool m_inited = false;
  bool m_closed = false;
  std::shared_ptr<sarray<flexible_type>> m_sarray;
  std::vector<sarray<flexible_type>::iterator> m_out_iters;
  flex_type_enum m_given_dtype = flex_type_enum::UNDEFINED;
  std::set<flex_type_enum> m_types_inserted;

  std::vector<std::shared_ptr<boost::circular_buffer<flexible_type>>> m_history;
};

} // namespace turi
#endif // TURI_UNITY_SARRAY_BUILDER_HPP
