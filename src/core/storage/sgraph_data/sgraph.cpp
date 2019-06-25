/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sgraph_data/hilbert_parallel_for.hpp>
#include <core/storage/sframe_data/shuffle.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/sframe_data/sarray_sorted_buffer.hpp>
#include <core/storage/sframe_data/sarray_reader_buffer.hpp>
#include <core/storage/sframe_data/sframe_saving.hpp>
#include <atomic>
#include <core/system/platform/timer//timer.hpp>
#include <sparsehash/sparse_hash_set>

/**
 * Specialization of hash of pairs.
 */
namespace std {
template<>
struct hash<pair<size_t, size_t>> {
  size_t operator()(const pair<size_t, size_t>& x) const {
    return (hash<size_t>()(x.first)) ^ (hash<size_t>()(x.second));
  }
};

template<>
struct hash<pair<turi::flexible_type, turi::flexible_type>> {
  size_t operator()(const pair<turi::flexible_type, turi::flexible_type>& x) const {
    return turi::hash64_combine(x.first.hash(), x.second.hash());
  }
};
}

namespace turi {

const char* sgraph::DEFAULT_GROUP_NAME = "default";
const char* sgraph::VID_COLUMN_NAME = "__id";
const char* sgraph::SRC_COLUMN_NAME = "__src_id";
const char* sgraph::DST_COLUMN_NAME = "__dst_id";
const flex_type_enum sgraph::INTERNAL_ID_TYPE = flex_type_enum::INTEGER;

/**************************************************************************/
/*                                                                        */
/*                              Constructors                              */
/*                                                                        */
/**************************************************************************/
sgraph::sgraph(size_t num_partitions) {
  ASSERT_GT(num_partitions, 0);
  init(num_partitions);
}

void sgraph::init(size_t num_partitions) {
  clear();
  ASSERT_TRUE(is_power_of_2(num_partitions));
  m_num_partitions = num_partitions;
  m_num_groups = 1;
  m_vertex_group_names.push_back(DEFAULT_GROUP_NAME);
  // create a vector of m_num_partitions sframes for each vertex group
  m_vertex_groups.push_back(std::vector<sframe>(m_num_partitions));
  for (auto& sf : m_vertex_groups[0]) {
    init_empty_sframe(sf, {VID_COLUMN_NAME}, {m_vid_type});
  }

  // create a vector of m_num_partitions*m_num_partitions sframes for each edge group
  m_edge_groups.insert({{0,0},
    std::vector<sframe>(m_num_partitions * m_num_partitions)});
  for (auto& sf : m_edge_groups[{0,0}]) {
    init_empty_sframe(sf, {SRC_COLUMN_NAME, DST_COLUMN_NAME}, {m_vid_type, m_vid_type});
  }
}

void sgraph::bootstrap_vertex_id_type(flex_type_enum id_type) {
  if ((id_type != flex_type_enum::INTEGER) &&
      (id_type != flex_type_enum::STRING)) {
    log_and_throw("Vertex id type must be either integer or string");
  }
  ASSERT_EQ(num_edges(), 0);
  ASSERT_EQ(num_vertices(), 0);
  m_vid_type = id_type;
  for (auto& g: m_vertex_groups) {
    for (auto& sf : g) {
      init_empty_sframe(sf, {VID_COLUMN_NAME}, {m_vid_type});
    }
  }
  for (auto& g: m_edge_groups) {
    for (auto& sf : g.second) {
      init_empty_sframe(sf, {SRC_COLUMN_NAME, DST_COLUMN_NAME},
                        {INTERNAL_ID_TYPE, INTERNAL_ID_TYPE});
    }
  }
}

void sgraph::increase_number_of_groups(size_t num_groups) {
  ASSERT_GT(num_groups, m_num_groups);
  for (size_t i = m_num_groups; i < num_groups; ++i) {
    m_vertex_groups.push_back(std::vector<sframe>(m_num_partitions));
    for (auto& sf : m_vertex_groups[i]) {
      init_empty_sframe(sf, {VID_COLUMN_NAME}, {m_vid_type});
    }

    for (size_t from_group = 0; from_group < num_groups; ++from_group) {
      for (size_t to_group = 0; to_group < num_groups; ++to_group) {
        if (from_group < m_num_groups && to_group < m_num_groups) {
          continue;
        } else {
          m_edge_groups.insert({{from_group, to_group}, std::vector<sframe>(m_num_partitions * m_num_partitions)});
          for (auto& sf : m_edge_groups[{from_group,to_group}]) {
            init_empty_sframe(sf, {SRC_COLUMN_NAME, DST_COLUMN_NAME}, {INTERNAL_ID_TYPE, INTERNAL_ID_TYPE});
          }
        }
      }
    }
  }
  m_num_groups = num_groups;
}

/**************************************************************************/
/*                                                                        */
/*                               Observers                                */
/*                                                                        */
/**************************************************************************/
std::vector<std::string> sgraph::get_vertex_fields(size_t groupid) const {
  auto& vgroup = vertex_group(groupid);
  ASSERT_EQ(vgroup.size(), m_num_partitions);
  return vgroup[0].column_names();
}

std::vector<flex_type_enum> sgraph::get_vertex_field_types(size_t groupid) const {
  auto& vgroup = vertex_group(groupid);
  ASSERT_EQ(vgroup.size(), m_num_partitions);
  return vgroup[0].column_types();
}

std::vector<std::string> sgraph::get_edge_fields(size_t groupa,
                                                 size_t groupb) const {
  auto& egroup = edge_group(groupa, groupb);
  ASSERT_EQ(egroup.size(), m_num_partitions*m_num_partitions);
  return egroup[0].column_names();
}

std::vector<flex_type_enum> sgraph::get_edge_field_types(size_t groupa,
                                                         size_t groupb) const {
  auto& egroup = edge_group(groupa, groupb);
  ASSERT_EQ(egroup.size(), m_num_partitions*m_num_partitions);

  auto ret = egroup[0].column_types();

  size_t id_column_idx = get_vertex_field_id(VID_COLUMN_NAME);
  flex_type_enum vid_type = get_vertex_field_types()[id_column_idx];

  size_t src_column_idx = egroup[0].column_index(SRC_COLUMN_NAME);
  size_t dst_column_idx = egroup[0].column_index(DST_COLUMN_NAME);

  ret[src_column_idx] = ret[dst_column_idx] = vid_type;
  return ret;
}

sframe sgraph::get_vertices(const std::vector<flexible_type>& vid_vec,
                            const options_map_t& field_constraint,
                            size_t group) const {
  sframe ret;

  // no vertices, return
  if (num_vertices(group) == 0) {
    ret.open_for_write(get_vertex_fields(), get_vertex_field_types());
    ret.close();
    return ret;
  }

  std::vector<std::string> vfields = get_vertex_fields(group);
  const std::vector<sframe>& vgroup = vertex_group(group);

  std::vector< std::pair<size_t, flexible_type> > value_constraint;
  for (const auto& kv : field_constraint) {
    value_constraint.push_back({vgroup[0].column_index(kv.first), kv.second});
  }

  size_t vid_column_idx = vgroup[0].column_index(VID_COLUMN_NAME);
  std::unordered_set<flexible_type> vid_constraint(vid_vec.begin(), vid_vec.end());

  // fast pass if no filter is needed
  if (vid_vec.empty() && field_constraint.empty()) {
    for (auto& sf: vgroup) {
      ret = ret.append(sf);
    }
    return ret;
  }

  // We need to filter some vertices
  std::function<bool(const std::vector<flexible_type>&)> filter_fn;

  std::function<bool(const std::vector<flexible_type>&)> value_filter =
      [&](const std::vector<flexible_type>& row) {
        for (auto& kv : value_constraint) {
          const flexible_type& actual = row[kv.first];
          const flexible_type& expected = kv.second;
          if (!(
                (expected.get_type() == flex_type_enum::UNDEFINED) ||
                (actual == expected)
               )) {
            return false;
          }
        }
        return true;
      };

  if (vid_vec.empty()) {
    filter_fn = value_filter;
  } else if (field_constraint.empty()) {
    filter_fn = [&](const std::vector<flexible_type>& row) {
                  return vid_constraint.count(row[vid_column_idx]);
                };
  } else {
    filter_fn = [&](const std::vector<flexible_type>& row) {
                  return ((vid_constraint.find(row[vid_column_idx]) != vid_constraint.end())
                          && value_filter(row));
                };
  }

  for (auto& sf_in: vgroup) {
    sframe sf_out;
    sf_out.open_for_write(sf_in.column_names(), sf_in.column_types(), "", sf_in.num_segments());
    copy_if (sf_in, sf_out, filter_fn);
    sf_out.close();
    ret = ret.append(sf_out);
  }
  return ret;
}

sframe sgraph::get_edges(const std::vector<flexible_type>& source_vids,
                         const std::vector<flexible_type>& target_vids,
                         const options_map_t& field_constraint,
                         size_t groupa, size_t groupb) const {
  sframe ret;
  if (num_edges(groupa, groupb) == 0) {
    ret.open_for_write(get_edge_fields(), get_edge_field_types());
    ret.close();
    return ret;
  }

  std::vector<std::string> efields = get_edge_fields(groupa, groupb);
  const std::vector<sframe>& egroup = edge_group(groupa, groupb);

  // Configure the field constraints
  std::vector< std::pair<size_t, flexible_type> > value_constraint;
  for (const auto& kv : field_constraint) {
    value_constraint.push_back({egroup[0].column_index(kv.first), kv.second});
  }

  // lambda for checking value constraint
  std::function<bool(const std::vector<flexible_type>&)> satisfy_value_constraint =
      [&](const std::vector<flexible_type>& edge_data) {
        for(const auto& kv : value_constraint) {
          const flexible_type& actual = edge_data[kv.first];
          const flexible_type& expected = kv.second;
          if (!(
                (expected.get_type() == flex_type_enum::UNDEFINED)
                || (actual == expected))) {
            return false;
          }
        }
        return true;
      };

  // column indices of the source vid and target vid
  size_t src_column_idx = egroup[0].column_index(SRC_COLUMN_NAME);
  size_t dst_column_idx = egroup[0].column_index(DST_COLUMN_NAME);

  // lambda for transform edge ids
  std::function<std::vector<flexible_type>(const std::vector<flexible_type>&,
                                           const std::vector<flexible_type>& source_vids,
                                           const std::vector<flexible_type>& target_vids)>
      edge_id_transform = [&](const std::vector<flexible_type>& row,
                              const std::vector<flexible_type>& source_vids,
                              const std::vector<flexible_type>& target_vids) {
        std::vector<flexible_type> ret = row;
        size_t src_idx = row[src_column_idx];
        size_t dst_idx = row[dst_column_idx];
        DASSERT_LT(src_idx, source_vids.size());
        DASSERT_LT(dst_idx, target_vids.size());
        ret[src_column_idx] = source_vids[src_idx];
        ret[dst_column_idx] = target_vids[dst_idx];
        return ret;
      };

  // Cache of {partition, group} -> List[vertex_ids]
  // Preamble functions will load the requied ids into this cache.
  std::unordered_map<std::pair<size_t, size_t>, std::vector<flexible_type>>
    partition_vid_cache;

  std::function<void(std::vector<std::pair<size_t, size_t>>)> load_partition_vids =
    [&](std::vector<std::pair<size_t, size_t>> coordinates) {
      std::set<std::pair<size_t, size_t>> pairs_to_load;
      std::set<std::pair<size_t, size_t>> pairs_to_unload;
      for (auto& c: coordinates) {
        pairs_to_load.insert({c.first, groupa});
        pairs_to_load.insert({c.second, groupb});
      }
      for (auto& kv: partition_vid_cache) {
        if (pairs_to_load.count(kv.first)) {
          pairs_to_load.erase(pairs_to_load.find(kv.first));
        } else {
          pairs_to_unload.insert(kv.first);
        }
      }
      for (auto& pair: pairs_to_unload) {
        partition_vid_cache.erase(partition_vid_cache.find(pair));
      }
      for (auto& pair: pairs_to_load) {
        partition_vid_cache[pair] = {};
      }
      std::vector<std::pair<size_t, size_t>> pairs_to_load_vec(
          pairs_to_load.begin(), pairs_to_load.end());
      parallel_for(0, pairs_to_load_vec.size(), [&](size_t i) {
        auto coord = pairs_to_load_vec[i];
        partition_vid_cache[coord] = get_vertex_ids(coord.first, coord.second);
      });
    };

  // Configure the id constraints
  bool match_all_vertices = true;
  for (size_t i = 0; i < source_vids.size(); ++i) {
    if (!((source_vids[i].get_type() == flex_type_enum::UNDEFINED)
        && (target_vids[i].get_type() == flex_type_enum::UNDEFINED))) {
      match_all_vertices = false;
      break;
    }
  }

  std::vector<sframe> out_edge_blocks(m_num_partitions * m_num_partitions);
  // Case 1: there is no source or target id constraints.
  if (match_all_vertices) {
    sgraph_compute::hilbert_blocked_parallel_for(
        get_num_partitions(),
        load_partition_vids,
        [&](std::pair<size_t, size_t> coordinate) {
          size_t i = coordinate.first;
          size_t j = coordinate.second;
          const std::vector<flexible_type>& src_partition_vids = partition_vid_cache.at({i, groupa});
          const std::vector<flexible_type>& dst_partition_vids = partition_vid_cache.at({j, groupb});

          sframe edge_sframe = edge_partition(i, j, groupa, groupb);
          std::vector<flex_type_enum> out_column_types = edge_sframe.column_types();
          out_column_types[src_column_idx] = m_vid_type;
          out_column_types[dst_column_idx] = m_vid_type;

          sframe out_sframe;
          out_sframe.open_for_write(edge_sframe.column_names(), out_column_types,
                                    "", edge_sframe.num_segments());
          copy_transform_if(edge_sframe, out_sframe, satisfy_value_constraint,
                            boost::bind(edge_id_transform, _1,
                                        boost::cref(src_partition_vids),
                                        boost::cref(dst_partition_vids)));
          out_sframe.close();
          out_edge_blocks[i * m_num_partitions + j] = std::move(out_sframe);
        });
  // Case 2: reorganize the id constraints into partitions.
  } else {
    // separate the source/target vertices that are matching wildcards.
    std::vector<std::unordered_set<flexible_type>> wild_source_vids(m_num_partitions);
    std::vector<std::unordered_set<flexible_type>> wild_target_vids(m_num_partitions);
    // normal source target id constraints:  map from (src_partition, dst_partition) -> set<(src_id, dst_id>
    std::unordered_map<std::pair<size_t, size_t>, std::unordered_set<std::pair<flexible_type, flexible_type>>>
      vid_constraints;
    for (size_t i = 0; i < m_num_partitions; ++i) {
      for (size_t j = 0; j < m_num_partitions; ++j) {
        vid_constraints[{i,j}] = {};
      }
    }

    for (size_t i = 0; i < source_vids.size(); ++i) {
      const flexible_type& source = source_vids[i];
      const flexible_type& target = target_vids[i];
      size_t source_pid = source.hash() % m_num_partitions;
      size_t target_pid = target.hash() % m_num_partitions;
      if (source.get_type() == flex_type_enum::UNDEFINED) {
        wild_target_vids[target_pid].insert(target);
      } else if (target.get_type() == flex_type_enum::UNDEFINED) {
        wild_source_vids[source_pid].insert(source);
      } else {
        vid_constraints[{source_pid, target_pid}].insert({source, target});
      }
    }
    sgraph_compute::hilbert_blocked_parallel_for(
        get_num_partitions(),
        load_partition_vids,
        [&](std::pair<size_t, size_t> coordinate) {
        size_t i = coordinate.first;
        size_t j = coordinate.second;
        const std::vector<flexible_type>& src_partition_vids = partition_vid_cache.at({i, groupa});
        const std::vector<flexible_type>& dst_partition_vids = partition_vid_cache.at({j, groupb});
        sframe edge_sframe = edge_partition(i, j, groupa, groupb);

        std::vector<flex_type_enum> out_column_types = edge_sframe.column_types();
        out_column_types[src_column_idx] = m_vid_type;
        out_column_types[dst_column_idx] = m_vid_type;

        sframe out_sframe;
        out_sframe.open_for_write(edge_sframe.column_names(), out_column_types,
                                  "", edge_sframe.num_segments());

        // The filter function checks the id constraints and then value constraints
        std::function<bool(const std::vector<flexible_type>&)> filter_fn =
            [&](const std::vector<flexible_type>& row) {
              size_t src_idx = row[src_column_idx];
              size_t dst_idx = row[dst_column_idx];
              const flexible_type& source = src_partition_vids[src_idx];
              const flexible_type& target = dst_partition_vids[dst_idx];
              std::pair<size_t, size_t> partition_addr{i, j};
              std::pair<flexible_type, flexible_type> source_target_pair{source, target};
              if ((wild_source_vids[i].count(source) || wild_target_vids[j].count(target)) ||
                  (vid_constraints.at(partition_addr).count(source_target_pair) > 0)) {
               return satisfy_value_constraint(row);
              }
              return false;
            };

        copy_transform_if(edge_sframe, out_sframe, filter_fn,
                          boost::bind(edge_id_transform, _1,
                                      boost::cref(src_partition_vids),
                                      boost::cref(dst_partition_vids)));
        out_sframe.close();
        out_edge_blocks[i * m_num_partitions + j] = std::move(out_sframe);
    });
  }
  for (auto& sf : out_edge_blocks)
    ret = ret.append(sf);
  return ret;
}

/**************************************************************************/
/*                                                                        */
/*                               Modifiers                                */
/*                                                                        */
/**************************************************************************/
bool sgraph::add_vertices(sframe vertices,
                          const std::string& id_field_name,
                          size_t group) {
  if (vertices.num_rows() == 0 || vertices.num_columns() == 0) {
    return true;
  }
  if (group >= m_num_groups) {
    increase_number_of_groups(group + 1);
  }
  DASSERT_LT(group, m_num_groups);

  size_t id_column_idx = vertices.column_index(id_field_name);
  vertices.set_column_name(id_column_idx, VID_COLUMN_NAME);

  fast_validate_add_vertices(vertices, group);

  std::vector<sframe> vertex_partitions =
    shuffle(vertices, m_num_partitions,
        [&](const std::vector<flexible_type>& row) {
          return get_vertex_partition(row[id_column_idx]);
        });
  commit_vertex_buffer(group, vertex_partitions);
  logstream(LOG_EMPH) << "Num vertices for group " << group << ": " << num_vertices(group) << std::endl;
  return true;
}

void sgraph::commit_vertex_buffer(size_t group,
                                  std::vector<sframe>& vertex_partitions) {
  DASSERT_EQ(vertex_partitions.size(), m_num_partitions);
  // We want to keep the invariant that all sframes in the group
  // maintain the same column names and column types.
  std::vector<std::string> all_column_names = get_vertex_fields(group);
  std::vector<flex_type_enum> all_column_types = get_vertex_field_types(group);
  std::unordered_map<std::string, flex_type_enum> all_columns;
  for (size_t i = 0; i < all_column_names.size(); ++i) {
    all_columns[all_column_names[i]] = all_column_types[i];
  }

  // Precompute the column_names and column_types for the group
  // after committing the buffers.
  sframe sf = vertex_partitions[0];
  for (size_t i = 0; i < sf.num_columns(); ++i) {
    std::string name = sf.column_name(i);
    flex_type_enum type = sf.column_type(i);
    if (all_columns.count(name)) {
      DASSERT_EQ((int)type, (int)all_columns[name]);
    } else {
      all_columns[name] = type;
      all_column_names.push_back(name);
      all_column_types.push_back(type);
    }
  }

  std::vector<size_t> num_vertex_added(m_num_partitions);
  parallel_for (0, m_num_partitions, [&](size_t i) {
  // for (size_t i = 0; i < m_num_partitions; ++i) {
    sframe& old_partition = vertex_partition(i, group);
    sframe& new_partition = vertex_partitions[i];
    reorder_and_add_new_columns(old_partition, all_column_names, all_column_types);
    reorder_and_add_new_columns(new_partition, all_column_names, all_column_types);
    new_partition = merge_vertex_partition(old_partition, new_partition);
    num_vertex_added[i] = (new_partition.size() - old_partition.size());
    old_partition = new_partition;
  });
  // }
  for (size_t i : num_vertex_added) {
    m_num_vertices += i;
  }
}

sframe sgraph::merge_vertex_partition(sframe& current_data,
                                      sframe& new_data) {

  size_t id_column_idx = current_data.column_index(VID_COLUMN_NAME);

  std::vector<std::vector<flexible_type>> buffer_a, buffer_b;
  std::unordered_map<flexible_type, std::vector<flexible_type>*> join_hash_map;

  // read both sframe in memory
  current_data.get_reader()->read_rows(0, current_data.size(), buffer_a);
  new_data.get_reader()->read_rows(0, new_data.size(), buffer_b);

  for (auto& row : buffer_a) {
    flexible_type vid = row[id_column_idx];
    join_hash_map[vid] = &row;
  }

  for (auto& row : buffer_b) {
    flexible_type vid = row[id_column_idx];
    if (vid.get_type() == flex_type_enum::UNDEFINED) {
      std::string error_message =
          std::string("Vertex id column cannot contain missing value. ") +
          "Please use dropna() to drop the missing value from the input and try again.";
      log_and_throw(error_message);
    }
    join_hash_map[vid] = &row;
  }

  // prepare the return sframe.
  sframe ret;
  size_t num_segments = 1; // use one logical segment
  ret.open_for_write(current_data.column_names(),
                     current_data.column_types(),
                     "", num_segments);

  auto out = ret.get_output_iterator(0);
  for (auto& row: buffer_a) {
    decltype(join_hash_map)::const_iterator iter = join_hash_map.find(row[id_column_idx]);
    *out = std::move(*(iter->second));
    ++out;
    join_hash_map.erase(iter);
  }
  for (auto& kv : join_hash_map) {
    *out = std::move(*(kv.second));
    ++out;
  }
  ret.close();
  return ret;
}

bool sgraph::add_vertices(const dataframe_t& vertices,
                          const std::string& id_field_name,
                          size_t group) {
  return add_vertices(sframe(vertices), id_field_name, group);
}

bool sgraph::add_edges(sframe edges,
                       const std::string& source_field_name,
                       const std::string& target_field_name,
                       size_t groupa, size_t groupb) {
  if (edges.num_rows() == 0 || edges.num_columns() == 0) {
    return true;
  }
  if (groupa >= m_num_groups || groupb >= m_num_groups) {
    increase_number_of_groups(std::max(groupa, groupb) + 1);
  }
  ASSERT_LT(groupa, m_num_groups);
  ASSERT_LT(groupb, m_num_groups);
  size_t src_column_idx = edges.column_index(source_field_name);
  size_t dst_column_idx = edges.column_index(target_field_name);
  edges.set_column_name(src_column_idx, SRC_COLUMN_NAME);
  edges.set_column_name(dst_column_idx, DST_COLUMN_NAME);

  fast_validate_add_edges(edges, groupa, groupb);

  commit_edge_buffer(groupa, groupb, edges);
  logstream(LOG_EMPH) << "Num vertices for group " << groupa << ": " << num_vertices(groupa) << "\n"
                      << "Num vertices for group " << groupb << ": " << num_vertices(groupb) << "\n"
                      << "Num edges " << groupa << " -> " << groupb << ": " << num_edges(groupa, groupb)
                      << std::endl;
  return true;
}

bool sgraph::add_edges(const dataframe_t& edges,
                       const std::string& source_field_name,
                       const std::string& target_field_name,
                       size_t groupa, size_t groupb) {
  return add_edges(sframe(edges), source_field_name, target_field_name, groupa, groupb);
}


void sgraph::commit_edge_buffer(size_t groupa,
                                size_t groupb,
                                sframe edges) {

  timer local_timer, global_timer;
  std::atomic<size_t> vertices_added(0);
  std::atomic<size_t> edges_added(0);

  global_timer.start();

  logstream(LOG_EMPH) << "In commit edge buffer (" << groupa << "," << groupb << ")"
                       << std::endl;

  // This function is monsterous. Bascially, 3 big steps are involved.
  // 1. Keeping track of the new vertex id that got introduced by the incoming edges.
  // 2. Add an empty vertex for each new vertex id to its vertex partition.
  // 3. Translate the source and target ids in each edge partition to
  // be the row id of the corresponding vertex in the vertex partition, and append
  // to the existing edge partition.

  /**************************************************************************/
  /*                                                                        */
  /*                                 Step 1                                 */
  /*                                                                        */
  /**************************************************************************/
  //// First pass of all edge partitions, we aggregate all unique vertices
  //// seen in each vertex partition.

  local_timer.start();
  // Prepare vertex dedupliication buffer for each partition
  typedef sarray_sorted_buffer<flexible_type> vid_buffer_type;
  std::vector<std::shared_ptr<vid_buffer_type>> vid_buffer;
  size_t vertex_partition_size = (groupa == groupb) ? m_num_partitions : (2 * m_num_partitions);
  for (size_t i = 0; i < vertex_partition_size; ++i) {
    vid_buffer.push_back(
        std::make_shared<vid_buffer_type>(SGRAPH_INGRESS_VID_BUFFER_SIZE,
                                          [](const flexible_type& a,
                                             const flexible_type& b){ return a < b; },
                                          true // deduplicate flag
                                          )
        );
  }
  std::vector<std::shared_ptr<vid_buffer_type>> source_vid_buffers;
  std::vector<std::shared_ptr<vid_buffer_type>> target_vid_buffers;
  if (groupa == groupb) {
    for (size_t i = 0; i < vertex_partition_size; ++i) {
      source_vid_buffers.push_back(vid_buffer[i]);
      target_vid_buffers.push_back(vid_buffer[i]);
    }
  } else {
    for (size_t i = 0; i < vertex_partition_size / 2; ++i) {
      source_vid_buffers.push_back(vid_buffer[i]);
      target_vid_buffers.push_back(vid_buffer[i + m_num_partitions]);
    }
  }

  size_t src_column_idx = edges.column_index(SRC_COLUMN_NAME);
  size_t dst_column_idx = edges.column_index(DST_COLUMN_NAME);

  auto add_to_deduplication_buffer = [&](const std::vector<flexible_type>& row, size_t thread_id) {
    const auto& src_id = row[src_column_idx];
    const auto& dst_id = row[dst_column_idx];
    if (src_id.get_type() == flex_type_enum::UNDEFINED) {
      std::string error_message =
        std::string("source vid column cannot contain missing value. ") +
        "Please use dropna() to drop the missing value from the input and try again";
      log_and_throw(error_message);
    }
    if (dst_id.get_type() == flex_type_enum::UNDEFINED) {
      std::string error_message =
        std::string("target vid column cannot contain missing value. ") +
        "Please use dropna() to drop the missing value from the input and try again";
      log_and_throw(error_message);
    }
    size_t src_partition = get_vertex_partition(src_id);
    size_t dst_partition = get_vertex_partition(dst_id);
    source_vid_buffers[src_partition]->add(src_id, thread_id);
    target_vid_buffers[dst_partition]->add(dst_id, thread_id);
  };

  // prepare the vertex id aggregate buffer for each partition.
  logstream(LOG_EMPH) << "Shuffling edges ..." << std::endl;
  std::vector<sframe> edge_partitions =
    shuffle(edges, m_num_partitions * m_num_partitions,
        [&](const std::vector<flexible_type>& row) {
          return get_edge_partition(row[src_column_idx], row[dst_column_idx]);
        },
        add_to_deduplication_buffer);
  DASSERT_EQ(edge_partitions.size(), m_num_partitions * m_num_partitions);
  logstream(LOG_EMPH) << "Done shuffling edges in " << local_timer.current_time() << " secs" << std::endl;


  local_timer.start();
  logstream(LOG_EMPH) << "Aggregating unique vertices..." << std::endl;
  std::vector<sarray<flexible_type>> unique_vertex_ids(vid_buffer.size());
  parallel_for(0, vid_buffer.size(), [&](size_t i) {
    vid_buffer[i]->close();
    auto& vid_array = unique_vertex_ids[i];
    {
      vid_array.open_for_write(1);
      vid_array.set_type(m_vid_type);
      auto out = vid_array.get_output_iterator(0);
      vid_buffer[i]->sort_and_write(out);
      vid_array.close();
    }
  });
  logstream(LOG_EMPH) << "Done aggregating unique vertex in " << local_timer.current_time() << " secs" << std::endl;

  /**************************************************************************/
  /*                                                                        */
  /*                                 Step 2                                 */
  /*                                                                        */
  /**************************************************************************/
  local_timer.start();
  logstream(LOG_EMPH) << "Combine vertex data" << std::endl;

  std::vector<std::string> column_names_of_first_group = get_vertex_fields(groupa);
  std::vector<flex_type_enum> column_types_of_first_group = get_vertex_field_types(groupa);

  std::vector<std::string> column_names_of_second_group = get_vertex_fields(groupb);
  std::vector<flex_type_enum> column_types_of_second_group = get_vertex_field_types(groupb);

  parallel_for(0, vid_buffer.size(), [&](size_t i) {
  // for (size_t i = 0; i < vid_buffer.size(); ++i) {
    size_t groupid, partitionid;
    timer timer;
    std::vector<std::string> column_names;
    std::vector<flex_type_enum> column_types;
    if (i < m_num_partitions) {
      groupid = groupa;
      partitionid = i;
      column_names = column_names_of_first_group;
      column_types = column_types_of_first_group;
    } else {
      groupid = groupb;
      partitionid = i - m_num_partitions;
      column_names = column_names_of_second_group;
      column_types = column_types_of_second_group;
    }

    std::vector<flexible_type> old_vids = get_vertex_ids(partitionid, groupid);
    google::sparse_hash_set<flexible_type, std::hash<flexible_type>>
        old_vid_set(std::make_move_iterator(old_vids.begin()),
                    std::make_move_iterator(old_vids.end()));

    sarray<flexible_type>& raw_id_sarray = unique_vertex_ids[i];
    sarray<flexible_type> new_raw_id_sarray;
    size_t new_vertices_cnt = 0;

    if (old_vids.empty()) {
      new_raw_id_sarray = raw_id_sarray;
    } else {
      new_raw_id_sarray.open_for_write(1);
      new_raw_id_sarray.set_type(m_vid_type);
      turi::copy_if(raw_id_sarray,
                        new_raw_id_sarray,
                        [&](const flexible_type& id) {
                          return old_vid_set.count(id) == 0;
                        });
      new_raw_id_sarray.close();
    }
    new_vertices_cnt = new_raw_id_sarray.size();

    sframe new_vertices;
    vertices_added += new_vertices_cnt;
    new_vertices = new_vertices.add_column(std::make_shared<sarray<flexible_type>>(new_raw_id_sarray),
                                           VID_COLUMN_NAME);
    logstream(LOG_INFO) << "Finish writing new vertices in partition " << partitionid
                        << " in " << timer.current_time() << " secs" << std::endl;

    sframe& old_vertices = vertex_partition(partitionid, groupid);
    ASSERT_TRUE(union_columns(old_vertices, new_vertices));
    old_vertices = old_vertices.append(new_vertices);

    // debug print
    // std::cerr << "New vertices in partition " << i << ":\n";
    // new_vertices.debug_print();
  // }
  });

  logstream(LOG_EMPH) << "Done phase 2 in " << local_timer.current_time() << " secs" << std::endl;

  /**************************************************************************/
  /*                                                                        */
  /*                                 Step 3                                 */
  /*                                                                        */
  /**************************************************************************/
  local_timer.start();
  logstream(LOG_EMPH) << "Rename id columns " << std::endl;

  std::unordered_map<std::pair<size_t, size_t>,
                     std::shared_ptr<vid_hash_map_type>> vid_hash_map_cache;

  /**
   * Preamble function to load the vid to row id lookup table in memory for the
   * next batch of coordinates.
   */
  std::function<void(std::vector<std::pair<size_t, size_t>>)>
    load_vid_hash_map_cache = [&](std::vector<std::pair<size_t, size_t>> coordinates) {
      std::unordered_set<std::pair<size_t, size_t>> block_to_load;
      for (auto& corrd : coordinates) {
        block_to_load.insert({corrd.first, groupa});
        block_to_load.insert({corrd.second, groupb});
      }
      std::unordered_set<std::pair<size_t, size_t>> block_to_unload;
      for (auto& kv : vid_hash_map_cache) {
        if (block_to_load.count(kv.first)) {
          block_to_load.erase(block_to_load.find(kv.first));
        } else {
          block_to_unload.insert(kv.first);
        }
      }
      for (auto& coord : block_to_unload) {
        vid_hash_map_cache.erase(vid_hash_map_cache.find(coord));
      }
      for (auto& coord : block_to_load) {
        vid_hash_map_cache[coord] = {};
      }
      std::vector<std::pair<size_t, size_t>> block_to_load_vec(
          block_to_load.begin(), block_to_load.end());
      parallel_for(0, block_to_load_vec.size(), [&](size_t i) {
          auto coord = block_to_load_vec[i];
          vid_hash_map_cache[coord] = fetch_vid_hash_map(coord.first, coord.second);
          });

      std::stringstream message_ss;
      message_ss <<  "Processing edge partitions: ";
      for (auto& coord: coordinates) {
        message_ss << "(" << coord.first << " , " << coord.second << ") ";
      }
      logstream(LOG_INFO) << message_ss.str() << std::endl;
      logstream(LOG_INFO) << "Number of vid maps in cache: " << vid_hash_map_cache.size() << std::endl;
    };

  //// Second pass for every edge, we rename the global vertex id
  //// into the rowid in the local vertex partition.
  sgraph_compute::hilbert_blocked_parallel_for(
      m_num_partitions,
      load_vid_hash_map_cache,
      [&](std::pair<size_t, size_t> coordinate) {
        size_t i = coordinate.first;
        size_t j = coordinate.second;
        size_t edge_partition_id = i * m_num_partitions + j;

        std::shared_ptr<vid_hash_map_type>
          vid_lookup_a = vid_hash_map_cache[{i, groupa}];
        std::shared_ptr<vid_hash_map_type>
          vid_lookup_b = vid_hash_map_cache[{j, groupb}];

        auto& new_edges = edge_partitions[edge_partition_id];
        size_t src_column_idx = new_edges.column_index(SRC_COLUMN_NAME);
        size_t dst_column_idx = new_edges.column_index(DST_COLUMN_NAME);
        auto src_column = new_edges.select_column(src_column_idx);
        auto dst_column = new_edges.select_column(dst_column_idx);
        std::shared_ptr<sarray<flexible_type>> new_src_column
          = std::make_shared<sarray<flexible_type>>();
        std::shared_ptr<sarray<flexible_type>> new_dst_column
          = std::make_shared<sarray<flexible_type>>();
        new_src_column->open_for_write(src_column->num_segments());
        new_src_column->set_type(flex_type_enum::INTEGER);
        transform(*src_column, *new_src_column,
            [&](const flexible_type& val) {
              return vid_lookup_a->find(val)->second;
            });
        new_src_column->close();
        new_dst_column->open_for_write(dst_column->num_segments());
        new_dst_column->set_type(flex_type_enum::INTEGER);
        transform(*dst_column, *new_dst_column,
            [&](const flexible_type& val) {
              return vid_lookup_b->find(val)->second;
            });
        new_dst_column->close();

        sframe normalized_edges({new_src_column, new_dst_column},
                                {SRC_COLUMN_NAME, DST_COLUMN_NAME});
        for (auto& col : new_edges.column_names()) {
          if (col != SRC_COLUMN_NAME && col != DST_COLUMN_NAME) {
            auto data_col = new_edges.select_column(new_edges.column_index(col));
            normalized_edges = normalized_edges.add_column(data_col, col);
          }
        }

        // commit the new edge block
        sframe& old_edges = edge_partition(i, j, groupa, groupb);
        ASSERT_TRUE(union_columns(old_edges, normalized_edges));

        size_t prev_size = old_edges.num_rows();
        old_edges = old_edges.append(normalized_edges);
        edges_added += (old_edges.num_rows() - prev_size);
      });

  logstream(LOG_EMPH) << "Done in " << local_timer.current_time() << " secs" << std::endl;

  logstream(LOG_EMPH) << "Finish committing edge in "
                       << global_timer.current_time() << " secs" << std::endl;

  m_num_edges += edges_added;
  m_num_vertices += vertices_added;
}

bool sgraph::copy_vertex_field(const std::string& field,
                               const std::string& new_field,
                               size_t group) {
  DASSERT_LT(group, m_num_groups);
  std::vector<sframe>& vdata = vertex_group(group);
  ASSERT_TRUE(vdata[0].contains_column(field));
  // ASSERT_FALSE(vdata[0].contains_column(new_field));
  if (vdata[0].contains_column(new_field)) {
    for (auto& sf : vdata) {
      std::shared_ptr<sarray<flexible_type>> clone_column(sf.select_column(field)->clone());
      sf = sf.replace_column(clone_column, new_field);
    }
  } else {
    for (auto& sf : vdata) {
      std::shared_ptr<sarray<flexible_type>> clone_column(sf.select_column(field)->clone());
      sf = sf.add_column(clone_column, new_field);
    }
  }
  return true;
}

bool sgraph::copy_edge_field(const std::string& field,
                             const std::string& new_field,
                             size_t groupa, size_t groupb) {
  DASSERT_LT(groupa, m_num_groups);
  DASSERT_LT(groupb, m_num_groups);
  std::vector<sframe>& edata = edge_group(groupa, groupb);
  ASSERT_TRUE(edata[0].contains_column(field));
  // ASSERT_FALSE(edata[0].contains_column(new_field));
  if (edata[0].contains_column(new_field)) {
    for (auto& sf : edata) {
      std::shared_ptr<sarray<flexible_type>> clone_column(sf.select_column(field)->clone());
      sf = sf.replace_column(clone_column, new_field);
    }
  } else {
    for (auto& sf : edata) {
      std::shared_ptr<sarray<flexible_type>> clone_column(sf.select_column(field)->clone());
      sf = sf.add_column(clone_column, new_field);
    }
  }
  return true;
}

bool sgraph::remove_vertex_field(const std::string& field, size_t group) {
  DASSERT_LT(group, m_num_groups);
  std::vector<sframe>& vdata = vertex_group(group);
  ASSERT_TRUE(vdata[0].contains_column(field));
  for (auto& sf : vdata) {
    sf = sf.remove_column(sf.column_index(field));
  }
  return true;
}

bool sgraph::remove_edge_field(const std::string& field, size_t groupa, size_t groupb) {
  DASSERT_LT(groupa, m_num_groups);
  DASSERT_LT(groupb, m_num_groups);
  std::vector<sframe>& edata = edge_group(groupa, groupb);
  ASSERT_TRUE(edata[0].contains_column(field));
  for (auto& sf : edata) {
    sf = sf.remove_column(sf.column_index(field));
  }
  return true;
}

bool sgraph::init_vertex_field(const std::string& field, const flexible_type& init_value, size_t group) {
  DASSERT_LT(group, m_num_groups);
  std::vector<sframe>& vdata = vertex_group(group);
  if (!vdata[0].contains_column(field)) {
    for (auto& sf : vdata) {
      std::shared_ptr<sarray<flexible_type>> sa =
        std::make_shared<sarray<flexible_type>>(init_value, sf.size());
      sf = sf.add_column(sa, field);
    }
  } else {
    for (auto& sf : vdata) {
      std::shared_ptr<sarray<flexible_type>> sa =
        std::make_shared<sarray<flexible_type>>(init_value, sf.size());
      sf = sf.replace_column(sa, field);
    }
  }
  return true;
}

bool sgraph::init_edge_field(const std::string& field, const flexible_type& init_value, size_t groupa, size_t groupb) {
  DASSERT_LT(groupa, m_num_groups);
  DASSERT_LT(groupb, m_num_groups);
  std::vector<sframe>& edata = edge_group(groupa, groupb);
  if (!edata[0].contains_column(field)) {
    for (auto& sf : edata) {
      std::shared_ptr<sarray<flexible_type>> sa =
        std::make_shared<sarray<flexible_type>>(init_value, sf.size());
      sf = sf.add_column(sa, field);
    }
  } else {
    for (auto& sf : edata) {
      std::shared_ptr<sarray<flexible_type>> sa =
        std::make_shared<sarray<flexible_type>>(init_value, sf.size());
      sf = sf.replace_column(sa, field);
    }
  }
  return true;
}

bool sgraph::select_vertex_fields(const std::vector<std::string>& fields, size_t group) {
  DASSERT_TRUE(std::set<std::string>(fields.begin(), fields.end()).count(VID_COLUMN_NAME));
  auto& sframe_vec = vertex_group(group);
  for (auto& sf: sframe_vec) {
    sf = sf.select_columns(fields);
  }
  return true;
}

bool sgraph::select_edge_fields(const std::vector<std::string>& fields, size_t groupa, size_t groupb) {
  DASSERT_TRUE(std::set<std::string>(fields.begin(), fields.end()).count(SRC_COLUMN_NAME));
  DASSERT_TRUE(std::set<std::string>(fields.begin(), fields.end()).count(DST_COLUMN_NAME));
  auto& sframe_vec = edge_group(groupa, groupb);
  for (auto& sf: sframe_vec) {
    sf = sf.select_columns(fields);
  }
  return true;
}

bool sgraph::clear() {
  // clear EVERYTHING!!
  m_vertex_group_names.clear();
  m_vertex_groups.clear();
  m_edge_groups.clear();

  // Reinitialize with the given number of partitions
  m_num_partitions = 0;
  m_num_groups = 0;
  m_num_vertices = 0;
  m_num_edges = 0;
  m_vid_type = flex_type_enum::UNDEFINED;
  return true;
}


/**
 * Helper function to segment an sarray into K sarrays using the segment layout. segment_lengths
 * must sum up to the same length as the original array.
 */
std::vector<std::shared_ptr<sarray<flexible_type>>> segment_sarray(std::shared_ptr<sarray<flexible_type>> sa, const std::vector<size_t>& segment_lengths) {
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret;
  bool is_empty = std::all_of(segment_lengths.begin(), segment_lengths.end(), [](size_t i) { return i == 0; });
  for (size_t i = 0; i < segment_lengths.size(); ++i) {
    ret.push_back(std::make_shared<sarray<flexible_type>>());
    ret[i]->open_for_write(1);
    ret[i]->set_type(sa->get_type());
  }
  if (!is_empty) {
    auto reader = sa->get_reader(segment_lengths);
    parallel_for(0, segment_lengths.size(), [&](size_t i) {
      auto out = ret[i]->get_output_iterator(0);
      auto begin = reader->begin(i);
      auto end = reader->end(i);
      while (begin != end) {
        *out = *begin;
        begin++;
        out++;
      }
    });
  }
  for (auto& sa : ret) sa->close();
  return ret;
}

void sgraph::add_vertex_field(std::shared_ptr<sarray<flexible_type>> data, std::string field) {
  std::vector<size_t> segment_lengths;
  for (const auto& sf: vertex_group()) {
    segment_lengths.push_back(sf.size());
  }
  add_vertex_field(segment_sarray(data, segment_lengths), field);
}

void sgraph::add_edge_field(std::shared_ptr<sarray<flexible_type>> data, std::string field) {
  std::vector<size_t> segment_lengths;
  for (const auto& sf: edge_group()) {
    segment_lengths.push_back(sf.size());
  }
  add_edge_field(segment_sarray(data, segment_lengths), field);
}

void sgraph::swap_vertex_fields(const std::string& field1, const std::string& field2) {
  size_t field1_id = get_vertex_field_id(field1);
  size_t field2_id = get_vertex_field_id(field2);
  for (auto& sf: vertex_group()) {
    sf = sf.swap_columns(field1_id, field2_id);
  }
}

void sgraph::swap_edge_fields(const std::string& field1, const std::string& field2) {
  size_t field1_id = get_edge_field_id(field1);
  size_t field2_id = get_edge_field_id(field2);
  for (auto& sf: edge_group()) {
    sf = sf.swap_columns(field1_id, field2_id);
  }
}

void sgraph::rename_vertex_fields(const std::vector<std::string>& oldnames,
    const std::vector<std::string>& newnames) {
  std::vector<size_t> field_ids;
  for (auto& name: oldnames) {
    field_ids.push_back(get_vertex_field_id(name));
  }
  for (auto& sf : vertex_group()) {
    for (size_t i = 0; i < oldnames.size(); ++i) {
      sf.set_column_name(field_ids[i], newnames[i]);
    }
  }
}

void sgraph::rename_edge_fields(const std::vector<std::string>& oldnames,
    const std::vector<std::string>& newnames) {
  std::vector<size_t> field_ids;
  for (auto& name: oldnames) {
    field_ids.push_back(get_edge_field_id(name));
  }
  for (auto& sf : edge_group()) {
    for (size_t i = 0; i < oldnames.size(); ++i) {
      sf.set_column_name(field_ids[i], newnames[i]);
    }
  }
}

bool sgraph::replace_vertex_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                                  std::string column_name,
                                  size_t groupid) {
  auto vfields = get_vertex_fields();
  if (std::count(vfields.begin(), vfields.end(), column_name) == 0) {
    logstream(LOG_ERROR) << "Vertex field not found." << std::endl;
    return false;
  }
  auto& vgroups = vertex_group(groupid);
  if (vgroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }
  for (size_t i = 0; i < vgroups.size(); ++i) {
    vgroups[i] = vgroups[i].replace_column(column[i], column_name);
  }
  return true;
}


