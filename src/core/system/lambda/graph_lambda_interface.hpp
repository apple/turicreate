/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_LAMBDA_GRAPH_LAMBDA_INTERFACE_HPP
#define TURI_LAMBDA_GRAPH_LAMBDA_INTERFACE_HPP
#include <core/storage/sgraph_data/sgraph_types.hpp>
#include <core/storage/sgraph_data/sgraph_synchronize.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/system/cppipc/magic_macros.hpp>

namespace turi {

namespace lambda {

typedef sgraph_compute::vertex_partition_exchange vertex_partition_exchange;

GENERATE_INTERFACE_AND_PROXY(graph_lambda_evaluator_interface, graph_lambda_evaluator_proxy,
      (std::vector<sgraph_edge_data>, eval_triple_apply, (const std::vector<sgraph_edge_data>&)(size_t)(size_t)(const std::vector<size_t>&))
      (void, init, (const std::string&)(size_t)(const std::vector<std::string>&)(const std::vector<std::string>&)(size_t)(size_t))
      (void, load_vertex_partition, (size_t)(std::vector<sgraph_vertex_data>&))
      (bool, is_loaded, (size_t))
      (void, update_vertex_partition, (vertex_partition_exchange&))
      (vertex_partition_exchange, get_vertex_partition_exchange, (size_t)(const std::unordered_set<size_t>&)(const std::vector<size_t>&))
      (void, clear, )
    )
} // namespace lambda
} // namespace turi

#endif
