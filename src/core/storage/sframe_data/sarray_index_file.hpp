/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SARRAY_INDEX_FILE_HPP
#define TURI_UNITY_SARRAY_INDEX_FILE_HPP
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
namespace turi {
class oarchive;
class iarchive;


/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * Describes all the information in an sarray index file.
 * The index_file_information struct contains all the information assocaited
 * with a *single sarray column*. In a version 1 SArray, the index file
 * describes a single column. As such index_file will point to the actual
 * file location.
 * In a version 2 SArray, the index file describes
 * multiple columns. As such, index_file is of the form
 * [file_location]:[column_number], and column_number may be non-zero.
 * Column numbers are 0 indexed. segment_files are similar. In the v1 format,
 * the segment_files point to the actual files. In the v2 format, the segment
 * files are of the form [file_location]:[column_number].
 */
struct index_file_information {
  /// Input file name
  std::string index_file;
  /// The format version of the sarray
  int version = -1;
  /// The number of segments in the array
  size_t nsegments = 0;
  /// block_size; Required for version 1.
  size_t block_size = 0;
  /// The datatype of the array (typeid(T).name()).
  std::string content_type;
  /// The length of each segment (number of entries).
  std::vector<size_t> segment_sizes;
  /// The file name of each segment
  std::vector<std::string> segment_files;
  /// Any additional metadata stored with the array
  std::map<std::string, std::string> metadata;

  void save(oarchive& oarc) const;
  void load(iarchive& iarc);
};

/**
 * Reads an sarray index file from disk.
 * This will automatically adapt to v1 and v2 index file formats.
 * - If index_file is "xxx.sidx", and is a v1 format, it will be read as normal
 * - If index_file is "xxx.sidx", and is a v2 format (i.e. array group),
 *   it will return the 1st column (column 0) of the group.
 * - If index_file is "xxx.sidx:n", and is a v2 format (i.e. array group),
 *   it will return column n of the group.
 * All other conditions will fail.
 * Raise an exception on failure.
 *
 * This function will also automatically de-relativize the
 * \ref sframe_index_file_information::column_files to get absolute paths
 */
index_file_information read_index_file(std::string index_file);


/**
 * The group index file is the version 2 SArray index file format.
 * It holds multiple columns in a single fileset. As such, the
 * group_index_file_information struct basically comprises of some common
 * information (version, nsegments, etc), but also contains a vector
 * of an index_file_information for each column in the group.
 */
struct group_index_file_information {
  /// Input file name
  std::string group_index_file;
  /// The format version of the sarray
  int version;
  /// The number of segments in the array
  size_t nsegments;
  /// The file name of each segment
  std::vector<std::string> segment_files;
  /**
   * The index file information for each column.
   * The index_file_information basically has fields which mirror the
   * fields here. for instance, version, segment_files
   * basically are the same. The exceptions are:
   *  - columns[0].index_file = group_index_file + ":0"
   *  - columns[0].column_number = 0
   *  - columns[1].index_file = group_index_file + ":1"
   *  - columns[1].column_number = 1
   */
  std::vector<index_file_information> columns;
};
/**
 * Reads an sarray group index file from disk.
 * Raises an exception on failure.
 *
 * An array_group is a group of sarrays in a single collection of files.
 */
group_index_file_information read_array_group_index_file(std::string group_index_file);


/**
 * Writes an sarray v2 index file to disk.
 * Raises an exception on failure.
 *
 * This function will also automatically relativize the
 * \ref sframe_index_file_information::column_files to get relative paths
 * when writing to disk
 */
void write_array_group_index_file(std::string group_index_file,
                                  const group_index_file_information& info);


/**
 * Splits a filename of the form [filename]:N into a pair of {filename, N}.
 * If the filename is not of that form, or cannot be interpreted as that form,
 * {filename, 0} is returned.
 */
std::pair<std::string, size_t> parse_v2_segment_filename(std::string fname);

/// \}
} // namespace turi
#endif
