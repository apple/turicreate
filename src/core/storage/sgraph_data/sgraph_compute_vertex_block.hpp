/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_COMPUTE_VERTEX_BLOCK_HPP
#define TURI_SGRAPH_SGRAPH_COMPUTE_VERTEX_BLOCK_HPP
#include <map>
#include <vector>
#include <algorithm>
#include <core/parallel/mutex.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
namespace turi {


/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute SGraph Compute
 * \{
 */

/**
 * Graph Computation Functions
 */
namespace sgraph_compute {

/**
 * Represents a partition of vertices which is held in memory.
 */
template <typename SIterableType>
class vertex_block {
 public:

  /**
   * Loads an SFrame/SArray into memory (accessible directly via m_vertices)
   * if not already loaded.
   */
  void load_if_not_loaded(const SIterableType& sf) {
    if (!m_loaded) {
      load_impl(sf);
      m_loaded = true;
    }
  }

  /**
   * Loads an SFrame/SArray into memory (accessible directly via m_vertices)
   * reloading it if it has already been loaded.
   */
  void load(const SIterableType& sf) {
    load_impl(sf);
    m_loaded = true;
  }

  void flush(SIterableType& outputsf) {
    std::copy(m_vertices.begin(), m_vertices.end(),
              outputsf.get_output_iterator(0));
    outputsf.close();
  }

  void flush(SIterableType& outputsf, const std::vector<size_t>& mutated_field_index) {
    auto out = outputsf.get_output_iterator(0);
    std::vector<flexible_type> temp(mutated_field_index.size());
    for (const auto& value: m_vertices) {
      for (size_t i = 0; i < mutated_field_index.size(); ++i) {
        temp[i] = value[mutated_field_index[i]];
      }
      *out = temp;
      ++out;
    }
    outputsf.close();
  }

  /**
   * Unloads the loaded data, releasing all memory used.
   */
  void unload() {
    m_loaded = false;
    m_vertices.clear();
    m_vertices.shrink_to_fit();
    if (is_modified()) {
      m_reader.reset();
    }
    clear_modified_flag();
  }

  /**
   * Returns true if the SFrame is loaded. False otherwise.
   */
  bool is_loaded() {
    return m_loaded;
  }

  /**
   * Returns true if the SFrame is modified. False otherwise.
   */
  bool is_modified() {
    return m_modified;
  }

  /**
   * Sets the modified flag
   */
  void set_modified_flag() {
    m_modified = true;
  }

  /**
   * Clears the modified flag
   */
  void clear_modified_flag() {
    m_modified = false;
  }

  typename SIterableType::value_type& operator[](size_t i) {
    return m_vertices[i];
  }

  const typename SIterableType::value_type& operator[](size_t i) const {
    return m_vertices[i];
  }
  /// The loaded data
  std::vector<typename SIterableType::value_type> m_vertices;

 private:
  /// Internal load implementation
  void load_impl(const SIterableType& sf) {
    if (m_last_index_file != sf.get_index_file() || !m_reader) {
      m_last_index_file = sf.get_index_file();
      m_reader = std::move(sf.get_reader());
    }
    m_vertices.reserve(sf.size());
    m_reader->read_rows(0, m_reader->size(), m_vertices);
  }

  /// Flag denoting if the data has been loaded
  bool m_loaded = false;
  /// Flag denoting modification
  bool m_modified = false;
  // cache the reader
  std::string m_last_index_file;
  std::unique_ptr<typename SIterableType::reader_type> m_reader;
};


} // sgraph_compute

/// \}
} // turicreate
#endif
