/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sgraph_data/sgraph_constants.hpp>
#include <core/util/bitops.hpp>
#include <core/globals/globals.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/export.hpp>

namespace turi {

EXPORT size_t SGRAPH_TRIPLE_APPLY_LOCK_ARRAY_SIZE = 1024 * 1024;
EXPORT size_t SGRAPH_BATCH_TRIPLE_APPLY_LOCK_ARRAY_SIZE = 1024 * 1024;
EXPORT size_t SGRAPH_TRIPLE_APPLY_EDGE_BATCH_SIZE = 1024;
EXPORT size_t SGRAPH_DEFAULT_NUM_PARTITIONS = 8;
EXPORT size_t SGRAPH_INGRESS_VID_BUFFER_SIZE = 1024 * 1024 * 1;
EXPORT size_t SGRAPH_HILBERT_CURVE_PARALLEL_FOR_NUM_THREADS = thread::cpu_count();

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_TRIPLE_APPLY_LOCK_ARRAY_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_BATCH_TRIPLE_APPLY_LOCK_ARRAY_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_TRIPLE_APPLY_EDGE_BATCH_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1; });


REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_DEFAULT_NUM_PARTITIONS,
                            true,
                            +[](int64_t val){ return val >= 1 && is_power_of_2((uint64_t)val); });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_INGRESS_VID_BUFFER_SIZE,
                            true,
                            +[](int64_t val){ return val >= 1; });

REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            SGRAPH_HILBERT_CURVE_PARALLEL_FOR_NUM_THREADS,
                            true,
                            +[](int64_t val){ return val >= 1; });
}
