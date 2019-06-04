/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/flexible_type/flexible_type.hpp>
#include <functional>
#include <core/data/sframe/gl_sarray.hpp>

namespace turi {

/**  Flattens list or dictionary types to a non-nested dictionary of
 *   (string key : numeric value) pairs. Each nested key is a
 *   concatenation of the keys in the separation with sep_char
 *   separating them.  For example, if sep_char = ".", then
 *
 *     {"a" : {"b" : 1}, "c" : 2}
 *
 *   becomes
 *
 *     {"a.b" : 1, "c" : 2}.
 *
 *   - List and vector elements are handled by converting the index of
 *     the appropriate element to a string.
 *
 *   - String values are handled by treating them as a single
 *     {"string_value" : 1} pair.
 *
 *   - FLEX_UNDEFINED values are handled by replacing them with the
 *     string contents of `undefined_string`.
 *
 *   - image and datetime types are handled by calling
 *     image_value_handler and datetime_value_handler.  These
 *     functions must either throw an exception, which propegates up,
 *     return any other flexible type (e.g. dict, list, vector, etc.),
 *     or return FLEX_UNDEFINED, in which case that value is ignored.
 *
 *
 */
flex_dict to_flat_dict(const flexible_type& dict_or_list,
                                 const flex_string& separator,
                                 const flex_string& undefined_string,
                                 std::function<flexible_type(const flexible_type&)> nonnumeric_value_handler);

/** Identical to to_flat_dict, except that image_policy and
 *  datetime_policy determine the handling of image and datetime types
 *  rather than a custom function.  Currently, the only possible value
 *  for this is "error".
 */
flex_dict to_flat_dict(const flexible_type& input,
                       const flex_string& separator,
                       const flex_string& undefined_string,
                       const std::string& image_policy = "error",
                       const std::string& datetime_policy = "error");

/** Applies to_flat_dict to all elements in an sarray, returning
 *  the transformed sarray.
 */
gl_sarray to_sarray_of_flat_dictionaries(gl_sarray input,
                                         const flex_string& sep,
                                         const flex_string& undefined_string,
                                         const std::string& image_policy = "error",
                                         const std::string& datetime_policy = "error");
}
