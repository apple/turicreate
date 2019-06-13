/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_ML_DATA_2_TESTING_UTILS_H_
#define TURI_UNITY_ML_DATA_2_TESTING_UTILS_H_

#include <core/storage/sframe_data/sframe.hpp>
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <unordered_set>
#include <core/storage/sframe_data/testing_utils.hpp>

namespace turi { namespace v2 {

/**  Creates a random SFrame for testing purposes.  The
 *  column_type_info gives the types of the column.
 *
 *  \param[in] n_rows The number of observations to run the timing on.
 *  \param[in] column_type_info A string with each character denoting
 *  one type of column.  The legend is as follows:
 *
 *     n:  numeric column.
 *     b:  categorical column with 2 categories.
 *     z:  categorical column with 5 categories.
 *     Z:  categorical column with 10 categories.
 *     c:  categorical column with 100 categories.
 *     C:  categorical column with 1000000 categories.
 *     s:  categorical column with short string keys and 1000 categories.
 *     S:  categorical column with short string keys and 100000 categories.
 *     v:  numeric vector with 10 elements.
 *     V:  numeric vector with 1000 elements.
 *     u:  categorical set with up to 10 elements.
 *     U:  categorical set with up to 1000 elements.
 *     d:  dictionary with 10 entries.
 *     D:  dictionary with 100 entries.
 *
 *  \param[in] create_target_column If true, then create a random
 *  target column as well.
 *
 *  \param[in] options Additional ml_data option flags passed
 *  to ml_data::fill.
 *
 *  \return A pair of sframe, with the raw data, and an ml_data object
 *  made from that sframe.
 *
 */
std::pair<sframe, ml_data> make_random_sframe_and_ml_data(
    size_t n_rows, std::string column_types, bool create_target_column = false,
    const std::map<std::string, flexible_type>& options = std::map<std::string, flexible_type>());


/**  The information returned by the
 *
 */
struct sframe_and_side_info {
  sframe main_sframe;
  std::vector<sframe> side_sframes;
  std::vector<std::vector<flexible_type> > joined_data;

  ml_data data;
};


/** Creates an ml_data structure with side information.  side gives a
 *  vector of (nrows, creation_string) pairs.
 *
 */
sframe_and_side_info make_ml_data_with_side_data(
    size_t n_main_rows, const std::string& main,
    const std::vector<std::pair<size_t, std::string> >& side,
    bool create_target_column,
    const std::map<std::string, flexible_type>& options = std::map<std::string, flexible_type>());


/** Better equality testing stuff. Handles out-of-order on the
 *  categorical_vector, which is assumed by ml_data.
 */
static inline bool ml_testing_equals(const flexible_type& v1, const flexible_type& v2) {

  if(v1.get_type() != v2.get_type())
    return false;

  // Have to hijack a few of these here, since the eigen stuff doesn't
  // deal with duplicates well
  switch(v1.get_type()) {
    case flex_type_enum::LIST: {
      return (std::unordered_set<flexible_type>(
          v1.get<flex_list>().begin(), v1.get<flex_list>().end())
              ==
              std::unordered_set<flexible_type>(
                  v2.get<flex_list>().begin(), v2.get<flex_list>().end()));
    }

    case flex_type_enum::VECTOR: {
      if(v1.size() == v2.size()) {
        return v1 == v2;
      } else if(v1.size() == 0) {
        for(size_t i = 0; i < v2.size(); ++i) {
          if(v2[i] != 0) return false;
        }
        return true;
      } else if(v2.size() == 0) {
        for(size_t i = 0; i < v1.size(); ++i) {
          if(v1[i] != 0) return false;
        }
        return true;
      }
      return false;
    }

    default:
      return v2 == v1;

  }
}

}}


/** Printing out a row.
 *
 */
std::ostream& operator<<(std::ostream& os, const std::vector<turi::v2::ml_data_entry>& v);



#endif /* _TESTING_UTILS_H_ */
