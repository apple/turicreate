/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_OPTION_HANDLING_OPTION_INFO_H_
#define TURI_OPTION_HANDLING_OPTION_INFO_H_

#include <string>
#include <vector>
#include <core/data/flexible_type/flexible_type.hpp>

namespace turi { namespace option_handling {

/**
 * \ingroup toolkit_util
 * The primary structure for information regarding the possible
 *  parameters of the algorithm.  The values passed into the model are
 *  checked against this information.
 */
struct option_info {
  /// The name of the parameter.
  std::string name;

  /// A short description of the parameter.
  std::string description;

  /// The default value
  flexible_type default_value;

  /** The type of the parameter.  If REAL or CATEGORICAL, allowed
   *  values are specified in the parameters below.  Integer behaves
   *  like REAL, but a warning is issued if the given value is not an
   *  integer. If BOOL, the specified value must translate to either 0
   *  or 1.  If COLUMN, then it must be a valid column in the data.
   */
  enum {REAL, INTEGER, BOOL, CATEGORICAL, STRING, FLEXIBLE_TYPE} parameter_type;

  /// If real, these specify the allowed range of the model.
  flexible_type lower_bound, upper_bound;

  /// If categorical, this specifies the allowed values.
  std::vector<flexible_type> allowed_values;

  /// Export to dictionary
  flexible_type to_dictionary() const;

  /** Interpret a value according to the current options.
   */
  flexible_type interpret_value(const flexible_type& value) const;

  /** Validate a given option.  If the option doesn't match up with
   * what is specified, a string error is raised detailing what's wrong.
   */
  void check_value(const flexible_type& value) const;

  /// Serialization -- save
  void save(turi::oarchive& oarc) const;

  /// Serialization -- load
  void load(turi::iarchive& iarc);

};

}}

#endif /* TURI_OPTION_HANDLING_OPTION_INFO_H_*/
