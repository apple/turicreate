/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_MISSING_VALUES_H_
#define TURI_ML2_DATA_MISSING_VALUES_H_

#include <map>
#include <string>
#include <core/data/flexible_type/flexible_type.hpp>

#ifdef ERROR
#undef ERROR
#endif


namespace turi { namespace v2 { namespace ml_data_internal {

/*********************************************************************************
 *
 * Missing Value Action
 * ================================================================================
 *
 * IMPUTE
 * ------
 * Imputes the data with the mean. Do not use this during creation time because
 * the means will change over time. Imnputation only makes sense when you
 * do it during predict/evaluate time.
 *
 *
 * ERROR
 * ------
 * Error out when a missing value occurs in a numeric columns. Keys (categorical
 * variables of dictionary keys) can accept missing values.
 *
 */
enum class missing_value_action {IMPUTE, ERROR};


/** Returns the enum giving the missing_value_action based on the
 *  options.
 */
missing_value_action get_missing_value_action(
    const std::map<std::string, flexible_type>& options,
    bool training_mode);

}}}

#endif /* TURI_ML2_DATA_MISSING_VALUES_H_ */
