/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_function_macros.hpp>
#include <toolkits/feature_engineering/dict_transform_utils.hpp>

using namespace turi;

/** Identical to flatten_to_dict, except that image_policy and
 *  datetime_policy determine the handling of image and datetime types
 *  rather than a custom function.  Currently, the only possible value
 *  for this is "error".
 */
EXPORT flex_dict _to_flat_dict(const flexible_type& input,
                               const flex_string& sep_string,
                               const flex_string& undefined_string,
                               const std::string& image_policy,
                               const std::string& datetime_policy) {
  return to_flat_dict(input, sep_string, undefined_string, image_policy, datetime_policy);
}

/** Applies flatten_to_dict to all elements in an sarray, returning
 *  the transformed sarray.
 */
EXPORT gl_sarray _to_sarray_of_flat_dictionaries(gl_sarray input,
                                                 const flex_string& sep,
                                                 const flex_string& undefined_string,
                                                 const std::string& image_policy,
                                                 const std::string& datetime_policy) {

  return to_sarray_of_flat_dictionaries(input, sep, undefined_string,
                                        image_policy, datetime_policy);
}

BEGIN_FUNCTION_REGISTRATION
REGISTER_FUNCTION(_to_flat_dict, "input", "seperator", "none_tag",
                  "image_value_policy", "datetime_value_policy");

REGISTER_FUNCTION(_to_sarray_of_flat_dictionaries, "data", "seperator", "none_tag",
                  "image_value_policy", "datetime_value_policy");

END_FUNCTION_REGISTRATION