bool sgraph::replace_edge_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                                std::string column_name,
                                size_t groupa,
                                size_t groupb) {
  auto efields = get_edge_fields();
  if (std::count(efields.begin(), efields.end(), column_name) == 0) {
    logstream(LOG_ERROR) << "Edge field not found." << std::endl;
    return false;
  }
  auto& egroups = edge_group(groupa, groupb);
  if (egroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }
  for (size_t i = 0; i < egroups.size(); ++i) {
    egroups[i] = egroups[i].replace_column(column[i], column_name);
  }
  return true;
}


bool sgraph::add_vertex_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                              std::string column_name,
                              size_t groupid) {
  auto vfields = get_vertex_fields();
  if (std::count(vfields.begin(), vfields.end(), column_name) != 0) {
    logstream(LOG_ERROR) << "Vertex field already exists." << std::endl;
    return false;
  }
  auto& vgroups = vertex_group(groupid);
  if (vgroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }
  for (size_t i = 0; i < vgroups.size(); ++i) {
    vgroups[i] = vgroups[i].add_column(column[i], column_name);
  }
  return true;
}

bool sgraph::add_edge_field(const std::vector<std::shared_ptr<sarray<flexible_type>>>& column,
                            std::string column_name,
                            size_t groupa,
                            size_t groupb) {
  auto efields = get_edge_fields();
  if (std::count(efields.begin(), efields.end(), column_name) != 0) {
    logstream(LOG_ERROR) << "Edge field already exists." << std::endl;
    return false;
  }
  auto& egroups = edge_group(groupa, groupb);
  if (egroups.size() != column.size()) {
    logstream(LOG_ERROR) << "Partition Size Mismatch." << std::endl;
    return false;
  }
  for (size_t i = 0; i < egroups.size(); ++i) {
    egroups[i] = egroups[i].add_column(column[i], column_name);
  }
  return true;
}



