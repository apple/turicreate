/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cmath>
#include <cstddef>
#include <core/globals/globals.hpp>
#include <core/export.hpp>

namespace turi {

/**
** Global configuration for sframe, keep them as non-constants because we want to
** allow user/server to change the configuration according to the environment
**/
namespace sframe_config {
EXPORT size_t SFRAME_SORT_BUFFER_SIZE = size_t(2*1024*1024)*size_t(1024);
EXPORT size_t SFRAME_READ_BATCH_SIZE = 128;

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_SORT_BUFFER_SIZE,
                            true,
                            +[](int64_t val){ return (val >= 1024) &&
                            // Check against overflow...no more than an exabyte
                            (val <= int64_t(1024*1024*1024)*int64_t(1024*1024*1024)); });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SFRAME_READ_BATCH_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1; });

}
}
