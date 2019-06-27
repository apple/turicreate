/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef SFRAME_ALGORITHM_EC_PERMUTE_HPP
#define SFRAME_ALGORITHM_EC_PERMUTE_HPP


#include <vector>
#include <memory>
/*
 * See ec_sort.hpp for details
 */
namespace turi {
class sframe;

template <typename T>
class sarray;

namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup Algorithms Algorithms
 * \{
 */
/**
 * Permutes an sframe by a forward map.
 * forward_map has the same length as the sframe and must be a permutation
 * of all the integers [0, len-1].
 *
 * The input sframe is then permuted so that sframe row i is written to row
 * forward_map[i] of the returned sframe.
 *
 * \note The forward_map is not checked that it is a valid permutation
 * If the constraints is not met, either an exception will be thrown, or
 * the result is ill-defined.
 */
sframe permute_sframe(sframe &values_sframe,
                      std::shared_ptr<sarray<flexible_type> > forward_map);

/// \}
} // query_eval
} // turicreate

#endif