/**
 * Extracts the data for a particular field of a group of vertices.
 * The column must exist. Assertion failure otherwise.
 */
std::vector<std::shared_ptr<sarray<flexible_type>>>
sgraph::fetch_vertex_data_field(std::string column_name, size_t groupid) const {
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret;
  auto& vgroups = vertex_group(groupid);
  for (size_t i = 0; i < vgroups.size(); ++i) {
    ret.push_back(vgroups[i].select_column(column_name));
  }
  return ret;
}

/**
 * Same as \ref fetch_vertex_data_field, but store all values in memory
 * and return std::vector<std::vector<flexible_type>>
 * The column must exist. Assertion failure otherwise.
 */
std::vector<std::vector<flexible_type>>
sgraph::fetch_vertex_data_field_in_memory(std::string column_name, size_t groupid) const {
  std::vector<std::vector<flexible_type>> ret;
  auto& vgroups = vertex_group(groupid);
  for (size_t i = 0; i < vgroups.size(); ++i) {
    auto sa = vgroups[i].select_column(column_name);
    std::vector<flexible_type> buffer;
    sa->get_reader()->read_rows(0, sa->size(), buffer);
    ret.push_back(std::move(buffer));
  }
  return ret;
}



/**
 * Extracts the data for a particular field of a group of edges.
 * The column must exist. Assertion failure otherwise.
 */
