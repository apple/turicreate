/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sframe/sframe.hpp>
#include <sgraph/sgraph.hpp>

namespace turi {
namespace graph_testing_utils {

/**
 * Helper datastructure for creating sframe column.
 */
struct column {
  std::string name;
  flex_type_enum type;
  std::vector<flexible_type> data;
};

/**
 * Create an sframe with columns.
 */
sframe create_sframe(const std::vector<column>& columns) {
  sframe ret;
  ret.open_for_write({}, {});
  ret.close();
  for (auto& col : columns) {
    std::shared_ptr<sarray<flexible_type>> sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    sa->set_type(col.type);
    turi::copy(col.data.begin(), col.data.end(), *sa);
    sa->close();
    ret = ret.add_column(sa, col.name);
  }
  return ret;
}

/**
 * Create the zachary karate dataset
 */
sgraph create_zachary_dataset() {
    // Zachary's Karate Club dataset. 
    // Initial labels determined manually with the following labels:
    // label 0: vertices 1, 3
    // label 1: vertices  4, 16
    // label 2: vertices 15, 25, 32
    // Expected labels created by running the original label propagation and
    // visually inspecting that predicted labels match true labels (and
    // predictions are reasonable where true labels do not exist).
    std::vector<flexible_type> ids = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
      13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
      31, 32, 33};
    std::vector<flexible_type> labels = std::vector<flexible_type>(ids.size(), FLEX_UNDEFINED);
    labels[1] = 0;
    labels[3] = 0;
    labels[4] = 1;
    labels[16] = 1;
    labels[15] = 2;
    labels[25] = 2;
    labels[32] = 2;
    std::vector<flexible_type> expected = {0, 0, 0, 0, 1, 1, 1, 0, 2, 2, 1, 0,
      0, 0, 2, 2, 1, 0, 2, 0, 2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
    std::vector<flexible_type> srcs = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4,
      5, 5, 5, 6, 8, 8, 8, 9, 13, 14, 14, 15, 15, 18, 18, 19, 20, 20, 22, 22,
      23, 23, 23, 23, 23, 24, 24, 24, 25, 26, 26, 27, 28, 28, 29, 29, 30, 30,
      31, 31, 32};
    std::vector<flexible_type> dsts = {1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12, 13,
      17, 19, 21, 31, 2, 3, 7, 13, 17, 19, 21, 30, 3, 7, 8, 9, 13, 27, 28, 32,
      7, 12, 13, 6, 10, 6, 10, 16, 16, 30, 32, 33, 33, 33, 32, 33, 32, 33, 32,
      33, 33, 32, 33, 32, 33, 25, 27, 29, 32, 33, 25, 27, 31, 31, 29, 33, 33,
      31, 33, 32, 33, 32, 33, 32, 33, 33};
    std::vector<flexible_type> edges = std::vector<flexible_type>(srcs.size(), 1.0);

    size_t num_partitions = 4;
    sgraph g(num_partitions);
    sframe vertex_data = create_sframe({{"id", flex_type_enum::INTEGER, ids}, 
                                        {"label", flex_type_enum::INTEGER, labels}, 
                                        {"expected", flex_type_enum::INTEGER, expected}});
    sframe edge_data = create_sframe({{"src", flex_type_enum::INTEGER, srcs}, 
                                     {"dst", flex_type_enum::INTEGER, dsts}, 
                                     {"data", flex_type_enum::INTEGER, edges}});
    g.add_vertices(vertex_data, "id", 0);
    g.add_edges(edge_data, "src", "dst", 0, 0);
    return g;
}


} // namespace graph_testing_util
} // namespace turi
