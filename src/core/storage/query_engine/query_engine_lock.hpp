/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_HPP
#define TURI_SFRAME_QUERY_ENGINE_HPP
#include <thread>
namespace turi {
class recursive_mutex;

/**
 * SFrame Lazy Evaluation and Execution
 */
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * A global lock around all external entry points to the query execution.
 * For now,
 * - materialize()
 * - infer_planner_node_type()
 * - infer_planner_node_length()
 */
extern recursive_mutex global_query_lock;
} // query_eval
} // turicreate
#endif
