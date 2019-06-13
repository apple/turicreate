/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/graph_analytics/graph_coloring.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/simple_model.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <core/export.hpp>

namespace turi {
namespace graph_coloring {

const std::string COLOR_COLUMN = "color_id";

/**
 * Helper function to add a value to flex_vec
 * and keep all values in the flex_vec unique.
 */
void set_insert(flexible_type& set, const flexible_type& value) {
  flex_vec& vec = set.mutable_get<flex_vec>();
  bool is_unique = true;
  for (auto& v : vec) {
    if (v == (double)(value)) {
      is_unique = false;
      break;
    }
  }
  if (is_unique) {
    vec.push_back((double)(value));
  }
}

/**
 * Returns the min value that is not in the set.
 * Assuming the vector is SORTED.
 **/
int find_min_value_not_in_set(const flex_vec& vec) {
  int min_value = 0;
  if (vec.size() ==0 ) return 0;
  size_t cnt = 0;
  int current = (int)(vec[0]);
  while (true) {
    if (min_value < current) {
      return min_value;
    } else if (min_value == current) {
      ++min_value;
      ++cnt;
      if (cnt == vec.size()) break;
      current = (int)vec[cnt];
    } else {
      // impossible condition
      ASSERT_TRUE(false);
    }
  }
  return min_value;
}

/**
 * validate that the graph has a valid coloring.
 */
void validate_coloring(sgraph& g) {
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;
  sgraph_compute::sgraph_engine<flexible_type> ga;
  const size_t id_idx = g.get_vertex_field_id(sgraph::VID_COLUMN_NAME);
  const size_t color_idx = g.get_vertex_field_id(COLOR_COLUMN);

  ga.gather(g,
            [id_idx, color_idx](
               const graph_data_type& center,
               const graph_data_type& edge,
               const graph_data_type& other,
               edge_direction edgedir,
               flexible_type& combiner) {
                 if (center[color_idx] == other[color_idx]) {
                   throw(std::string("Color collide for ")
                         + (std::string)other[id_idx] + " and "
                         + (std::string)center[id_idx]);
                 }
               },
               flexible_type(0),
               edge_direction::ANY_EDGE);
}

/**
 * Compute a coloring for g so that neighboring vertices have different colors.
 * Add a COLOR_COLUMN to vertex data containing the color id for each vertex.
 * Return the number of unique colors in the graph.
 */
size_t compute_coloring(sgraph& g) {
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;
  sgraph_compute::sgraph_engine<flexible_type> ga;

  g.init_vertex_field(COLOR_COLUMN, 0);
  flex_vec empty_gather{};
  std::atomic<int64_t> num_changed;
  const size_t id_idx = g.get_vertex_field_id(sgraph::VID_COLUMN_NAME);
  const size_t color_idx = g.get_vertex_field_id(COLOR_COLUMN);

  table_printer table({{"Number of vertices updated", 0}});
  table.print_header();
  while(true) {
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }
    num_changed = 0;
    std::vector<std::shared_ptr<sarray<flexible_type>>> ret =
        ga.gather(g,
                  [id_idx, color_idx](
                     const graph_data_type& center,
                     const graph_data_type& edge,
                     const graph_data_type& other,
                     edge_direction edgedir,
                     flexible_type& combiner) {
                       if (center[id_idx].hash() > other[id_idx].hash()) {
                         set_insert(combiner, other[color_idx]);
                       }
                     },
                     flexible_type(empty_gather),
                     edge_direction::ANY_EDGE);

    // compute the change in coloring
    auto apply_result = sgraph_compute::vertex_apply(
        g,
        COLOR_COLUMN, // first argument in lambda is the color column in g
        ret,          // second argument in lambda is from here.
        flex_type_enum::FLOAT,
        [&](const flexible_type& x, flexible_type& y) {
          flex_vec& vec = y.mutable_get<flex_vec>();
          std::sort(vec.begin(), vec.end());
          int new_color = find_min_value_not_in_set(vec);
          if (new_color != (int)(x)) {
            ++num_changed;
          }
          return new_color;
        });

    table.print_row(num_changed);
    g.replace_vertex_field(apply_result, COLOR_COLUMN);
    if (num_changed == 0) {
      break;
    }
  }

  table.print_footer();
  // Compute the unique colors;
  auto colors = g.fetch_vertex_data_field(COLOR_COLUMN);
  std::vector<std::unordered_set<int>> unique_colors(colors.size());
  parallel_for (0, colors.size(), [&](size_t idx) {
    sarray_reader_buffer<flexible_type> reader(colors[idx]->get_reader(),
                                               0, colors[idx]->size());
    auto& set = unique_colors[idx];
    while (reader.has_next()) {
      set.insert((int)reader.next());
    }
  });
  auto& set_combine = unique_colors[0];
  for (size_t i = 1; i < unique_colors.size(); ++i) {
    const auto& other = unique_colors[i];
    set_combine.insert(other.begin(), other.end());
  }
  return set_combine.size();
}

/**************************************************************************/
/*                                                                        */
/*                             Main Function                              */
/*                                                                        */
/**************************************************************************/
variant_map_type  exec(variant_map_type& params) {
  timer mytimer;
  std::shared_ptr<unity_sgraph> source_graph =
      safe_varmap_get<std::shared_ptr<unity_sgraph>>(params, "graph");
  ASSERT_TRUE(source_graph != NULL);
  sgraph& source_sgraph = source_graph->get_graph();
  // Do not support vertex groups yet.
  ASSERT_EQ(source_sgraph.get_num_groups(), 1);

  // Setup the graph we are going to work on. Copying sgraph is cheap.
  sgraph g(source_sgraph);
  g.select_vertex_fields({sgraph::VID_COLUMN_NAME});
  g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});

  size_t num_colors = compute_coloring(g);

#ifndef NDEBUG
  validate_coloring(g);
#endif

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type model_params;
  model_params["graph"] = to_variant(result_graph);
  model_params["color_id"] = to_variant(result_graph->get_vertices());
  model_params["training_time"] = mytimer.current_time();
  model_params["num_colors"] = num_colors;

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));
  return response;
}

static const variant_map_type MODEL_FIELDS{
};

variant_map_type get_model_fields(variant_map_type& params) {
  return {
    {"graph", "A new SGraph with the color id as a vertex property"},
    {"component_id", "An SFrame with each vertex's component id"},
    {"component_size", "An SFrame with the size of each component"},
    {"training_time", "Total training time of the model"}
  };
  variant_map_type response;
  response = MODEL_FIELDS;
  return response;
}

/**************************************************************************/
/*                                                                        */
/*                          Toolkit Registration                          */
/*                                                                        */
/**************************************************************************/


BEGIN_FUNCTION_REGISTRATION
REGISTER_NAMED_FUNCTION("create", exec, "params");
REGISTER_FUNCTION(get_model_fields, "params");
END_FUNCTION_REGISTRATION

} // end of namespace graph_coloring
} // end of namespace turi
