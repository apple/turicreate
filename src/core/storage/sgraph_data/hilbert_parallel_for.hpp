/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_HILBERT_PARALLE_FOR_HPP
#define TURI_SGRAPH_HILBERT_PARALLE_FOR_HPP
#include <utility>
#include <functional>
#include <core/parallel/lambda_omp.hpp>
#include <core/storage/sgraph_data/hilbert_curve.hpp>
#include <core/storage/sgraph_data/sgraph_constants.hpp>
#include <core/util/blocking_queue.hpp>

namespace turi {



/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute SGraph Compute
 * \{
 */

/**
 * Graph Computation Functions
 */
namespace sgraph_compute {

/**
 * This performs a parallel sweep over an n*n grid following the Hilbert
 * curve ordering. The parallel sweep is broken into two parts. A "preamble"
 * callback which is called sequentially, which contains a list of all the
 * coordinates to be executed in the next pass, and a function which is
 * executed on every coordinate in the pass.
 *
 * The function abstractly implements the following:
 *
 * \code
 * for i = 0 to n*n step parallel_limit
 *   // collect all the coordinates to be run in this pass
 *   std::vector<pair<size_t, size_t> > coordinates
 *   for j = i to min(i + parallel_limit, n*n)
 *      coordinates.push_back(convert_hilbert_curve_to_coordinates(j))
 *   // run the preamble
 *   preamble(coordinates)
 *
 *   parallel for over coordinate in coordinates:
 *      fn(coordinate)
 * \endcode
 *
 * n must be at least 2 and a power of 2.
 */
inline void hilbert_blocked_parallel_for(size_t n,
                                  std::function<void(std::vector<std::pair<size_t, size_t> >) > preamble,
                                  std::function<void(std::pair<size_t, size_t>)> fn,
                                  size_t parallel_limit = SGRAPH_HILBERT_CURVE_PARALLEL_FOR_NUM_THREADS) {
  for (size_t i = 0;i < n*n; i += parallel_limit) {
    std::vector<std::pair<size_t, size_t> >  coordinates;
    // accumulate the list of coordinates to run
    size_t lastcoord_this_pass = std::min(i + parallel_limit, n*n);
    for(size_t j = i; j < lastcoord_this_pass; ++j) {
      coordinates.push_back(hilbert_index_to_coordinate(j, n));
    }
    preamble(coordinates);
    parallel_for(coordinates.begin(), coordinates.end(), fn);
  }
}

/**
 * Non blocking version.
 */
inline void hilbert_parallel_for(size_t n,
                                 std::function<void(std::vector<std::pair<size_t, size_t> >) > preamble,
                                 std::function<void(std::pair<size_t, size_t>)> fn) {

  blocking_queue<std::pair<size_t, size_t> > coordinates_queue;
  std::vector<std::pair<size_t, size_t> > coordinates;
  for (size_t i = 0;i < n*n; i ++) {
    auto coord = hilbert_index_to_coordinate(i, n);
    coordinates_queue.enqueue(coord);
    coordinates.push_back(coord);
  }
  preamble(coordinates);

  parallel_for(0, n*n, [&](size_t i) {
    auto coord_success_pair = coordinates_queue.dequeue();
    ASSERT_TRUE(coord_success_pair.second);
    fn(coord_success_pair.first);
  });
}

}  // sgraph_compute

/// \}
} // turicreate
#endif // TURI_SGRAPH_HILBERT_PARALLE_FOR_HPP
