/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <util/string_util.hpp>

#include <util/basic_types.hpp>

#include <string>
using std::max;
using std::string;

string cc_sprintf(const char* format_str, ...) {
  va_list args;

  int64_t len_max = 64;
  char* buf = (char*)malloc(len_max);

  va_start(args, format_str);
  int64_t len_act = vsnprintf(buf, len_max, format_str, args);
  va_end(args);

  if (len_act < len_max) {
    string ret = string(buf);
    free(buf);
    return ret;
  }

  len_max = len_act+1;
  free(buf);
  buf = (char*)malloc(len_max);

  va_start(args, format_str);
  vsnprintf(buf, len_max, format_str, args);
  va_end(args);

  string ret = string(buf);
  free(buf);
  return ret;
}


string cc_repstr(string s, int64_t count) {
  ostringstream os;
  for (int64_t i = 0; i < max<int64_t>(0, count); i++) {
    os << s;
  }
  return os.str();
}


string format_hex(const string& s) {
  ostringstream st;
  for (uint8_t c : s) {
    st << cc_sprintf("%02x", c);
  }
  return st.str();
}


string format_hex(int64_t n) {
  ostringstream st;
  st << cc_sprintf("%016llx", int64_t(n));
  return st.str();
}


void fmt_single(ostream& os, char c, int64_t arg) {
  if (c == 'I') {
    os << cc_sprintf("%lld", arg);
  } else {
    fmt_single_poly(os, arg);
  }
}


void fmt_single(ostream& os, char c, string arg) {
  if (c == 's') {
    os << arg;
  } else {
    fmt_single_poly(os, arg);
  }
}


void fmt_single(ostream& os, char c, const char* arg) {
  if (c == 's') {
    os << arg;
  } else {
    fmt_single_poly(os, arg);
  }
}


// Recursive variadic template: base case.
void fmt_ext_loop(ostream& os, const string& fmt_str, int64_t i) {
  int64_t i_next = 0;
  while (true) {
    i_next = fmt_str.find("%", i);
    if (i_next == -1) {
      os << fmt_str.substr(i);
      break;
    }
    os << fmt_str.substr(i, i_next - i);
    i = i_next + 1;
    if (i >= static_cast<int64_t>(fmt_str.length())) {
      {
        ostringstream os_inner;
        os_inner << " ***** Incomplete pattern at end of format string: " << fmt_str << endl;
        cerr << os_inner.str();
        AU();
      }
    }
    if (fmt_str[i] == '%') {
      os << '%';
      ++i;
      continue;
    }
    {
      ostringstream os_inner;
      os_inner << " ***** Not enough arguments for format string: " << fmt_str << endl;
      cerr << os_inner.str();
      AU();
    }
  }
}


bool contains(const string& x, const string& x_sub) {
  return x.find(x_sub) != string::npos;
}


bool starts_with(const string& x, const string& x_sub) {
  int64_t pos = x.find(x_sub);
  if (pos == static_cast<int64_t>(string::npos)) {
    return false;
  }
  return (pos == 0);
}


bool ends_with(const string& x, const string& x_sub) {
  int64_t pos = x.rfind(x_sub);
  if (pos == static_cast<int64_t>(string::npos)) {
    return false;
  }
  return (pos == len(x) - len(x_sub));
}


bool starts_with(const vector<string>& x, const vector<string>& x_sub) {
  if (len(x) < len(x_sub)) {
    return false;
  }
  for (int64_t i = 0; i < len(x_sub); i++) {
    if (x[i] != x_sub[i]) {
      return false;
    }
  }
  return true;
}


bool ends_with(const vector<string>& x, const vector<string>& x_sub) {
  if (len(x) < len(x_sub)) {
    return false;
  }
  for (int64_t i = 0; i < len(x_sub); i++) {
    if (x[len(x_sub)-1-i] != x_sub[i]) {
      return false;
    }
  }
  return true;
}


vector<string> split(const string& s, const string& tok) {
  vector<string> ret;
  int64_t i = 0;
  while (i != static_cast<int64_t>(string::npos) && i < static_cast<int64_t>(s.length())) {
    int64_t i_next = s.find(tok, i);
    if (i_next == static_cast<int64_t>(string::npos)) {
      ret.push_back(s.substr(i));
      i = i_next;
    } else {
      ret.push_back(s.substr(i, i_next - i));
      i = i_next + tok.length();
    }
  }
  if (i == static_cast<int64_t>(s.length())) {
    ret.push_back(string());
  }
  return ret;
}


string join(const vector<string>& v, const string& tok) {
  ostringstream ret;
  for (int64_t i = 0; i < static_cast<int64_t>(v.size()); i++) {
    if (i > 0) {
      ret << tok;
    }
    ret << v[i];
  }
  return ret.str();
}


string lstrip_all(const string& s, const string& tok) {
  int64_t ret_index = 0;
  while (static_cast<int64_t>(s.find(tok, ret_index)) == ret_index) {
    ret_index += len(tok);
  }
  return s.substr(ret_index);
}


string rstrip_all(const string& s, const string& tok) {
  int64_t ret_index = s.size();
  while (static_cast<int64_t>(s.rfind(tok, ret_index - 1)) == ret_index - len(tok)) {
    ret_index -= len(tok);
  }
  return s.substr(0, ret_index);
}


string strip_all(const string& s, const string& tok) {
  return lstrip_all(rstrip_all(s, tok), tok);
}
