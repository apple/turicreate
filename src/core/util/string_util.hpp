/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_STRING_UTIL_H_
#define TURI_STRING_UTIL_H_

#include <string>
#include <sstream>
#include <vector>

template<class T> std::vector<T> strip_seq_prefix(const std::vector<T>& v, const std::vector<T>& tok) {
  std::vector<T> ret;
  bool good = true;
  int64_t tok_i = 0;
  for (auto s : v) {
    if (tok_i >= static_cast<int64_t>(tok.size())) {
      good = false;
    }
    if (good) {
      if (s != tok[tok_i]) {
        good = false;
      }
    }
    if (!good) {
      ret.push_back(s);
    }
    ++tok_i;
  }
  return ret;
}

bool starts_with(const std::string& x, const std::string& x_sub);
bool ends_with(const std::string& x, const std::string& x_sub);
bool contains(const std::string& x, const std::string& x_sub);

bool starts_with(const std::vector<std::string>& x, const std::vector<std::string>& x_sub);
bool ends_with(const std::vector<std::string>& x, const std::vector<std::string>& x_sub);

std::vector<std::string> split(const std::string& s, const std::string& tok);
std::string join(const std::vector<std::string>& v, const std::string& tok);
std::string lstrip_all(const std::string& s, const std::string& tok);
std::string rstrip_all(const std::string& s, const std::string& tok);
std::string strip_all(const std::string& s, const std::string& tok);

#endif
