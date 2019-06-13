/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_TYPES_HPP
#define TURI_SGRAPH_SGRAPH_TYPES_HPP

#include<core/data/flexible_type/flexible_type.hpp>

namespace turi {

/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_main Main SGraph Objects
 * \{
 */

/**
 * The type of data that can be stored on vertices
 */
typedef std::vector<flexible_type> sgraph_vertex_data;

/**
 * The type of data that can be stored on edges
 */
typedef std::vector<flexible_type> sgraph_edge_data;

}

#endif