std::vector<std::shared_ptr<sarray<flexible_type>>>
sgraph::fetch_edge_data_field(std::string column_name, size_t groupa, size_t groupb) const {
  std::vector<std::shared_ptr<sarray<flexible_type>>> ret;
  auto& egroups = edge_group(groupa, groupb);
  for (size_t i = 0; i < egroups.size(); ++i) {
    ret.push_back(egroups[i].select_column(column_name));
  }
  return ret;
}


/**
 * Extracts the data for a particular field of a group of vertices.
 * The column must exist. Assertion failure otherwise.
 */
size_t sgraph::get_vertex_field_id(std::string column_name, size_t groupid) const {
  const auto& vgroups = vertex_group(groupid);
  return vgroups[0].column_index(column_name);
}


/**
 * Extracts the data for a particular field of a group of edges.
 * The column must exist. Assertion failure otherwise.
 */
size_t sgraph::get_edge_field_id(std::string column_name, size_t groupa, size_t groupb) {
  auto& egroups = edge_group(groupa, groupb);
  return egroups[0].column_index(column_name);
}


/**************************************************************************/
/*                                                                        */
/*                             Serialization                              */
/*                                                                        */
/**************************************************************************/

void parallel_save_sframes(const std::vector<sframe>& sf_vec,
                           oarchive& oarc,
                           bool save_reference) {
  std::vector<std::string> prefixes;
  for (size_t i = 0; i < sf_vec.size(); ++i) {
    prefixes.push_back(oarc.dir->get_next_write_prefix());
  }

  parallel_for(0, sf_vec.size(), [&](size_t i) {
    std::string name = prefixes[i] + ".frame_idx";
    if (save_reference) {
      sframe_save_weak_reference(sf_vec[i], name);
    } else {
      sf_vec[i].save(name);
    }
  });
}

