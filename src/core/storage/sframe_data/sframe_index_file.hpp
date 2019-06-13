/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_INDEX_FILE_HPP
#define TURI_UNITY_SFRAME_INDEX_FILE_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/serialization/serialize.hpp>
namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 * Describes all the information in an sframe index file
 */
struct sframe_index_file_information {
  /// The format version of the sframe
  size_t version = -1;
  /// The number of segments in the frame
  size_t nsegments = 0;
  /// The number of columns in the frame
  size_t ncolumns = 0;
  /// The number of rows in the frame
  size_t nrows = 0;
  /// The names of each column. The length of this must match ncolumns
  std::vector<std::string> column_names;
  /// The file names of each column (the sidx files). The length of this must match ncolumns
  std::vector<std::string> column_files;
  /// Any additional metadata stored with the frame
  std::map<std::string, std::string> metadata;

  std::string file_name;

  void save(oarchive& oarc) const {
    oarc << version << nsegments << ncolumns
         << nrows << column_names << column_files << metadata;
  }

  void load(iarchive& iarc) {
    iarc >> version >> nsegments >> ncolumns
         >> nrows >> column_names >> column_files >> metadata;
  }
};
/**
 * Reads an sframe index file from disk.
 * Raises an exception on failure.
 *
 * This function will also automatically de-relativize the
 * \ref sframe_index_file_information::column_files to get absolute paths
 */
sframe_index_file_information read_sframe_index_file(std::string index_file);

/**
 * Writes an sframe index file to disk.
 * Raises an exception on failure.
 *
 * This function will also automatically relativize the
 * \ref sframe_index_file_information::column_files to get relative paths
 * when writing to disk
 */
void write_sframe_index_file(std::string index_file,
                             const sframe_index_file_information& info);

/// \}
} // namespace turi
#endif
