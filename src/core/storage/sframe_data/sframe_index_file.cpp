/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/sframe_data/sframe_index_file.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/util/boost_property_tree_utils.hpp>

namespace turi {
sframe_index_file_information read_sframe_index_file(std::string index_file) {
  sframe_index_file_information ret;

  // try to open the file
  general_ifstream fin(index_file);
  if (fin.fail()) {
    log_and_throw(std::string("Unable to open frame index file at ") + index_file);
  }

  // parse the file
  boost::property_tree::ptree data;
  try {
    boost::property_tree::ini_parser::read_ini(fin, data);
  } catch(boost::property_tree::ini_parser_error e) {
    log_and_throw(std::string("Unable to parse frame index file ") + index_file);
  }

  //read the sframe properties.
  try {
    ret.version = std::atoi(data.get<std::string>("sframe.version").c_str());
    ret.nsegments = (size_t)(-1);
    ret.ncolumns = std::atol(data.get<std::string>("sframe.num_columns").c_str());
    ret.nrows = std::atol(data.get<std::string>("sframe.nrows").c_str());

    ret.column_names =
        ini::read_sequence_section<std::string>(data, "column_names", ret.ncolumns);
    ret.column_files =
        ini::read_sequence_section<std::string>(data, "column_files", ret.ncolumns);

  } catch(std::string e) {
    log_and_throw(e);
  } catch(...) {
    log_and_throw(std::string("Unable to parse sframe index file ") + index_file);
  }


  // Read the metadata
  if (data.count("metadata")) {
    ret.metadata = ini::read_dictionary_section<std::string>(data, "metadata");
  }

  // if column_files are relative, we need to do fix it up with the index path
  boost::filesystem::path target_index_path(index_file);
  std::string root_dir = target_index_path.parent_path().string();
  for (auto& fname: ret.column_files) {
    // if it "looks" like a URL continue
    if (fname.empty() || boost::algorithm::contains(fname, "://")) continue;
    // otherwise, it is a local file path
    boost::filesystem::path p(fname);
    // if it is a relative path, we need to fixup with the parent path
    if (p.is_relative()) {
      fname = fileio::make_absolute_path(root_dir, fname);
    }
  }

  ret.file_name = index_file;
  return ret;
}


void write_sframe_index_file(std::string index_file,
                             const sframe_index_file_information& info) {
  using boost::filesystem::path;
  using boost::algorithm::starts_with;

  path target_index_path(index_file);
  std::string root_dir = target_index_path.parent_path().string();


  if (info.column_names.size() != info.ncolumns ||
      info.column_files.size() != info.ncolumns) {
    log_and_throw(std::string("Malformed index_file_information. ncolumns mismatch"));
  }
  // commit the index file
  boost::property_tree::ptree data;
  data.put("sframe.version", info.version);
  data.put("sframe.num_segments", info.nsegments);
  data.put("sframe.num_columns", info.ncolumns);
  data.put("sframe.nrows", info.nrows);
  ini::write_dictionary_section(data, "metadata", info.metadata);
  ini::write_sequence_section(data, "column_names", info.column_names);

  std::vector<std::string> relativized_file_names;
  for (auto filename: info.column_files) {
    if (root_dir.length() > 0 &&
        boost::algorithm::starts_with(filename, root_dir)) {
      filename = filename.substr(root_dir.length() + 1);
    }
    relativized_file_names.push_back(filename);
  }
  ini::write_sequence_section(data, "column_files", relativized_file_names);

  // now write the index
  general_ofstream fout(index_file);
  boost::property_tree::ini_parser::write_ini(fout, data);
  if (!fout.good()) {
    log_and_throw_io_failure("Fail to write. Disk may be full.");
  }
  fout.close();
}

} // namespace turi