/**
 * \Internal
 *
 * Save to a directory oarchive.
 */
void sgraph::save(oarchive& oarc) const {
  oarc << m_num_partitions << m_num_groups
       << m_num_vertices << m_num_edges << m_vid_type
       << m_vertex_group_names;
  bool save_reference = false;
  for (const auto& vgroup : m_vertex_groups) {
    // This relies on the serialization format of vector,
    // otherwise old will not load.
    oarc << vgroup.size();
    parallel_save_sframes(vgroup, oarc, save_reference);
  }
  for (const auto& kv : m_edge_groups) {
    oarc << kv.first;
    oarc << kv.second.size();
    parallel_save_sframes(kv.second, oarc, save_reference);
  }
}

void sgraph::save_reference(oarchive& oarc) const {
  ASSERT_TRUE(oarc.dir != NULL);
  oarc << m_num_partitions << m_num_groups
       << m_num_vertices << m_num_edges << m_vid_type
       << m_vertex_group_names;
  bool save_reference = true;
  for (const auto& vgroup : m_vertex_groups) {
    // This relies on the serialization format of vector,
    // otherwise it will not load.
    oarc << vgroup.size();
    parallel_save_sframes(vgroup, oarc, save_reference);
  }
  for (const auto& kv : m_edge_groups) {
    oarc << kv.first;
    oarc << kv.second.size();
    parallel_save_sframes(kv.second, oarc, save_reference);
  }
}

