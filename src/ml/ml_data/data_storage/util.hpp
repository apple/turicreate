/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DATA_STORAGE_UTIL_H_
#define TURI_DATA_STORAGE_UTIL_H_

#include <ml/ml_data/metadata.hpp>
#include <core/storage/sframe_data/sarray.hpp>

namespace turi { namespace ml_data_internal {

/** Estimate the number of rows to stick into one block.  The goal is
 *  to have it about ML_DATA_TARGET_ROW_BYTE_MINIMUM bytes per block.
 *
 */
size_t estimate_row_block_size(
    size_t original_sframe_num_rows,
    const row_metadata& rm,
    const std::vector<std::shared_ptr<sarray<flexible_type>::reader_type> >& column_readers);

}}

#endif /* TURI_DATA_STORAGE_UTIL_H_ */
