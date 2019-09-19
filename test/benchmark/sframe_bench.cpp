/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/parallel_csv_parser.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <timer/timer.hpp>
using namespace turi;

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << argv[0] << " [csv file]\n";
    std::cout << "file must contain headers, and be comma separated\n";
    return 0;
  }
  std::string prefix = get_temp_name();
  timer ti;
  csv_line_tokenizer tokenizer;
  tokenizer.delimiter = ',';
  tokenizer.init();
  sframe frame;
  frame.init_from_csvs(argv[1],
                       tokenizer,
                       true,   // header
                       true,   // continue on failure
                       false,  // do not store errors
                       std::map<std::string, flex_type_enum>());

  std::cout << "CSV file parsed in " << ti.current_time() << " seconds\n";
  std::cout << "Columns are: \n";
  for (size_t i = 0; i < frame.num_columns(); ++i) {
    std::cout << frame.column_name(i) << "\n";
  }
  std::cout << frame.num_rows() << " rows\n";
}
