/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_GL_GFRAME_HPP
#define TURI_UNITY_GL_GFRAME_HPP

#include "gl_sframe.hpp"

namespace turi {
class gl_sframe;
class gl_sgraph;
class gl_sarray;

// Possible types of gl_gframe
enum class gframe_type_enum:int {VERTEX_GFRAME, EDGE_GFRAME};

/**
 * \ingroup gl_sdk
 *
 * A proxy for the \ref gl_sframe for the vertex and edge data of the SGRaph
 */
class gl_gframe : public gl_sframe {
 public:
  gl_gframe() = delete;
  gl_gframe(const gl_gframe&) = default;
  gl_gframe(gl_gframe&&) = default;
  gl_gframe& operator=(const gl_gframe&) = default;
  gl_gframe& operator=(gl_gframe&&) = default;

  /*
   * Construct gl_gframe from sgraph, and sframe.
   */
  gl_gframe(gl_sgraph*, gframe_type_enum);

  /*
   * Implicit converters
   */
  operator std::shared_ptr<unity_sframe>() const;
  operator std::shared_ptr<unity_sframe_base>() const;

  /**
   * Returns number of rows. If type is VERTEX_GFRAME, the value
   * is also the number of vertices (or edges if type is EDGE_GFRAME)
   * in the \ref gl_sgraph.
   *
   * \see gl_sgraph::num_vertices
   * \see gl_sgraph::num_edges
   */
  size_t size() const override;

  /**
   * Returns number of columns. If type is VERTEX_GFRAME, the value
   * is also the number of vertex fields (or edge fields if type is EDGE_GFRAME)
   * in the \ref gl_sgraph.
   */
  size_t num_columns() const override;

  /**
   * Returns a list of column names. If type is VERTEX_GFRAME, the value
   * is also the names of the vertex fields (or edge fields if type is EDGE_GFRAME)
   * in the \ref gl_sgraph.
   *
   * \see gl_sgraph::get_vertex_fields
   * \see gl_sgraph::get_edge_fields
   */
  std::vector<std::string> column_names() const override;

  /**
   * Returns a list of column types. If type is VERTEX_GFRAME, the value
   * is also the names of the vertex fields (or edge fields if type is EDGE_GFRAME)
   * in the \ref gl_sgraph.
   *
   * \see gl_sgraph::get_vertex_field_types
   * \see gl_sgraph::get_edge_field_types
   */
  std::vector<flex_type_enum> column_types() const override;

  /**
   * Add a new column with constant value. If type is VERTEX_GFRAME, the column
   * is added as a new vertex field (or edge field if type is EDGE_GFRAME)
   * in the \ref gl_sgraph.
   *
   * \param data the constant value to fill the column
   * \param name the name of the new column
   *
   * \see gl_sgraph::add_vertex_field(const flexible_type&, const std::string&)
   * \see gl_sgraph::add_edge_field(const flexible_type&, const std::stirng&)
   */
  void add_column(const flexible_type& data, const std::string& name) override;

  /**
   * Add a new column with given column name and data. If type is
   * VERTEX_GFRAME, the column is added as a new vertex field (or edge field if
   * type is EDGE_GFRAME) in the \ref gl_sgraph.
   *
   * \param data the constant value to fill the column
   * \param name the name of the new column
   *
   * \see gl_sgraph::add_vertex_field(const gl_sarray&, const std::string&)
   * \see gl_sgraph::add_edge_field(const gl_sarray&, const std::string&)
   */
  void add_column(const gl_sarray& data, const std::string& name) override;

  /**
   * Batch version of \ref add_column.
   *
   * \param data a map from column name to column data
   */
  void add_columns(const gl_sframe& data) override;

  /**
   * Remove a column with the given name. If type is
   * VERTEX_GFRAME, the column is removed from vertex data (or edge data if
   * type is EDGE_GFRAME) from the \ref gl_sgraph.
   *
   * \param name the column name to be removed
   *
   * \see gl_sgraph::remove_vertex_field
   * \see gl_sgraph::remove_edge_field
   */
  void remove_column(const std::string& name) override;

  /**
   * Rename columns.
   *
   * \param old_to_new_names map from old column name to new column name.
   *
   * \see gl_sgraph::rename_vertex_fields
   * \see gl_sgraph::rename_edge_fields
   */
  void rename(const std::map<std::string, std::string>& old_to_new_names) override;

  /**
   * Swap the order of two columns
   */
  void swap_columns(const std::string& column_1, const std::string& column_2) override;

  virtual std::shared_ptr<unity_sframe> get_proxy() const override;

 private:
  gl_sgraph* m_sgraph;
  gframe_type_enum m_gframe_type;
};

}

#endif
