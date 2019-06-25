/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/recsys/user_item_graph.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <algorithm>
#include <core/parallel/pthread_tools.hpp>
#include <perf/memory_info.hpp>

namespace turi { namespace recsys {

typedef std::pair<size_t, double> IV;
const std::string VERTEX_ID_COLUMN = "vertex_id";
const std::string VERTEX_DATA_COLUMN = "vertex_data";


/**
 *  Build a user-item bipartite graph and initialize node data.
 *  Each node has two columns: vertex_id and vertex_data
 *
 *  vertex_id is the unique id for each node.
 *  For item nodes, vertex ids are just original item_ids.
 *  For user nodes, vertex_ids are re-indexed from user_id -> (user_id + num_items)
 *
 *  For each user node, vertex_data includes a flex_dict, which contains all the
 *  (item, rating) pairs.
 *  For each item node, vertex_data includes a empty flex_dict.
 *
 *
 *  TODO: make it a general function of setting up a bipartite graph
 */
void add_vertex_data(sgraph &g,
                    const std::shared_ptr<sarray<flex_dict>>& user_item_lists,
                    const size_t num_users,
                    const size_t num_items) {

  DASSERT_EQ(user_item_lists->size(), num_users);

  const size_t num_vertices = num_users + num_items ;
  const size_t num_segments = user_item_lists->num_segments();

  // initialize a vector of ids from 0 to num_vertices - 1.
  std::vector<size_t> ids(num_vertices);
  std::iota(ids.begin(), ids.end(), 0);
  // copy to sarray
  std::shared_ptr<sarray<flexible_type>> vertex_ids(new sarray<flexible_type>());
  vertex_ids->open_for_write(num_segments);
  vertex_ids->set_type(flex_type_enum::INTEGER);
  turi::copy(ids.begin(), ids.end(), *vertex_ids);
  vertex_ids->close();
  DASSERT_TRUE(vertex_ids->size() == num_vertices);

  // copy user_data
  std::shared_ptr<sarray<flexible_type>> user_data(new sarray<flexible_type>());;
  user_data->open_for_write(num_segments);
  user_data->set_type(flex_type_enum::DICT);

  auto reader = user_item_lists->get_reader(num_segments);

  parallel_for(0, num_segments, [&](size_t i) {
        auto rbegin = reader->begin(i);
        auto rend = reader->end(i);
        auto outiter = user_data->get_output_iterator(i);
        for (auto iter = rbegin; iter != rend; iter++) {
          *outiter = *iter;
          ++outiter;
        }
    });
  user_data->close();

  std::shared_ptr<sarray<flexible_type>> item_data(new sarray<flexible_type>(flex_dict(), num_items));

  // TODO: find a better way of initializing vertex_data
  std::shared_ptr<sarray<flexible_type>> vertex_data(new sarray<flexible_type>(item_data->append(*user_data)));

  DASSERT_TRUE(vertex_data->size() == num_vertices);

  sframe vertex_sf({vertex_ids, vertex_data}, {VERTEX_ID_COLUMN, VERTEX_DATA_COLUMN});
  g.add_vertices(vertex_sf, VERTEX_ID_COLUMN);

}


/**
 *  Add the edges to the graph.
 *  Only add item -> user edges.
 *  Edges are associated with ratings.
 */
void add_edges(sgraph& g, const std::shared_ptr<sarray<flex_dict>>& user_item_lists) {


  const size_t num_segments = user_item_lists->num_segments();

  sframe edges_sf;
  std::vector<std::string> column_names = {"user_id", "item_id", "rating"};
  std::vector<flex_type_enum> column_types = {flex_type_enum::INTEGER,
                                            flex_type_enum::INTEGER,
                                            flex_type_enum::FLOAT};
  edges_sf.open_for_write(column_names, column_types, "", num_segments);

  size_t num_users = user_item_lists->size();
  size_t num_items = g.num_vertices() - num_users;
  auto reader = user_item_lists->get_reader(num_segments);

  std::vector<size_t> user_segment_begin(num_segments);

  size_t users_in_segments = 0;
  for (size_t idx = 0; idx < user_segment_begin.size(); idx++) {
    user_segment_begin[idx] = users_in_segments;
    users_in_segments += reader->segment_length(idx);
  }

  DASSERT_EQ(num_users, users_in_segments);

  parallel_for(0, num_segments, [&] (size_t i) {
      auto rbegin = reader->begin(i);
      auto rend = reader->end(i);
      auto outiter = edges_sf.get_output_iterator(i);
      auto user_idx = user_segment_begin[i];

      // re-index user_id, and write it to the sframe
      for (auto iter = rbegin; iter != rend; ++iter) {
        for (auto& ivpair : *iter) {
          *outiter = std::vector<flexible_type>({user_idx + num_items,
                                                  ivpair.first,
                                                  ivpair.second});
          ++outiter;
        }
        ++user_idx;
      }

    });

  edges_sf.close();

  // only create item-> user edges
  g.add_edges(edges_sf, "item_id", "user_id");

}

/**
 *  Set up the bipartite graph using user_item_lists
 */
void make_user_item_graph(const v2::ml_data& data,
                          const std::shared_ptr<sarray<flex_dict>>& user_item_lists,
                          sgraph& g) {

  DASSERT_GE(data.metadata()->num_columns(), 2);
  const size_t num_users = data.metadata()->column_size(0);
  const size_t num_items = data.metadata()->column_size(1);


  memory_info::log_usage();
  add_vertex_data(g, user_item_lists, num_users, num_items);

  memory_info::log_usage();
  add_edges(g, user_item_lists);

}


}}
