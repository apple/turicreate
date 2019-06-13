/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_LIB_PARALLEL_CSV_PARSER_HPP
#define TURI_UNITY_LIB_PARALLEL_CSV_PARSER_HPP
#include <string>
#include <vector>
#include <map>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/csv_line_tokenizer.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup csv_utils CSV Parsing and Writing
 * \{
 */

/**
 * std::getline replacement that correctly handles all \\r, \\n and \\r\\n
 * line break characters.
 */
std::istream& eol_safe_getline(std::istream& is, std::string& t);

/**
 * All the options pertaining to top level CSV file handling
 */
struct csv_file_handling_options {
  /// Whether the first (non-commented) line of the file is the column name header.
  bool use_header = true;

  /// Whether we should just skip line errors.
  bool continue_on_failure = false;

  /// Whether failed parses will be stored in an sarray of strings and returned.
  bool store_errors = false;

  /// collection of column name->type. Every other column type will be parsed as a string
  std::map<std::string, flex_type_enum> column_type_hints;

  /// Output column names
  std::vector<std::string> output_columns;

  /// The number of rows to read.  If 0, all lines are read
  size_t row_limit = 0;

  /// Number of rows at the start of each file to ignore
  size_t skip_rows = 0;
};

/**
 * Parses a CSV file / glob of CSV files to an SFrame.
 *
 * \param url Path or Glob to read files
 * \param tokenizer CSV tokenization options
 * \param options Other file handling options
 * \param frame Returned sframe object. This should be an uninitialized sframe.
 * \param frame_sidx_file Location to save the result. Optional. Defaults to cache.
 *
 * \returns a map of filename to sarray<flexible_type> of string type where each
 * row contains a line of the file that failed to parse. This is only filled
 * if options.store_errors = true
 */
std::map<std::string, std::shared_ptr<sarray<flexible_type>>> parse_csvs_to_sframe(
    const std::string& url,
    csv_line_tokenizer& tokenizer,
    csv_file_handling_options options,
    sframe& frame,
    std::string frame_sidx_file = "");

/// \}
} // namespace turi

#endif // TURI_UNITY_LIB_PARALLEL_CSV_PARSER_HPP
