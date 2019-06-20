/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/system/lambda/lambda_utils.hpp>
#include <core/system/lambda/graph_pylambda.hpp>
#include <core/system/lambda/pylambda.hpp>
#include <core/system/lambda/python_callbacks.hpp>

namespace turi {
namespace lambda {

/**************************************************************************/
/*                                                                        */
/*                          pysgraph_synchronize                          */
/*                                                                        */
/**************************************************************************/
void pysgraph_synchronize::init(size_t num_partitions,
                                const std::vector<std::string>& vertex_keys) {
  // clear everything
  m_vertex_partitions.clear();
  m_is_partition_loaded.clear();

  // initialize members
  m_num_partitions = num_partitions;
  m_vertex_partitions.resize(m_num_partitions);
  m_is_partition_loaded.resize(m_num_partitions, false);
  m_vertex_keys = vertex_keys;
}

void pysgraph_synchronize::load_vertex_partition(size_t partition_id, std::vector<sgraph_vertex_data>& vertices) {
  DASSERT_LT(partition_id, m_num_partitions);
  DASSERT_FALSE(m_is_partition_loaded[partition_id]);
  m_vertex_partitions[partition_id] = std::move(vertices);
  m_is_partition_loaded[partition_id] = true;
  DASSERT_TRUE(is_loaded(partition_id));
}

void pysgraph_synchronize::update_vertex_partition(vertex_partition_exchange& vpartition_exchange) {
  DASSERT_TRUE(m_is_partition_loaded[vpartition_exchange.partition_id]);

  auto& vertex_partition = m_vertex_partitions[vpartition_exchange.partition_id];
  auto& fields_ids = vpartition_exchange.field_ids;
  for (auto& vid_data_pair : vpartition_exchange.vertices) {
    size_t id = vid_data_pair.first;
    sgraph_vertex_data& vdata = vid_data_pair.second;
    for (size_t i = 0; i < fields_ids.size(); ++i)
      vertex_partition[id][fields_ids[i]] = vdata[i];
  }
}

vertex_partition_exchange pysgraph_synchronize::get_vertex_partition_exchange(size_t partition_id, const std::unordered_set<size_t>& vertex_ids, const std::vector<size_t>& field_ids) {
  DASSERT_TRUE(m_is_partition_loaded[partition_id]);
  vertex_partition_exchange ret;
  ret.partition_id = partition_id;
  ret.field_ids = field_ids;

  auto& vertex_partition = m_vertex_partitions[partition_id];
  for (size_t vid:  vertex_ids) {
    auto& vdata = vertex_partition[vid];
    sgraph_vertex_data vdata_subset;
    for (auto fid: field_ids) {
      vdata_subset.push_back(vdata[fid]);
    }
    ret.vertices.push_back({vid, std::move(vdata_subset)});
  }
  return ret;
}


/**************************************************************************/
/*                                                                        */
/*                             graph_pylambda                             */
/*                                                                        */
/**************************************************************************/
graph_pylambda_evaluator::graph_pylambda_evaluator() {
}

graph_pylambda_evaluator::~graph_pylambda_evaluator() {
  if(m_lambda_id != size_t(-1)) {
    release_lambda(m_lambda_id);
  }
}

void graph_pylambda_evaluator::init(const std::string& lambda,
                                    size_t num_partitions,
                                    const std::vector<std::string>& vertex_fields,
                                    const std::vector<std::string>& edge_fields,
                                    size_t src_column_id,
                                    size_t dst_column_id) {
  clear();

  // initialize members
  size_t new_lambda_id = make_lambda(lambda);

  // If it has changed, release the old one.
  if(m_lambda_id != size_t(-1) && new_lambda_id != m_lambda_id) {
    release_lambda(m_lambda_id);
  }

  m_lambda_id = new_lambda_id;

  m_vertex_keys = vertex_fields;
  m_edge_keys = edge_fields;
  m_srcid_column = src_column_id;
  m_dstid_column = dst_column_id;
  m_graph_sync.init(num_partitions, vertex_fields);
}

void graph_pylambda_evaluator::clear() {
  m_vertex_keys.clear();
  m_edge_keys.clear();
  m_graph_sync.clear();
  m_srcid_column = -1;
  m_dstid_column = -1;
}

std::vector<sgraph_edge_data> graph_pylambda_evaluator::eval_triple_apply(
    const std::vector<sgraph_edge_data>& all_edge_data,
    size_t src_partition, size_t dst_partition,
    const std::vector<size_t>& mutated_edge_field_ids) {

  std::lock_guard<mutex> lg(m_mutex);

  logstream(LOG_INFO) << "graph_lambda_worker eval triple apply " << src_partition
                      << ", " << dst_partition << std::endl;

  DASSERT_TRUE(is_loaded(src_partition));
  DASSERT_TRUE(is_loaded(dst_partition));

  auto& source_partition = m_graph_sync.get_partition(src_partition);
  auto& target_partition = m_graph_sync.get_partition(dst_partition);

  std::vector<std::string> mutated_edge_keys;
  for (size_t fid: mutated_edge_field_ids) {
    mutated_edge_keys.push_back(m_edge_keys[fid]);
  }

  std::vector<sgraph_edge_data> ret(all_edge_data.size());

  lambda_graph_triple_apply_data lgt;

  lgt.all_edge_data = &all_edge_data;
  lgt.out_edge_data = &ret;
  lgt.source_partition = &source_partition;
  lgt.target_partition = &target_partition;
  lgt.vertex_keys = &m_vertex_keys;
  lgt.edge_keys = &m_edge_keys;
  lgt.mutated_edge_keys = &mutated_edge_keys;
  lgt.srcid_column = m_srcid_column;
  lgt.dstid_column = m_dstid_column;

  evaluation_functions.eval_graph_triple_apply(m_lambda_id, &lgt);
  python::check_for_python_exception();

  return ret;
}

}
}
