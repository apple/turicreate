/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/******************************************************************************
 * WARNING! This JSON utility is deprecated.                                  *
 * For bidirectional JSON serialization of flexible_type,                     *
 * use "json_flexible_type.hpp" in src/extensions/json.                 *
 ******************************************************************************/

#ifndef TURI_JSON_INCLUDE
#define TURI_JSON_INCLUDE

#define JSON_ISO_STRICT

// Get rid of stupid warnings for this library
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#endif


#include <libjson/libjson.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <vector>
#include <map>
// some useful utilities
namespace turi {
namespace json {


/**
 * Writes a vector of values into a JSON entry.
 *
 * For instance, given a 3 element vector containing {"hello", "pika", "chu"}
 * The vector be represented as
 *
 * key:["hello", "pika", "chu"]
 *
 * \see read_sequence_section
 *
 */
template <typename T>
JSONNode to_json_node(const std::string& key,
                       const std::vector<T>& values) {
  JSONNode ret(JSON_ARRAY);
  ret.set_name(key);
  for (size_t i = 0; i < values.size(); ++i) {
    ret.push_back(JSONNode("", values[i]));
  }
  return ret;
}

#ifdef _WIN32
// this specialization is needed on some compilers because
// size_t --> JSONNode int types is ambiguous. Because unsigned long is 32 bit.
// on windows and size_t is 64-bit, there is no JSONNode overload to handle
// 64 bit integers and casting has to happen. So it becomes ambiguous what
// you can cast to.
template <>
inline JSONNode to_json_node<size_t>(const std::string& key,
                       const std::vector<size_t>& values) {
  JSONNode ret(JSON_ARRAY);
  ret.set_name(key);
  for (size_t i = 0; i < values.size(); ++i) {
    ret.push_back(JSONNode("", (unsigned long)values[i]));
  }
  return ret;
}
#endif
/**
 * Writes a dictionary of values into an JSON entry.
 * For instance, given a 3 element map containing
 * {"fish":"hello", "and":"pika", "chips":"chu"}.
 *
 * In the json file this will be represented as:
 *
 * {"key": {"fish":"hello",
 *          "and":"pika",
 *          "chips":"chu"}}
 *
 * \see read_dictionary_section
 *
 */
template <typename T>
JSONNode to_json_node(const std::string& key,
                      const std::map<std::string, T>& values) {
  JSONNode ret(JSON_NODE);
  ret.set_name(key);
  for(const auto& map_entry : values) {
    ret.push_back(JSONNode(map_entry.first, map_entry.second));
  }
  return ret;
}

} // json
} // turicreate
#endif
