/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_SPIRIT_THREADSAFE

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/util/boost_property_tree_utils.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/data/json/json_include.hpp>
namespace turi {

void index_file_information::save(oarchive& oarc) const {
  oarc << index_file << version << nsegments << block_size
       << content_type << segment_sizes << segment_files << metadata;
}
void index_file_information::load(iarchive& iarc) {
  iarc >> index_file >> version >> nsegments >> block_size
       >> content_type >> segment_sizes >> segment_files >> metadata;
}


index_file_information read_index_file(std::string index_file) {
  std::pair<std::string, size_t> parsed_fname = parse_v2_segment_filename(index_file);
  group_index_file_information group_index = read_array_group_index_file(parsed_fname.first);
  logstream(LOG_INFO)  << "Reading index file: " << sanitize_url(parsed_fname.first)
                       << " column " << parsed_fname.second << std::endl;
  if (parsed_fname.second != (size_t)(-1)) {
    // this must be a v2. I am asking for a specific column
    if (parsed_fname.second < group_index.columns.size()) {
      return group_index.columns[parsed_fname.second];
    } else {
      log_and_throw(std::string("column does not exist in sarray index file at ") + index_file);
    }
  } else {
    // return 0th column
    return group_index.columns[0];
  }
}




group_index_file_information read_array_group_index_file(std::string group_index_file) {
  group_index_file_information ret;
  ret.group_index_file = group_index_file;

  // try to open the file
  general_ifstream fin(group_index_file);
  if (fin.fail()) {
    log_and_throw(std::string("Unable to open sarray index file at ") + group_index_file);
  }

  // parse the file
  boost::property_tree::ptree data;
  bool parse_success = false;
  try {
    // try to parse as json
    boost::property_tree::json_parser::read_json(fin, data);
    parse_success = true;
  } catch(...) { }

  if (!parse_success) {
    try {
      // unsucessful. try to parse as ini
      general_ifstream fin(group_index_file);
      boost::property_tree::ini_parser::read_ini(fin, data);
      parse_success = true;
    } catch(...) { }
  }

  if (!parse_success) {
    log_and_throw(std::string("Unable to parse sarray index file ") + group_index_file);
  }

  try {
    // the comon stuff are version, num_segments and segment_files
    ret.version = std::atoi(data.get<std::string>("sarray.version").c_str());
    if (ret.version != 2) {
      log_and_throw(std::string("Only v2 format is supported"));
    }

    ret.nsegments = std::atol(data.get<std::string>("sarray.num_segments").c_str());

    ret.segment_files =
        ini::read_sequence_section<std::string>(data, "segment_files", ret.nsegments);

    if (ret.segment_files.size() != ret.nsegments) {
      log_and_throw(std::string("Malformed index_file_information. nsegments mismatch"));
    }

    // if column_files are relative, we need to do fix it up with the index path
    boost::filesystem::path target_index_path(group_index_file);
    std::string root_dir = target_index_path.parent_path().string();
    for (auto& fname: ret.segment_files) {
      // if it "looks" like a URL continue
      if (fname.empty() || boost::algorithm::contains(fname, "://")) continue;
      // otherwise, it is a local file path
      boost::filesystem::path p(fname);
      // if it is a relative path, we need to fixup with the parent path
      if (p.is_relative()) {
        fname = fileio::make_absolute_path(root_dir, fname);
      }
    }
    boost::property_tree::ptree columns = data.get_child("columns");
    size_t column_number = 0;
    for (auto& key_child : columns) {
      auto& child = key_child.second;
      index_file_information info;
      info.version = ret.version;
      info.nsegments = ret.nsegments;
      info.segment_files = ret.segment_files;
      for (std::string& segfile : info.segment_files) {
        if (parse_v2_segment_filename(segfile).second == (size_t)(-1)) {
          segfile = segfile + ":" + std::to_string(column_number);
        }
      }
      info.index_file = group_index_file + ":" + std::to_string(column_number);
      // now get the segment sizes
      info.content_type = child.get<std::string>("content_type", "");
      info.segment_sizes =
          ini::read_sequence_section<size_t>(child, "segment_sizes", info.nsegments);
      if (child.count("metadata")) {
        info.metadata = ini::read_dictionary_section<std::string>(child, "metadata");
      }
      if (info.segment_sizes.size() != info.nsegments) {
        log_and_throw(std::string("Malformed index_file_information. nsegments mismatch"));
      }
      ret.columns.push_back(info);
      ++column_number;
    }

  } catch(std::string e) {
    log_and_throw(e);
  } catch(...) {
    log_and_throw(std::string("Unable to parse sarray index file ") + group_index_file);
  }

  return ret;
}

/*
 * Earlier versions of the index format uses ini files as the format representation.
 * However, ini files do not support "lists" and hence everythig is a dictionary.
 * This emulates a dictionary by changing a list [a,b,c] to {0000:a, 0001:b, 0002:c} etc.
 */
template <typename T>
std::map<std::string, std::string> legacy_vector_to_map(const std::vector<T>& vec) {
  std::map<std::string, std::string> ret_map;
  for(size_t i = 0;i < vec.size(); ++i) {
    std::stringstream strm;
    strm.fill('0'); strm.width(4); strm << i;
    auto key = strm.str();
    ret_map[key] = std::to_string(vec[i]);
  }
  return ret_map;
}

std::map<std::string, std::string> legacy_vector_to_map(const std::vector<std::string>& vec) {
  std::map<std::string, std::string> ret_map;
  for(size_t i = 0;i < vec.size(); ++i) {
    std::stringstream strm;
    strm.fill('0'); strm.width(4); strm << i;
    auto key = strm.str();
    ret_map[key] = vec[i];
  }
  return ret_map;
}

void write_array_group_index_file(std::string group_index_file,
                                  const group_index_file_information& info) {
#define LEGACY_INDEX_FORMAT

  ASSERT_EQ(info.version, 2);
  using boost::filesystem::path;
  using boost::algorithm::starts_with;

  path target_index_path(group_index_file);
  std::string root_dir = target_index_path.parent_path().string();

  JSONNode data;
  // the comon stuff are version, num_segments and segment_files
  JSONNode sarray_node(JSON_NODE);
  sarray_node.set_name("sarray");
  sarray_node.push_back(JSONNode("version", info.version));
          // the unsigned long needed to avoid issues on some compilers
          // where size_t appears to be ambiguous
  sarray_node.push_back(JSONNode("num_segments", (unsigned long)info.nsegments));
  data.push_back(sarray_node);
  ASSERT_EQ(info.segment_files.size(), info.nsegments);
  // relativize segment files
  std::vector<std::string> relativized_file_names;
  for (auto filename: info.segment_files) {
    filename = fileio::make_relative_path(root_dir, filename);
    relativized_file_names.push_back(filename);
  }
#ifdef LEGACY_INDEX_FORMAT
  data.push_back(json::to_json_node("segment_files",
                                    legacy_vector_to_map(relativized_file_names)));
#else
  data.push_back(json::to_json_node("segment_files", relativized_file_names));
#endif

  JSONNode columns(JSON_ARRAY);
  columns.set_name("columns");
  for (size_t i = 0;i < info.columns.size(); ++i) {
    JSONNode column(JSON_NODE);
    column.push_back(JSONNode("content_type", info.columns[i].content_type));
    column.push_back(json::to_json_node("metadata", info.columns[i].metadata));
    ASSERT_EQ(info.columns[i].segment_sizes.size(), info.nsegments);

#ifdef LEGACY_INDEX_FORMAT
  column.push_back(json::to_json_node("segment_sizes",
                                      legacy_vector_to_map(info.columns[i].segment_sizes)));
#else
  column.push_back(json::to_json_node("segment_sizes", info.columns[i].segment_sizes));
#endif
    columns.push_back(column);
  }
  data.push_back(columns);
  // now write the index
  general_ofstream fout(group_index_file);
  fout << data.write_formatted();

  if (!fout.good()) {
    log_and_throw_io_failure("Fail to write. Disk may be full.");
  }
  fout.close();
#undef LEGACY_INDEX_FORMAT
}

std::pair<std::string, size_t> parse_v2_segment_filename(std::string fname) {
  boost::algorithm::trim(fname);
  size_t sep = fname.find_last_of(':');
  size_t column_id = (size_t)(-1);
  if (sep != std::string::npos) {
    // there is a : seperator
    char* intendpos = 0;
    std::string trailing_str = fname.substr(sep + 1);
    errno = 0;
    size_t parsed_column_id = std::strtol(trailing_str.c_str(), &intendpos, 10);
    if (errno != ERANGE) {
      if (intendpos == trailing_str.c_str() + trailing_str.length()) {
        // conversion success. We read all the way to the end of the line.
        column_id = parsed_column_id;
        // rewrite the fname string to be just the root index file
        fname = fname.substr(0, sep);
      } // all other cases fail. this is not of the format [file]:N

    } else {
      //not convertble. this is not of the format [file]:N
    }
  }

  return {fname, column_id};
}

} // namespace turi
