/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_INI_BOOST_PROPERTY_TREE_UTILS_HPP
#define TURI_INI_BOOST_PROPERTY_TREE_UTILS_HPP

#define BOOST_SPIRIT_THREADSAFE

#include <map>
#include <vector>
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <core/logging/logger.hpp>
namespace turi {
namespace ini {

/**
 * \defgroup groupini INI Utilities
 * \brief Simple ini parsing utilities
 */

/**
 * \ingroup groupini
 *
 * Reads a key in an ini/json file as a sequence of values. In the ini file
 * this will be represented as
 *
 * [key]
 * 0000 = "hello"
 * 0001 = "pika"
 * 0002 = "chu"
 *
 * But in a JSON file this could be
 * {"0000":"hello","0001":"pika","0002":"chu"}
 * or
 * ["hello","pika","chu"]
 * depending on which writer is used. (The boost property tree writer will
 * create the first, a regular JSON writer will create the second).
 *
 * This will return a 3 element vector containing {"hello", "pika", "chu"}
 *
 * \see write_sequence_section
 */
template <typename T>
std::vector<T> read_sequence_section(const boost::property_tree::ptree& data,
                                     std::string key,
                                     size_t expected_elements) {
  std::vector<T> ret;
  if (expected_elements == 0) return ret;
  const boost::property_tree::ptree& section = data.get_child(key);
  ret.resize(expected_elements);

  // loop through the children of the column_names section
  size_t sid = 0;
  for(const auto& val: section) {
    const auto& key = val.first;
    if (key.empty()) {
      // this is the array-like sequences
      ret[sid] = boost::lexical_cast<T>(val.second.get_value<std::string>());
      ++sid;
    } else {
      // this is a dictionary-like sequence
      sid = std::stoi(key);
      if (sid >= ret.size()) {
        log_and_throw(std::string("Invalid ID in ") + key + " section."
                      "Segment IDs are expected to be sequential.");
      }
      ret[sid] = boost::lexical_cast<T>(val.second.get_value<std::string>());
    }
  }
  return ret;
}




/**
 * \ingroup groupini
 * Reads a key in an ini/json file as a dictionary of values. In the ini file
 * this will be represented as
 *
 * [key]
 * fish = "hello"
 * and = "pika"
 * chips = "chu"
 *
 * In a JSON file this will be represented as
 * {"fish":"hello", "and":"pika", "chips":"chu"}
 *
 * This will return a 3 element map containing
 * {"fish":"hello", "and":"pika", "chips":"chu"}.
 *
 * \see write_dictionary_section
 */
template <typename T>
std::map<std::string, T> read_dictionary_section(const boost::property_tree::ptree& data,
                                                 std::string key) {
  std::map<std::string, T> ret;
  // no section found
  if (data.count(key) == 0) {
    return ret;
  }
  const boost::property_tree::ptree& section = data.get_child(key);

  // loop through the children of the column_names section
  for(const auto& val: section) {
      ret.insert(std::make_pair(val.first,
                                val.second.get_value<T>()));
  }
  return ret;
}


/**
 * \ingroup groupini
 * Writes a vector of values into an ini file as a section.
 *
 * For instance, given a 3 element vector containing {"hello", "pika", "chu"}
 * The vector be represented as
 *
 * [key]
 * 0000 = "hello"
 * 0001 = "pika"
 * 0002 = "chu"
 *
 * \see read_sequence_section
 *
 */
template <typename T>
void write_sequence_section(boost::property_tree::ptree& data,
                            const std::string& key,
                            const std::vector<T>& values) {
  for (size_t i = 0; i < values.size(); ++i) {
    // make the 4 digit suffix
    std::stringstream strm;
    strm.fill('0'); strm.width(4); strm << i;
    data.put(key + "." + strm.str(), values[i]);
  }
}

/**
 * \ingroup groupini
 * Writes a dictionary of values into an ini file as a section.
 * For instance, given a 3 element map containing
 * {"fish":"hello", "and":"pika", "chips":"chu"}.
 *
 * In the ini file this will be represented as:
 *
 * [key]
 * fish = "hello"
 * and = "pika"
 * chips = "chu"
 *
 * \see read_dictionary_section
 *
 */
template <typename T>
void write_dictionary_section(boost::property_tree::ptree& data,
                            const std::string& key,
                            const std::map<std::string, T>& values) {
  // Write out metadata
  std::string m_heading = key + ".";
  for(const auto& map_entry : values) {
    std::string mkey(m_heading);
    mkey += map_entry.first;
    data.put(mkey, map_entry.second);
  }
}


} // ini
} // namespace turi
#endif
