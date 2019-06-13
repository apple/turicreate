/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/sframe/gl_sarray.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sgraph.hpp>
#include <core/data/sframe/gl_gframe.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
namespace turi {

std::shared_ptr<unity_sframe> gl_gframe::get_proxy () const {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    return m_sgraph->get_edges();
  } else {
    return m_sgraph->get_vertices();
  }
}

gl_gframe::gl_gframe(gl_sgraph* g, gframe_type_enum t) :
  m_sgraph(g), m_gframe_type(t) {
  DASSERT_TRUE(m_sgraph != NULL);
}

gl_gframe::operator std::shared_ptr<unity_sframe>() const {
  return get_proxy();
}

gl_gframe::operator std::shared_ptr<unity_sframe_base>() const {
  return get_proxy();
}

size_t gl_gframe::size() const {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    return m_sgraph->num_edges();
  } else {
    return m_sgraph->num_vertices();
  }
}

size_t gl_gframe::num_columns() const { return column_names().size(); }

std::vector<std::string> gl_gframe::column_names() const {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    return m_sgraph->get_edge_fields();
  } else {
    return m_sgraph->get_vertex_fields();
  }
}

std::vector<flex_type_enum> gl_gframe::column_types() const {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    return m_sgraph->get_edge_field_types();
  } else {
    return m_sgraph->get_vertex_field_types();
  }
};

void gl_gframe::add_column(const flexible_type& data, const std::string& name) {
  add_column(gl_sarray::from_const(data, size()), name);
}

void gl_gframe::add_column(const gl_sarray& data, const std::string& name) {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    m_sgraph->add_edge_field(data, name);
  } else {
    m_sgraph->add_vertex_field(data, name);
  }
}

void gl_gframe::add_columns(const gl_sframe& data) {
  for (const auto& k: data.column_names()) {
    add_column(data[k], k);
  }
}

void gl_gframe::remove_column(const std::string& name) {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    if (name == sgraph::SRC_COLUMN_NAME) {
      throw(std::string("Cannot remove \"__src_id\" column"));
    } else if (name == sgraph::DST_COLUMN_NAME) {
      throw(std::string("Cannot remove \"__dst_id\" column"));
    }
    m_sgraph->remove_edge_field(name);
  } else {
    if (name == sgraph::VID_COLUMN_NAME) {
      throw(std::string("Cannot remove \"__id\" column"));
    }
    m_sgraph->remove_vertex_field(name);
  }
}

void gl_gframe::swap_columns(const std::string& column_1, const std::string& column_2) {
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    m_sgraph->_swap_edge_fields(column_1, column_2);
  } else {
    m_sgraph->_swap_vertex_fields(column_1, column_2);
  }
}

void gl_gframe::rename(const std::map<std::string, std::string>& old_to_new_names) {
  std::vector<std::string> old_names;
  std::vector<std::string> new_names;
  for (const auto& kv : old_to_new_names) {
    old_names.push_back(kv.first);
    new_names.push_back(kv.second);
  }
  if (m_gframe_type == gframe_type_enum::EDGE_GFRAME) {
    m_sgraph->rename_edge_fields(old_names, new_names);
  } else {
    m_sgraph->rename_vertex_fields(old_names, new_names);
  }
}

} // end of turicreate