/**
 * \Internal
 *
 * Load from a directory oarchive.
 */
void sgraph::load(iarchive& iarc) {
  clear();
  iarc >> m_num_partitions >> m_num_groups
       >> m_num_vertices >> m_num_edges >> m_vid_type
       >> m_vertex_group_names;
  for (size_t i = 0; i < m_num_groups; ++i) {
    std::vector<sframe> vgroup;
    iarc >> vgroup;
    m_vertex_groups.push_back(std::move(vgroup));
  }
  for (size_t i = 0; i < m_num_groups; ++i) {
    for (size_t j = 0; j < m_num_groups; ++j) {
      std::pair<size_t, size_t> group_address;
      std::vector<sframe> egroup;
      iarc >> group_address >> egroup;
      m_edge_groups[group_address] = std::move(egroup);
    }
  }
}

/**************************************************************************/
/*                                                                        */
/*                            Helper Function                             */
/*                                                                        */
/**************************************************************************/
std::shared_ptr<sgraph::vid_hash_map_type> sgraph::fetch_vid_hash_map(size_t partition,
                                                                      size_t group) {
  std::shared_ptr<vid_hash_map_type> ret(new vid_hash_map_type);
  auto vid_sarray = vertex_partition(partition, group).select_column(VID_COLUMN_NAME);
  auto reader = vid_sarray->get_reader();
  auto reader_buffer = sarray_reader_buffer<flexible_type>(std::move(reader), 0, vid_sarray->size());
  size_t i = 0;
  while (reader_buffer.has_next()) {
    ret->insert({reader_buffer.next(), i});
    ++i;
  }
  return ret;
}

