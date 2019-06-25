/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_CONTANTS_HPP
#define TURI_SGRAPH_SGRAPH_CONTANTS_HPP
#include <cstddef>

namespace turi {


/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_main Main SGraph Objects
 * \{
 */

/**
 * Graph Computation Functions
 */
/**
 * Number of locks used for sgraph triple apply.
 */
extern size_t SGRAPH_TRIPLE_APPLY_LOCK_ARRAY_SIZE;

/**
 * Number of locks used for sgraph batch triple apply (used for python lambda).
 */
extern size_t SGRAPH_BATCH_TRIPLE_APPLY_LOCK_ARRAY_SIZE;

/**
 * Number of edges to for graph triple_apply to work on as a unit.
 */
extern size_t SGRAPH_TRIPLE_APPLY_EDGE_BATCH_SIZE;

/**
 * The default number of sgraph partitions
 */
extern size_t SGRAPH_DEFAULT_NUM_PARTITIONS;

/**
 * Buffer size for vertex deduplication during graph ingress
 */
extern size_t SGRAPH_INGRESS_VID_BUFFER_SIZE;

/**
 * Number of threads used for hilber curve parallel for
 */
extern size_t SGRAPH_HILBERT_CURVE_PARALLEL_FOR_NUM_THREADS;
}

/// \}
#endif
