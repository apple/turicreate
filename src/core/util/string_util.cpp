/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <core/util/string_util.hpp>
#include <vector>
#include <string>

std::vector<std::string> split(const std::string& s, const std::string& tok) {
  std::vector<std::string> ret;
  int64_t i = 0;
  while (i != static_cast<int64_t>(std::string::npos) && i < static_cast<int64_t>(s.length())) {
    int64_t i_next = s.find(tok, i);
    if (i_next == static_cast<int64_t>(std::string::npos)) {
      ret.push_back(s.substr(i));
      i = i_next;
    } else {
      ret.push_back(s.substr(i, i_next - i));
      i = i_next + tok.size();
    }
  }
  if (i == static_cast<int64_t>(s.size())) {
    ret.push_back(std::string());
  }
  return ret;
}


std::string join(const std::vector<std::string>& v, const std::string& tok) {
  std::ostringstream ret;
  for (int64_t i = 0; i < static_cast<int64_t>(v.size()); i++) {
    if (i > 0) {
      ret << tok;
    }
    ret << v[i];
  }
  return ret.str();
}


std::string lstrip_all(const std::string& s, const std::string& tok) {
  int64_t ret_index = 0;
  while (static_cast<int64_t>(s.find(tok, ret_index)) == ret_index) {
    ret_index += tok.size();
  }
  return s.substr(ret_index);
}


std::string rstrip_all(const std::string& s, const std::string& tok) {
  int64_t ret_index = s.size();
  while (static_cast<int64_t>(s.rfind(tok, ret_index - 1)) == ret_index - static_cast<int64_t>(tok.size())) {
    ret_index -= tok.size();
  }
  return s.substr(0, ret_index);
}


std::string strip_all(const std::string& s, const std::string& tok) {
  return lstrip_all(rstrip_all(s, tok), tok);
}
