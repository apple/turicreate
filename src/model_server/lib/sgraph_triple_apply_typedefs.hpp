/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SGRAPH_TRIPLE_APPLY_TYPEDEFS_HPP
#define TURI_UNITY_SGRAPH_TRIPLE_APPLY_TYPEDEFS_HPP

#include<map>
#include<core/data/flexible_type/flexible_type.hpp>

namespace turi {

/**
 * Argument type and return type for sgraph triple apply.
 */
struct edge_triple {
  std::map<std::string, flexible_type> source;
  std::map<std::string, flexible_type> edge;
  std::map<std::string, flexible_type> target;
};

/**
 * Type of the triple apply lambda function.
 */
typedef std::function<void(edge_triple&)> lambda_triple_apply_fn;

}

#endif
