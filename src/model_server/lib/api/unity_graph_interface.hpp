/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GRAPH_INTERFACE_HPP
#define TURI_UNITY_GRAPH_INTERFACE_HPP
#include <memory>
#include <vector>
#include <string>
#include <core/data/flexible_type/flexible_type.hpp>
#include <model_server/lib/options_map.hpp>
#include <model_server/lib/api/unity_sframe_interface.hpp>
#include <core/system/cppipc/magic_macros.hpp>

namespace turi {


#if DOXYGEN_DOCUMENTATION
// Doxygen fake documentation

/**
 * The \ref turi::unity_graph and \ref turi::unity_sgraph_base classes
 * implement a graph object on the server side which is exposed to the
 * client via the cppipc system. The unity_graph is a lazily evaluated, immutable
 * graph datastructure where most operations do not take time, and instead,
 * the graph is only fully constructed when accessed. See
 * \ref turi::unity_graph for detailed documentation on the functions.
 */
class unity_sgraph_base {
    options_map_t summary();
    std::vector<std::string> get_fields();
    std::shared_ptr<unity_sframe_base> get_vertices(const std::vector<flexible_type>&,
                             const options_map_t&);
    std::shared_ptr<unity_sframe_base> get_edges(const std::vector<flexible_type>&
                          const std::vector<flexible_type>&
                          const options_map_t&);
    bool save_graph(std::string)
    bool load_graph(std::string)
    std::shared_ptr<unity_sgraph_base> clone()
    std::shared_ptr<unity_sgraph_base> add_vertices(dataframe_t&, const std::string&)
    std::shared_ptr<unity_sgraph_base> add_vertices(unity_sframe&, const std::string&)
    std::shared_ptr<unity_sgraph_base> add_vertices_from_file(const std::string&, const std::string&, char, bool)
    std::shared_ptr<unity_sgraph_base> add_edges_from_file(const std::string&, const std::string&, const std::string&, char, bool)
    std::shared_ptr<unity_sgraph_base> add_edges(dataframe_t&, const std::string&, const std::string&)
    std::shared_ptr<unity_sgraph_base> select_fields(const std::vector<std::string>&)
    std::shared_ptr<unity_sgraph_base> copy_field(std::string, std::string)
    std::shared_ptr<unity_sgraph_base> delete_field(std::string)
}

#endif

GENERATE_INTERFACE_AND_PROXY(unity_sgraph_base, unity_graph_proxy,
    (options_map_t, summary, )
    (std::vector<std::string>, get_vertex_fields, (size_t))
    (std::vector<std::string>, get_edge_fields, (size_t)(size_t))
    (std::vector<flex_type_enum>, get_vertex_field_types, (size_t))
    (std::vector<flex_type_enum>, get_edge_field_types, (size_t)(size_t))

    (std::shared_ptr<unity_sframe_base>, get_vertices,
        (const std::vector<flexible_type>&)(const options_map_t&)(size_t))
    (std::shared_ptr<unity_sframe_base>, get_edges,
      (const std::vector<flexible_type>&)
      (const std::vector<flexible_type>&)
      (const options_map_t&)(size_t)(size_t))
    // (bool, save_graph_as_json, (std::string))
    (bool, save_graph, (std::string)(std::string))
    (bool, load_graph, (std::string))
    (std::shared_ptr<unity_sgraph_base>, clone, )
    (std::shared_ptr<unity_sgraph_base>, add_vertices, (std::shared_ptr<unity_sframe_base>)(const std::string&)(size_t))
    (std::shared_ptr<unity_sgraph_base>, add_edges, (std::shared_ptr<unity_sframe_base>)(const std::string&)(const std::string&)(size_t)(size_t))

    (std::shared_ptr<unity_sgraph_base>, select_vertex_fields, (const std::vector<std::string>&)(size_t))
    (std::shared_ptr<unity_sgraph_base>, copy_vertex_field, (std::string)(std::string)(size_t))
    (std::shared_ptr<unity_sgraph_base>, add_vertex_field, (std::shared_ptr<unity_sarray_base>)(std::string))
    (std::shared_ptr<unity_sgraph_base>, delete_vertex_field, (std::string)(size_t))
    (std::shared_ptr<unity_sgraph_base>, rename_vertex_fields, (const std::vector<std::string>&)(const std::vector<std::string>&))
    (std::shared_ptr<unity_sgraph_base>, swap_vertex_fields, (const std::string&)(const std::string&))

    (std::shared_ptr<unity_sgraph_base>, select_edge_fields, (const std::vector<std::string>&)(size_t)(size_t))
    (std::shared_ptr<unity_sgraph_base>, add_edge_field, (std::shared_ptr<unity_sarray_base>)(std::string))
    (std::shared_ptr<unity_sgraph_base>, copy_edge_field, (std::string)(std::string)(size_t)(size_t))
    (std::shared_ptr<unity_sgraph_base>, delete_edge_field, (std::string)(size_t)(size_t))
    (std::shared_ptr<unity_sgraph_base>, rename_edge_fields, (const std::vector<std::string>&)(const std::vector<std::string>&))
    (std::shared_ptr<unity_sgraph_base>, swap_edge_fields, (const std::string&)(const std::string&))

    (std::shared_ptr<unity_sgraph_base>, lambda_triple_apply, (const std::string&)(const std::vector<std::string>&))
    (std::shared_ptr<unity_sgraph_base>, lambda_triple_apply_native, (const function_closure_info&)(const std::vector<std::string>&))
)

} // namespace turi
#endif // TURI_UNITY_GRAPH_INTERFACE_HPP
