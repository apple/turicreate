/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<core/storage/sframe_interface/unity_sgraph.hpp>
#include<core/data/sframe/gl_sgraph.hpp>
#include<core/data/sframe/gl_sframe.hpp>
#include<core/data/sframe/gl_gframe.hpp>
#include<core/data/sframe/gl_sarray.hpp>

namespace turi {

/**************************************************************************/
/*                                                                        */
/*                         gl_sgraph Constructors                         */
/*                                                                        */
/**************************************************************************/

void gl_sgraph::instantiate_new() {
  m_sgraph = std::make_shared<unity_sgraph>();
}

gl_sgraph::gl_sgraph() {
  instantiate_new();
}

gl_sgraph:: gl_sgraph(const gl_sgraph& other) {
  m_sgraph = other.select_fields(other.get_fields());
}

gl_sgraph::gl_sgraph(gl_sgraph&& other) {
  m_sgraph = std::move(other.m_sgraph);
}

gl_sgraph& gl_sgraph::operator=(const gl_sgraph& other) {
  m_sgraph = other.select_fields(other.get_fields());
  return *this;
}

gl_sgraph& gl_sgraph::operator=(gl_sgraph&& other) {
  m_sgraph = std::move(other.m_sgraph);
  return *this;
}


gl_sgraph::gl_sgraph(const gl_sframe& vertex_sframe,
                     const gl_sframe& edge_sframe,
                     const std::string& vid_field,
                     const std::string& src_field,
                     const std::string& dst_field) {
  instantiate_new();
  if (!vertex_sframe.empty()) {
    m_sgraph = add_vertices(vertex_sframe, vid_field).m_sgraph;
  }
  if (!edge_sframe.empty()) {
    m_sgraph = add_edges(edge_sframe, src_field, dst_field).m_sgraph;
  }
}

gl_sgraph::gl_sgraph(const std::string& directory) {
  instantiate_new();
  m_sgraph->load_graph(directory);
}

/**************************************************************************/
/*                                                                        */
/*                   gl_sgraph Implicit Type Converters                   */
/*                                                                        */
/**************************************************************************/

gl_sgraph::gl_sgraph(std::shared_ptr<unity_sgraph> sgraph) {
  m_sgraph = sgraph;
}

gl_sgraph::gl_sgraph(std::shared_ptr<unity_sgraph_base> sgraph) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(sgraph);
}

gl_sgraph::operator std::shared_ptr<unity_sgraph>() const {
  return m_sgraph;
}
gl_sgraph::operator std::shared_ptr<unity_sgraph_base>() const {
  return m_sgraph;
}


gl_sgraph gl_sgraph::add_vertices(const gl_sframe& vertices, const std::string& vid_field) const {
  return m_sgraph->add_vertices(vertices, vid_field);
}

gl_sgraph gl_sgraph::add_edges(const gl_sframe& edges,
                               const std::string& src_field,
                               const std::string& dst_field) const {
  return m_sgraph->add_edges(edges, src_field, dst_field);
}

gl_sgraph gl_sgraph::select_vertex_fields(const std::vector<std::string>& fields) const {
  return m_sgraph->select_vertex_fields(fields);
}

gl_sgraph gl_sgraph::select_edge_fields(const std::vector<std::string>& fields) const {
  return m_sgraph->select_edge_fields(fields);
}

gl_sgraph gl_sgraph::select_fields(const std::vector<std::string>& fields) const {
  auto vfields = get_vertex_fields();
  auto efields = get_edge_fields();
  std::vector<std::string> selected_vfields, selected_efields;
  for (const auto& f : fields) {
    if (std::find(vfields.begin(), vfields.end(), f) != vfields.end()) {
      selected_vfields.push_back(f);
    } else if (std::find(efields.begin(), efields.end(), f) != efields.end()) {
      selected_efields.push_back(f);
    } else {
      std::stringstream error_msg;
      error_msg << "Field " << f << "not in graph";
      throw(error_msg.str());
    }
  }
  return select_vertex_fields(selected_vfields).select_edge_fields(selected_efields);
}

std::map<std::string, flexible_type> gl_sgraph::summary() const {
  return m_sgraph->summary();
}

