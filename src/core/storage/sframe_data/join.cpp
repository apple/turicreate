/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/join.hpp>

namespace turi {

sframe join(sframe& sf_left,
            sframe& sf_right,
            std::string join_type,
            const std::map<std::string,std::string> join_columns,
            size_t max_buffer_size) {
  // ***SANITY CHECKS

  std::vector<size_t> left_join_positions;
  std::vector<size_t> right_join_positions;
  for(const auto &col_pair : join_columns) {
    // Check that all columns exist (in both sframes)
    // These will throw if not found
    left_join_positions.push_back(sf_left.column_index(col_pair.first));
    right_join_positions.push_back(sf_right.column_index(col_pair.second));

    // Each column must have matching types to compare effectively
    if(sf_left.column_type(left_join_positions.back()) !=
        sf_right.column_type(right_join_positions.back())) {
      if((sf_left.num_rows() > 0) && (sf_right.num_rows() > 0)) {
        log_and_throw("Columns " + col_pair.first + " and " + col_pair.second +
            " do not have the same type in both SFrames.");
      }
    }
  }

  // Figure out what join type we have to do
  boost::algorithm::to_lower(join_type);

  join_type_t in_join_type;
  if(join_type == "outer") {
    in_join_type = FULL_JOIN;
  } else if(join_type == "left") {
    in_join_type = LEFT_JOIN;
  } else if(join_type == "right") {
    in_join_type = RIGHT_JOIN;
  } else if(join_type == "inner") {
    in_join_type = INNER_JOIN;
  } else {
    log_and_throw("Invalid join type given!");
  }

  // execute join (perhaps multiplex algorithm based on something?)
  join_impl::hash_join_executor join_executor(sf_left,
                                              sf_right,
                                              left_join_positions,
                                              right_join_positions,
                                              in_join_type,
                                              max_buffer_size);

  return join_executor.grace_hash_join();
}

} // end of turicreate
