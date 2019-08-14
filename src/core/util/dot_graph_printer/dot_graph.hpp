/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DOT_GRAPH_HPP
#define TURI_DOT_GRAPH_HPP
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
namespace turi {
class dot_graph {
 public:
  dot_graph() { }

  /**
   * Returns true if the vertex was succesfully added.
   * False if the vertex already exists.
   */
  inline bool add_vertex(const std::string& vid, const std::string& vlabel="") {
    if (m_vertices.count(vid)) return false;
    m_vertices.insert(vid);
    m_vertex_label[vid] = vlabel;
    return true;
  }

  inline void add_edge(const std::string& src, const std::string& dest) {
    m_edges.push_back({src, dest});
  }

  inline void print(std::ostream& out) {
     out << "digraph G {\n";
     for(auto vertex: m_vertices) {
       out << "\t\"" << vertex << "\" ";
       // output the label
       out << "[label=\"" << m_vertex_label[vertex] << "\"]\n";
     }
     for(auto edge: m_edges) {
       out << "\t\"" << edge.first << "\" -> \"" << edge.second << "\"\n";
     }
     out << "}";
  }


 private:
  std::set<std::string> m_vertices;
  std::vector<std::pair<std::string, std::string>> m_edges;
  std::map<std::string, std::string> m_vertex_label;
};

} // namespace turi;
#endif
