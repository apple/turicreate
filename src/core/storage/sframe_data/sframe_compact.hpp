/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_COMPACT_HPP
#define TURI_SFRAME_COMPACT_HPP
#include <vector>
#include <memory>
namespace turi {
class sframe;

template <typename T>
class sarray;
/**
 * sframe_fast_compact looks for runs of small segments
 * (comprising of less than FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT block), and
 * rebuilds them into bigger segments.
 * Returns true if any compaction was performed.
 */
bool sframe_fast_compact(const sframe& sf);

/**
 * Inplace compacts an SFrame. Fast compact is tried first and if
 * the number of segments do not fall below the target, a slow compaction
 * is performed.
 */
void sframe_compact(sframe& sf, size_t segment_threshold);

/**
 * sarray_fast_compact looks for runs of small segments
 * (comprising of less than FAST_COMPACT_BLOCKS_IN_SMALL_SEGMENT block), and
 * rebuilds them into bigger segments.
 * Returns true if any compaction was performed.
 */
template <typename T>
bool sarray_fast_compact(sarray<T>& column);

/**
 * Inplace compacts an SArray. Fast compact is tried first and if
 * the number of segments do not fall below the target, a slow compaction
 * is performed.
 */
template <typename T>
void sarray_compact(sarray<T>& column, size_t segment_threshold);

} // turi
#endif