bool sgraph::reorder_and_add_new_columns(sframe& sf,
                                         const std::vector<std::string>& column_names,
                                         const std::vector<flex_type_enum>& column_types) {
  DASSERT_EQ(column_names.size(), column_types.size());

  // Make sure the input column contains all existing columns.
  std::set<std::string> input_column_name_set(column_names.begin(), column_names.end());
  for (auto& name: sf.column_names()) {
    if (!(input_column_name_set.count(name))) {
      return false;
    }
  }

  std::vector<std::shared_ptr<sarray<flexible_type>>> columns;
  for (size_t i = 0; i < column_names.size(); ++i) {
    const std::string& name = column_names[i];
    flex_type_enum type = column_types[i];
    if (!sf.contains_column(name)) {
      auto dummy_col = std::make_shared<sarray<flexible_type>>(FLEX_UNDEFINED, sf.size(), 1 /* one seg*/, type);
      sf = sf.add_column(dummy_col, name);
    } else {
      if (sf.column_type(sf.column_index(name)) != type) {
        return false;
      }
    }
  }
  sf = sf.select_columns(column_names);
  return true;
}

bool sgraph::union_columns(sframe& left, sframe& right) {
  std::set<std::string> left_names(left.column_names().begin(), left.column_names().end());
  std::set<std::string> right_names(right.column_names().begin(), right.column_names().end());

  // check common columns have the same type.
  for (auto& col: left_names) {
    if (right_names.count(col)) {
      flex_type_enum left_type = left.column_type(left.column_index(col));
      flex_type_enum right_type = right.column_type(right.column_index(col));
      if ((left_type != flex_type_enum::UNDEFINED) && (right_type != flex_type_enum::UNDEFINED)
          && (left_type != right_type)) {
        logstream(LOG_INFO) << "Column type does not match for field : " << col
                            << " " << flex_type_enum_to_name(left_type) << "!="
                            << flex_type_enum_to_name(right_type) << std::endl;
        return false;
      }
    }
  }

  std::vector<std::string> names = left.column_names();
  std::vector<flex_type_enum> types = left.column_types();
  for (size_t i = 0; i < right.num_columns(); ++i) {
    std::string name = right.column_name(i);
    flex_type_enum type = right.column_type(i);
    if (!left_names.count(name)) {
      names.push_back(name);
      types.push_back(type);
    }
  }

  bool ret = reorder_and_add_new_columns(left, names, types) &&
             reorder_and_add_new_columns(right, names, types);

  return ret;
}

