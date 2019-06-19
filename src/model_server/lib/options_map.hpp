/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_OPTIONS_MAP_HPP
#define TURI_UNITY_OPTIONS_MAP_HPP
namespace turi {
/**
 * \ingroup unity
 * An options map is a map from string to a flexible_type.
 * Also see variant_map_type
 */
typedef std::map<std::string, flexible_type> options_map_t;
}
#endif
