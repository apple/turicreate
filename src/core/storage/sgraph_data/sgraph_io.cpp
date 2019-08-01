/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/storage/sgraph_data/sgraph_io.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sframe_data/sframe_io.hpp>
#include <core/data/json/json_include.hpp>

namespace turi {

  void save_sgraph_to_json(const sgraph& g, std::string targetfile) {
    general_ofstream fout(targetfile);
    if (!fout.good()) {
      log_and_throw_io_failure("Fail to write.");
    }

    // for one group only
    JSONNode vertices(JSON_ARRAY);
    vertices.set_name("vertices");
    const auto& vgroup = g.vertex_group();
    const auto& vertex_fields = g.get_vertex_fields();
    for (auto& sf: vgroup) {
      std::vector<std::vector<flexible_type>> buffer;
      sf.get_reader()->read_rows(0, sf.size(), buffer);
      for (auto& row : buffer) {
        JSONNode value;
        sframe_row_to_json(vertex_fields, row, value);
        vertices.push_back(value);
      }
    }

    JSONNode edges(JSON_ARRAY);
    edges.set_name("edges");
    const auto& edge_fields = g.get_edge_fields();
    sframe sf = g.get_edges();
    std::vector<std::vector<flexible_type>> buffer;
    sf.get_reader()->read_rows(0, sf.size(), buffer);
    for (auto& row : buffer) {
      JSONNode value;
      sframe_row_to_json(edge_fields, row, value);
      edges.push_back(value);
    }

    JSONNode everything;
    everything.set_name("graph");
    everything.push_back(vertices);
    everything.push_back(edges);
    fout << everything.write_formatted();
    if (!fout.good()) {
      log_and_throw_io_failure("Fail to write.");
    }
    fout.close();
  }

  void save_sgraph_to_csv(const sgraph& g, std::string targetdir) {
    auto status = fileio::get_file_status(targetdir);
    switch (status.first) {
     case fileio::file_status::MISSING:
       if (!fileio::create_directory(targetdir)) {
          log_and_throw_io_failure("Unable to create directory. " + status.second);
       }
       break;
     case fileio::file_status::DIRECTORY:
       break;
     case fileio::file_status::REGULAR_FILE:
       log_and_throw_io_failure("Cannot save to regular file. Must be a directory.");
       break;
     case fileio::file_status::FS_UNAVAILABLE:
       log_and_throw_io_failure("Error: " + status.second);
       break;
     default:
       std::stringstream ss;
       ss << "Error: Unknown. " << __FILE__ << " at " << __LINE__;
       log_and_throw_io_failure(ss.str());
    }
    // Write vertices
    sframe vertices = g.get_vertices();
    std::string vertex_file_name = targetdir + "/vertices.csv";
    general_ofstream v_fout(vertex_file_name);
    if (!v_fout.good()) {
      log_and_throw_io_failure("Fail to write.");
    }
    std::vector<std::string> vertex_fields = vertices.column_names();
    for (size_t i = 0; i < vertex_fields.size(); ++i) {
      v_fout << vertex_fields[i];
      if (i == vertices.num_columns()-1) {
        v_fout << "\n";
      } else {
        v_fout << ",";
      }
    }
    auto vertex_reader = vertices.get_reader();
    std::vector<std::vector<flexible_type>> buffer;
    size_t cnt = 0;
    size_t buflen = 512*1024;
    size_t bytes_written = 0;
    char* buf = new char[buflen]; // 512k buffer
    while (cnt < vertices.size()) {
      vertex_reader->read_rows(cnt, cnt +  DEFAULT_SARRAY_READER_BUFFER_SIZE, buffer);
      for (const auto& row : buffer) {
        bytes_written = sframe_row_to_csv(row, buf, buflen);
        if (bytes_written == buflen) {
          delete[] buf;
          v_fout.close();
          log_and_throw_io_failure("Row size exceeds max buffer.");
        }
        v_fout.write(buf, bytes_written);
      }
      cnt += buffer.size();
    }
    buffer.clear();
    cnt = 0;
    if (!v_fout.good()) {
      delete[] buf;
      log_and_throw_io_failure("Fail to write.");
    }
    v_fout.close();

    // Write edges
    sframe edges = g.get_edges();

    std::string edge_file_name = targetdir + "/edges.csv";
    general_ofstream e_fout(edge_file_name);
    if (!e_fout.good()) {
       std::stringstream ss;
       ss << "Fail to write to file: " + edge_file_name << ". "<< __FILE__ << " at " << __LINE__;
      log_and_throw_io_failure(ss.str());
    }

    std::vector<std::string> edge_fields = edges.column_names();
    for (size_t i = 0; i < edge_fields.size(); ++i) {
      e_fout << edge_fields[i];
      if (i == edges.num_columns()-1) {
        e_fout << "\n";
      } else {
        e_fout << ",";
      }
    }
    auto edge_reader = edges.get_reader();
    while (cnt < edges.size()) {
      edge_reader->read_rows(cnt, cnt +  DEFAULT_SARRAY_READER_BUFFER_SIZE, buffer);
      for (const auto& row : buffer) {
        bytes_written = sframe_row_to_csv(row, buf, buflen);
        if (bytes_written == buflen) {
          delete[] buf;
          e_fout.close();
          log_and_throw_io_failure("Row size exceeds max buffer for " + edge_file_name);
        }
        e_fout.write(buf, bytes_written);
      }
      cnt += buffer.size();
    }
    if (!e_fout.good()) {
      delete[] buf;
      log_and_throw_io_failure("Fail to write to " + edge_file_name);
    }
    delete[] buf;
    e_fout.close();
  }

}
