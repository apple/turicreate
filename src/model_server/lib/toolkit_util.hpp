/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_TOOLKIT_UTIL_HPP

#define TURI_UNITY_TOOLKIT_UTIL_HPP
#include <vector>
#include <utility>
#include <model_server/lib/variant.hpp>

/*
 * This contains a collection of useful utility function for toolkit
 * developement
 */
namespace turi {

template <typename T>
T safe_varmap_get(const variant_map_type& kv, std::string key) {
  if (kv.count(key) == 0) {
    log_and_throw("Required Key " + key + " not found");
  } else {
    return variant_get_value<T>(kv.at(key));
  }
  __builtin_unreachable();
}

/**
 * Extract all flexible_type values from the varmap into a std::map<std::String, flexible_type>.
 * All other value types will be ignored.
 */
inline std::map<std::string, flexible_type> varmap_to_flexmap(const variant_map_type& map) {
  std::map<std::string, flexible_type> ret;
  for (const auto& kv : map) {
    if (kv.second.type() == typeid(flexible_type)) {
      ret[kv.first] = boost::get<flexible_type>(kv.second);
    }
  }
  return ret;
}

/**
 * Cast each flexible type to variant type.
 */
inline std::map<std::string, variant_type> flexmap_to_varmap(const std::map<std::string, flexible_type>& map) {
  std::map<std::string, variant_type> ret;
  for (const auto& kv : map) {
    ret[kv.first] = (variant_type) kv.second;
  }
  return ret;
}


} // namespace turi
#endif
