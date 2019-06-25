/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_TRAIN_TEST_SPLIT_H_
#define TURI_RECSYS_TRAIN_TEST_SPLIT_H_

#include <cstdint>
#include <string>
#include <core/storage/sframe_data/sframe.hpp>

namespace turi { namespace recsys {

std::pair<sframe, sframe> make_recsys_train_test_split(
    sframe data,
    const std::string& user_column_name,
    const std::string& item_column_name,
    size_t max_num_users, double item_test_proportion,
    size_t random_seed);


}}

#endif /* TURI_RECSYS_TRAIN_TEST_SPLIT_H_ */
