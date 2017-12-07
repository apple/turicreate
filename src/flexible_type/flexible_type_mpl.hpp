/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FLEXIBLE_TYPE_MPL_HPP
#define TURI_FLEXIBLE_TYPE_MPL_HPP
#include <boost/mpl/vector.hpp>
#include <flexible_type/flexible_type_base_types.hpp>
namespace turi {
typedef boost::mpl::vector<flex_int, flex_float, flex_string, flex_vec, flex_list, flex_dict, flex_date_time> flexible_type_types;
}
#endif
