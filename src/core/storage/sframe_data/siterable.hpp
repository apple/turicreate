/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SITERABLE_HPP
#define TURI_UNITY_SITERABLE_HPP
#include <cstddef>
#include <vector>
namespace turi {

/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * The base interface type SIterable<T> conceptually provide a simple,
 * minimal parallel InputIterator concept.
 *
 * The SIterable manages the entire collection of parallel iterators within
 * one object for simplicity.  Conceptually, the SIterable defines a sequence
 * of objects of type T which is cut into a collection of segments (#segments
 * returned by num_segments). You can get iterator of the segment via
 * (begin(segmentid), and end(segmentid)), Basically, parallel iteration can
 * be written as:
 * \code
 *  #pragma omp parallel for
 *  for (int s = 0; s < sarray.num_segments(); ++s) {
 *    auto iter = sarray.begin(s);
 *    auto end = sarray.end(s);
 *     while(iter != end) {
 *       ...
 *       ++iter;
 *     }
 *  }
 * \endcode
 */
template<typename Iterator>
class siterable {
 public:
  typedef Iterator iterator;
  typedef typename Iterator::value_type value_type;

  inline virtual ~siterable() { };

  /// Return the number of segments in the collection.
  virtual size_t num_segments() const = 0;

  /// Return the number of rows in the segment.
  virtual size_t segment_length(size_t segment) const = 0;

  /// Return the begin iterator of the segment.
  virtual Iterator begin (size_t segmentid) const = 0;

  /// Return the end iterator of the segment.
  virtual Iterator end (size_t segmentid) const = 0;

  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the begin/end iterator
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   *
   * \note This function is not always efficient. Different file formats
   * implementations will have different characteristics.
   */
  virtual size_t read_rows(size_t row_start,
                           size_t row_end,
                           std::vector<typename Iterator::value_type>& out_obj) = 0;


  /// Reset all iterators (must be called in between creating
  /// two iterators on the same segment)
  virtual void reset_iterators() = 0;
};

/// \}
} // namespace turi
#endif
