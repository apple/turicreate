/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_USER_ITEM_LISTS_H_
#define TURI_RECSYS_USER_ITEM_LISTS_H_

#include <toolkits/ml_data_2/ml_data.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <memory>
#include <vector>

namespace turi { namespace recsys {

/** Make users' (item, rating) lists by user.
 *
 *  In this case, the ml_data structure for the user-item lists is
 *  must be sorted by rows, with the first column being the user
 *  column.
 *
 *  This operation is done without loading the data into memory.
 *
 *  The user column is assumed to be the first column, and the item
 *  column is assumed to be the second column.
 *
 *  \param[in] data An ml_data object sorted first by users (first
 *  column), then by items (second column).
 *
 *  \return An SArray of vectors of pairs of flex_int, flex_float, where the
 *  first index is the item and the second index is the rating.
 */
std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >
make_user_item_lists(const v2::ml_data& data);

}}

#endif /* _USER_ITEM_LISTS_H_ */