size_t gl_sgraph::num_vertices() const {
  return summary()["num_vertices"];
}

size_t gl_sgraph::num_edges() const {
  return summary()["num_edges"];
}

std::vector<std::string> gl_sgraph::get_vertex_fields() const {return m_sgraph->get_vertex_fields();}
std::vector<std::string> gl_sgraph::get_edge_fields() const {return m_sgraph->get_edge_fields();}
std::vector<std::string> gl_sgraph::get_fields() const {
  std::vector<std::string> ret;
  auto vfields = get_vertex_fields();
  auto efields = get_edge_fields();
  ret.insert(ret.end(), vfields.begin(), vfields.end());
  ret.insert(ret.end(), efields.begin(), efields.end());
  return ret;
}

std::vector<flex_type_enum> gl_sgraph::get_vertex_field_types() const {
  return m_sgraph->get_vertex_field_types();
}
std::vector<flex_type_enum> gl_sgraph::get_edge_field_types() const {
  return m_sgraph->get_edge_field_types();
}

gl_sframe gl_sgraph::get_edges(
    const std::vector<vid_pair>& ids,
    const std::map<std::string, flexible_type>& fields) const {

  std::vector<flexible_type> src_ids, dst_ids;
  for (const vid_pair& id_pairs : ids) {
    src_ids.push_back(id_pairs.first);
    dst_ids.push_back(id_pairs.second);
  }
  return m_sgraph->get_edges(src_ids, dst_ids, fields);
}

gl_sframe gl_sgraph::get_vertices(const std::vector<flexible_type>& ids,
    const std::map<std::string, flexible_type>& fields) const {
  return m_sgraph->get_vertices(ids, fields);
}


void gl_sgraph::add_vertex_field(gl_sarray column_data, const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->add_vertex_field(column_data, field));
}

void gl_sgraph::add_vertex_field(const flexible_type& column_data, const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(
      m_sgraph->add_vertex_field(gl_sarray::from_const(column_data, num_vertices()), field));
}


void gl_sgraph::remove_vertex_field(const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->delete_vertex_field(field));
}

void gl_sgraph::rename_vertex_fields(const std::vector<std::string>& oldnames,
                                     const std::vector<std::string>& newnames) {
  ASSERT_EQ(oldnames.size(), newnames.size());
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->rename_vertex_fields(oldnames, newnames));
}

void gl_sgraph::_swap_vertex_fields(const std::string& field1, const std::string& field2) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->swap_vertex_fields(field1, field2));
}

void gl_sgraph::add_edge_field(const flexible_type& column_data, const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->add_edge_field(
        gl_sarray::from_const(column_data, num_edges()), field));
}

void gl_sgraph::add_edge_field(gl_sarray column_data, const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->add_edge_field(column_data, field));
}

void gl_sgraph::remove_edge_field(const std::string& field) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->delete_edge_field(field));
}

void gl_sgraph::rename_edge_fields(const std::vector<std::string>& oldnames,
                                     const std::vector<std::string>& newnames) {
  ASSERT_EQ(oldnames.size(), newnames.size());
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->rename_edge_fields(oldnames, newnames));
}

void gl_sgraph::_swap_edge_fields(const std::string& field1, const std::string& field2) {
  m_sgraph = std::dynamic_pointer_cast<unity_sgraph>(m_sgraph->swap_edge_fields(field1, field2));
}

void gl_sgraph::save(const std::string& directory) const {
  m_sgraph->save_graph(directory, "bin");
}

void gl_sgraph::save_reference(const std::string& directory) const {
  m_sgraph->save_reference(directory);
}

gl_sgraph gl_sgraph::triple_apply(const lambda_triple_apply_fn& lambda,
                                  const std::vector<std::string>& mutated_fields) const {
  return m_sgraph->lambda_triple_apply_native(lambda, mutated_fields);
}

gl_gframe gl_sgraph::vertices() {
  return gl_gframe(this, gframe_type_enum::VERTEX_GFRAME);
}

gl_gframe gl_sgraph::edges() {
  return gl_gframe(this, gframe_type_enum::EDGE_GFRAME);
}

std::shared_ptr<unity_sgraph> gl_sgraph::get_proxy() const {
  return m_sgraph;
}

} // end of turicreate
