/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SWRITER_BASE_HPP
#define TURI_UNITY_SWRITER_BASE_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/siterable.hpp>
namespace turi {



/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * Base class for a basic parallel writer interface.
 *
 * Also see \ref siterable for the reader interface
 */
template<typename Iterator>
class swriter_base {
 public:
  typedef Iterator iterator;
  typedef typename Iterator::value_type value_type;

  virtual ~swriter_base() { };

  /** Sets the number of parallel output segments.
   *  Returns true if the number of segments is set successfully,
   *  false otherwise. Generally speaking, once an output iterator has been
   *  obtained, the number of segments can no longer be changed.
   *
   *  \param numseg A value greater than 0
   */
  virtual bool set_num_segments(size_t numseg) = 0;


  /// Returns the number of parallel output segments
  virtual size_t num_segments() const = 0;

  /// Gets an output iterator to the specified segment
  virtual iterator get_output_iterator(size_t segmentid) = 0;

  /**
   * Closes the array completely. This implicitly closes all segments.
   * After the writer is closed, no segments can be written.
   * And only after the write is finalized, that the result of the swriter
   * can be given to an sarray for reading.
   */
  virtual void close() = 0;
};

/// \}
} // namespace turi
#endif
