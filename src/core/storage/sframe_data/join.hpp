/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <cstdio>
#include <boost/algorithm/string.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/join_impl.hpp>

namespace turi {

/**
 * \ingroup sframe_physical
 * \addtogroup Join Join
 *
 * Joins two SFrames.
 *
 * \param sf_left Left side of the join
 * \param sfr_right Right side of the join
 * \param join_type Either "inner", "left", "right" or "outer"
 * \param join_columns A map of columns to equijoin on.
 * \param max_buffer_size The maximum number of cells to buffer in memory.
 */
sframe join(sframe& sf_left,
            sframe& sf_right,
            std::string join_type,
            const std::map<std::string,std::string> join_columns,
            size_t max_buffer_size = SFRAME_JOIN_BUFFER_NUM_CELLS);

} // end of turicreate
