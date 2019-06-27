/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/lambda/lambda_constants.hpp>
#include <core/util/bitops.hpp>
#include <core/globals/globals.hpp>

namespace turi {

size_t DEFAULT_NUM_PYLAMBDA_WORKERS = 16;

size_t DEFAULT_NUM_GRAPH_LAMBDA_WORKERS = 16;

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            DEFAULT_NUM_PYLAMBDA_WORKERS,
                            true,
                            +[](int64_t val){ return val >= 1; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            DEFAULT_NUM_GRAPH_LAMBDA_WORKERS,
                            true,
                            +[](int64_t val){ return val >= 1; });
}
