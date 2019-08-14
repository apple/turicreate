/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_TEST_UTIL_HPP
#define TURI_SGRAPH_TEST_UTIL_HPP

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>

namespace turi {

struct column {
  std::string name;
  flex_type_enum type;
  std::vector<flexible_type> data;
};

/**
 * Create an sframe with columns.
 */
inline sframe create_sframe(const std::vector<column>& columns) {
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

bool test_frame_equal(sframe left, sframe right, const std::vector<size_t>& key_columns);

/**
 * Create a ring graph.
 */
inline sgraph create_ring_graph(size_t nverts, size_t npartition,
                                bool bidirection=false,
                                bool validate=false) {
  sgraph g(npartition);
  std::vector<flexible_type> ids;
  std::vector<flexible_type> sources;
  std::vector<flexible_type> targets;
  std::vector<flexible_type> vdata;
  std::vector<flexible_type> edata;
  for (size_t i = 0; i < nverts; ++i) {
    ids.push_back(i);
    sources.push_back(i);
    targets.push_back((i + 1) % nverts);;
    vdata.push_back(1.0);
    edata.push_back(std::to_string(i) + std::to_string((i+1)%nverts));
  }
  column source_col = {
    "source",
    flex_type_enum::INTEGER,
    sources
  };
  column target_col = {
    "target",
    flex_type_enum::INTEGER,
    targets
  };
  column edata_col = {
    "edata",
    flex_type_enum::STRING,
    edata
  };
  column vdata_col = {
    "vdata",
    flex_type_enum::FLOAT,
    vdata
  };
  column id_col = {
    "id",
    flex_type_enum::INTEGER,
    ids
  };
  sframe edge_data = create_sframe({source_col, target_col, edata_col});
  sframe vertex_data = create_sframe({id_col, vdata_col});

  // Add one direction
  g.add_edges(edge_data, "source", "target");
  if (bidirection) {
    // Add the other direction
    g.add_edges(edge_data, "target", "source");
  }
  // Add vertex data
  g.add_vertices(create_sframe({id_col}), "id");
  g.add_vertices(vertex_data, "id");

  if (validate) {
    // Check graph
    TS_ASSERT_EQUALS(g.get_num_groups(), 1);
    TS_ASSERT_EQUALS(g.get_num_partitions(), npartition);
    TS_ASSERT_EQUALS(g.num_vertices(), nverts);
    TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
    if (bidirection) {
      TS_ASSERT_EQUALS(g.num_edges(), 2*nverts);
    } else {
      TS_ASSERT_EQUALS(g.num_edges(), nverts);
    }

    // check vertex fields
    std::vector<std::string> expected_vertex_fields {"__id", "vdata"};
    std::vector<flex_type_enum> expected_vertex_field_types {flex_type_enum::INTEGER, flex_type_enum::FLOAT};
    auto vertex_fields = g.get_vertex_fields();
    auto vertex_field_types = g.get_vertex_field_types();
    for (size_t i = 0; i < vertex_fields.size(); ++i) {
      TS_ASSERT_EQUALS(vertex_fields[i], expected_vertex_fields[i]);
      TS_ASSERT_EQUALS(vertex_field_types[i], expected_vertex_field_types[i]);
    }
    // check edge fields
    std::vector<std::string> expected_edge_fields {"__src_id", "__dst_id", "edata"};
    std::vector<flex_type_enum> expected_edge_field_types {sgraph::INTERNAL_ID_TYPE, sgraph::INTERNAL_ID_TYPE, flex_type_enum::STRING};
    auto edge_fields = g.get_edge_fields();
    auto edge_field_types = g.get_edge_field_types();
    for (size_t i = 0; i < edge_fields.size(); ++i) {
      TS_ASSERT_EQUALS(edge_fields[i], expected_edge_fields[i]);
      TS_ASSERT_EQUALS(edge_field_types[i], expected_edge_field_types[i]);
    }

    // check data
    vertex_data.set_column_name(0, "__id");
    TS_ASSERT(test_frame_equal(g.get_vertices(), vertex_data, {0}));
    if (!bidirection) {
      edge_data.set_column_name(0, "__src_id");
      edge_data.set_column_name(1, "__dst_id");
      TS_ASSERT(test_frame_equal(g.get_edges(), edge_data, {0, 1}));
    } else {
      source_col.data.insert(source_col.data.end(), targets.begin(), targets.end());
      target_col.data.insert(target_col.data.end(), sources.begin(), sources.end());
      edata_col.data.insert(edata_col.data.end(), edata.begin(), edata.end());
      sframe edge_data2 = create_sframe({source_col, target_col, edata_col});
      edge_data2.set_column_name(0, "__src_id");
      edge_data2.set_column_name(1, "__dst_id");
      TS_ASSERT(test_frame_equal(g.get_edges(), edge_data2, {0, 1}));
    }
  }
  return g;
}


/**
 * Create star graph.
 */
inline sgraph create_star_graph(size_t nverts, size_t npartition,
                                bool bidirection=false, bool validate=false) {
  sgraph g(npartition);
  std::vector<flexible_type> sources;
  std::vector<flexible_type> targets;
  std::vector<flexible_type> ids;
  std::vector<flexible_type> vdata;
  std::vector<flexible_type> edata;
  for (size_t i = 0; i < nverts; ++i) {
    ids.push_back(i);
    if (i > 0) {
      sources.push_back(i);
      targets.push_back(0);;
      edata.push_back(std::to_string(i) + std::to_string(0));
    }
    vdata.push_back(1.0);
  }
  column source_col = {
    "source",
    flex_type_enum::INTEGER,
    sources
  };
  column target_col = {
    "target",
    flex_type_enum::INTEGER,
    targets
  };
  column edata_col = {
    "edata",
    flex_type_enum::STRING,
    edata 
  };

  column vdata_col = {
    "vdata",
    flex_type_enum::FLOAT,
    vdata
  };

  column id_col = {
    "id",
    flex_type_enum::INTEGER,
    ids
  };
  sframe edge_data = create_sframe({source_col, target_col, edata_col});
  sframe vertex_data = create_sframe({id_col, vdata_col});
  // Add one direction
  g.add_edges(edge_data, "source", "target");
  if (bidirection) {
    g.add_edges(edge_data, "target", "source");
  }
  // Add vertex data
  g.add_vertices(create_sframe({id_col}), "id");
  // Add new vertex data
  g.add_vertices(vertex_data, "id");


  if (validate) {
    // Check graph
    TS_ASSERT_EQUALS(g.get_num_groups(), 1);
    TS_ASSERT_EQUALS(g.get_num_partitions(), npartition);
    TS_ASSERT_EQUALS(g.num_vertices(), nverts);
    TS_ASSERT_EQUALS(g.vertex_id_type(), flex_type_enum::INTEGER);
    if (bidirection) {
      TS_ASSERT_EQUALS(g.num_edges(), 2*(nverts-1));
    } else {
      TS_ASSERT_EQUALS(g.num_edges(), nverts-1);
    }

    // check vertex fields
    std::vector<std::string> expected_vertex_fields {"__id", "vdata"};
    std::vector<flex_type_enum> expected_vertex_field_types {flex_type_enum::INTEGER, flex_type_enum::FLOAT};
    auto vertex_fields = g.get_vertex_fields();
    auto vertex_field_types = g.get_vertex_field_types();
    for (size_t i = 0; i < vertex_fields.size(); ++i) {
      TS_ASSERT_EQUALS(vertex_fields[i], expected_vertex_fields[i]);
      TS_ASSERT_EQUALS(vertex_field_types[i], expected_vertex_field_types[i]);
    }
    // check edge fields
    std::vector<std::string> expected_edge_fields {"__src_id", "__dst_id", "edata"};
    std::vector<flex_type_enum> expected_edge_field_types {sgraph::INTERNAL_ID_TYPE, sgraph::INTERNAL_ID_TYPE, flex_type_enum::STRING};
    auto edge_fields = g.get_edge_fields();
    auto edge_field_types = g.get_edge_field_types();
    for (size_t i = 0; i < edge_fields.size(); ++i) {
      TS_ASSERT_EQUALS(edge_fields[i], expected_edge_fields[i]);
      TS_ASSERT_EQUALS(edge_field_types[i], expected_edge_field_types[i]);
    }

    // check data
    vertex_data.set_column_name(0, "__id");
    TS_ASSERT(test_frame_equal(g.get_vertices(), vertex_data, {0}));
    if (!bidirection) {
      edge_data.set_column_name(0, "__src_id");
      edge_data.set_column_name(1, "__dst_id");
      TS_ASSERT(test_frame_equal(g.get_edges(), edge_data, {0, 1}));
    } else {
      source_col.data.insert(source_col.data.end(), targets.begin(), targets.end());
      target_col.data.insert(target_col.data.end(), sources.begin(), sources.end());
      edata_col.data.insert(edata_col.data.end(), edata.begin(), edata.end());
      sframe edge_data2 = create_sframe({source_col, target_col, edata_col});
      edge_data2.set_column_name(0, "__src_id");
      edge_data2.set_column_name(1, "__dst_id");
      TS_ASSERT(test_frame_equal(g.get_edges(), edge_data2, {0, 1}));
    }
  }

  return g;
}

/**
 * Test whether the two sframe are the same.
 */
bool test_frame_equal(sframe left, sframe right,
                      const std::vector<size_t>& key_columns) {
  if ((left.size() != right.size()) ||
      (left.num_columns() != right.num_columns())) {
    std::cerr << "Size mismatch" << std::endl; 
    return false;
  }
  size_t nrow = left.num_rows();
  size_t ncol = left.num_columns();
  for (size_t i = 0; i < ncol; ++i) {
    if (left.column_name(i) != right.column_name(i)) {
      std::cerr << "Column name mismatch" << std::endl; 
      return false;
    }
    if (left.column_type(i) != right.column_type(i)) {
      std::cerr << "Column type mismatch" << std::endl; 
      return false;
    }
  }

  std::vector<std::vector<flexible_type>> left_data;
  std::vector<std::vector<flexible_type>> right_data;

  left.get_reader()->read_rows(0, left.size(), left_data);
  right.get_reader()->read_rows(0, right.size(), right_data);

  std::function<bool(const std::vector<flexible_type>&,
                     const std::vector<flexible_type>&)>
      comparator = [&] (const std::vector<flexible_type>& a,
                        const std::vector<flexible_type>& b) {
        for (const auto& idx : key_columns) {
          if (a[idx] < b[idx]) {
            return true;
          }
          if (a[idx] > b[idx]) {
            return false;
          }
          // if equal, keep comparing
        }
        return false;
      };

  std::sort(left_data.begin(), left_data.end(), comparator);
  std::sort(right_data.begin(), right_data.end(), comparator);

  for (size_t i = 0; i < nrow; ++i) {
    auto& a = left_data[i];
    auto& b = right_data[i];
    for (size_t j = 0; j < ncol; ++j) {
      if (!(
          (a[j].get_type() == flex_type_enum::UNDEFINED &&
           b[j].get_type() == flex_type_enum::UNDEFINED) ||
           (a[j] == b[j])
           )) {
        std::cerr << "Data (" << i << ", " << j <<  ") mismatch: " 
                  << a[j] << " !=  " << b[j] << std::endl;
        return false;
      }
    }
  }
  return true;
}


}

#endif
