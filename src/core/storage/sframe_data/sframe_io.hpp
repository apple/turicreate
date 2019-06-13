/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_IO_HPP
#define TURI_SFRAME_IO_HPP

#include<core/data/flexible_type/flexible_type.hpp>
#include<core/storage/serialization/oarchive.hpp>

class JSONNode;

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * Write a csv string of a vector of flexible_types (as a row in the sframe) to buffer.
 * Return the number of bytes written.
 */
size_t sframe_row_to_csv(const std::vector<flexible_type>& row, char* buf, size_t buflen);

/**
 * Write column_names and column_values (as a row in the sframe) to JSONNode.
 */
void sframe_row_to_json(const std::vector<std::string>& column_names,
                        const std::vector<flexible_type>& column_values,
                        JSONNode& node);

/// \}
}

#endif
