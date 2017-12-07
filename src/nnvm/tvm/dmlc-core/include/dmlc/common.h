/*!
 *  Copyright (c) 2015 by Contributors
 * \file common.h
 * \brief defines some common utility function.
 */
#ifndef DMLC_COMMON_H_
#define DMLC_COMMON_H_

#include <vector>
#include <string>
#include <sstream>

namespace dmlc {
/*!
 * \brief Split a string by delimiter
 * \param s String to be splitted.
 * \param delim The delimiter.
 * \return a splitted vector of strings.
 */
inline std::vector<std::string> Split(const std::string& s, char delim) {
  std::string item;
  std::istringstream is(s);
  std::vector<std::string> ret;
  while (std::getline(is, item, delim)) {
    ret.push_back(item);
  }
  return ret;
}

/*!
 * \brief hash an object and combines the key with previous keys
 */
template<typename T>
inline size_t HashCombine(size_t key, const T& value) {
  std::hash<T> hash_func;
  return key ^ (hash_func(value) + 0x9e3779b9 + (key << 6) + (key >> 2));
}

/*!
 * \brief specialize for size_t
 */
template<>
inline size_t HashCombine<size_t>(size_t key, const size_t& value) {
  return key ^ (value + 0x9e3779b9 + (key << 6) + (key >> 2));
}

}  // namespace dmlc

#endif  // DMLC_COMMON_H_