void sgraph::fast_validate_add_vertices(const sframe& vertices, size_t group) {
  size_t id_column_idx = vertices.column_index(VID_COLUMN_NAME);
  flex_type_enum vid_type = vertices.column_type(id_column_idx);
  ASSERT_TRUE(vid_type != flex_type_enum::UNDEFINED);
  if (m_vid_type == flex_type_enum::UNDEFINED) {
    bootstrap_vertex_id_type(vid_type);
  } else if (m_vid_type != vid_type) {
    log_and_throw(
        std::string("Input vertex id type does not match existing type: ")
        + flex_type_enum_to_name(m_vid_type)
    );
  }

  std::vector<std::string> current_names = get_vertex_fields(group);
  std::vector<flex_type_enum> current_types = get_vertex_field_types(group);
  std::unordered_map<std::string, flex_type_enum> input_types;
  for (size_t i = 0; i < vertices.num_columns(); ++i) {
    input_types[vertices.column_name(i)] = vertices.column_type(i);
  }

  for (size_t i = 0; i < current_names.size(); ++i) {
    const std::string& key = current_names[i];
    flex_type_enum expected_type = current_types[i];
    if (input_types.count(key) && input_types[key] != expected_type) {
      log_and_throw(
          std::string("Input vertex data [column=") + key
          + "] type does not match existing type: "
          + flex_type_enum_to_name(expected_type)
      );
    }
  }
}

void sgraph::fast_validate_add_edges(const sframe& edges, size_t groupa, size_t groupb) {
  size_t src_column_idx = edges.column_index(SRC_COLUMN_NAME);
  size_t dst_column_idx = edges.column_index(DST_COLUMN_NAME);
  flex_type_enum src_vid_type = edges.column_type(src_column_idx);
  flex_type_enum dst_vid_type = edges.column_type(dst_column_idx);
  if (src_vid_type != dst_vid_type) {
    log_and_throw("Input edge data source and target column have different types");
  }
  ASSERT_TRUE(src_vid_type != flex_type_enum::UNDEFINED);
  if (m_vid_type == flex_type_enum::UNDEFINED) {
    bootstrap_vertex_id_type(src_vid_type);
  } else if (m_vid_type != src_vid_type) {
    log_and_throw(
      std::string("Input edge data source id type does not match existing type: ")
      + flex_type_enum_to_name(m_vid_type)
    );
  }

  std::vector<std::string> current_names = get_edge_fields(groupa, groupb);
  std::vector<flex_type_enum> current_types = get_edge_field_types(groupa, groupb);

  std::unordered_map<std::string, flex_type_enum> input_types;
  for (size_t i = 0; i < edges.num_columns(); ++i) {
    input_types[edges.column_name(i)] = edges.column_type(i);
  }

  for (size_t i = 0; i < current_names.size(); ++i) {
    const std::string& key = current_names[i];
    flex_type_enum expected_type;
    if (key == SRC_COLUMN_NAME || key == DST_COLUMN_NAME) {
      expected_type = m_vid_type;
    } else {
      expected_type = current_types[i];
    }
    if (input_types.count(key) && input_types[key] != expected_type) {
      log_and_throw(
          std::string("Input edge data [column=") + key
          + "] type does not match existing type: "
          + flex_type_enum_to_name(expected_type)
      );
    }
  }
}
} // namespace turi
