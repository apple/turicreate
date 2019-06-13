/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_STRING_UTIL_H_
#define TURI_STRING_UTIL_H_

#include <util/basic_types.hpp>

#include <string>
#include <sstream>
#include <vector>

using std::cerr;
using std::endl;
using std::ostream;
using std::ostringstream;
using std::string;
using std::vector;

#include <cxxabi.h>

string cc_sprintf(const char* format_str, ...);
string cc_repstr(string s, int64_t count);

string format_hex(const string& s);
string format_hex(int64_t n);

template<class T> string type_str() {
  int32_t status = -1;
  char* ret_c = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
  if (status < 0) {
    cerr << typeid(T).name() << endl;
    AU();
  }
  string ret(ret_c);
  free(ret_c);
  return ret;
}

template<class T> void fmt_single_poly(ostream& os, T arg) {
  os << arg;
}

template<class T> void fmt_single(ostream& os, char c, T arg) {
  if (c != 'v') {
    ostringstream os_inner;
    os_inner << " ***** fmt_single:" << endl;
    os_inner << "  Expected: " << c << endl;
    os_inner << "  Received: " << type_str<T>() << endl;
    cerr << os_inner.str();
    AU();
  }

  fmt_single_poly(os, arg);
}

void fmt_single(ostream& os, char c, int64_t arg);
void fmt_single(ostream& os, char c, string arg);
void fmt_single(ostream& os, char c, const char* arg);


// Recursive variadic template: base case.
void fmt_ext_loop(ostream& os, const string& fmt_str, int64_t i);


// Recursive variadic template: inductive case.
template<class T, class... Ts> void fmt_ext_loop(
  ostream& os, const string& fmt_str, int64_t i, T arg, Ts... args) {

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
    fmt_single(os, fmt_str[i], arg);
    fmt_ext_loop(os, fmt_str, i+1, args...);
    break;
  }
}


template<class... Ts> ostream& fmt(ostream& os_final, const string& fmt_str, Ts... args) {
  ostringstream os;
  fmt_ext_loop(os, fmt_str, 0, args...);
  os_final << os.str();
  return os_final;
}


template<class T> vector<T> strip_seq_prefix(const vector<T>& v, const vector<T>& tok) {
  vector<T> ret;
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

bool starts_with(const string& x, const string& x_sub);
bool ends_with(const string& x, const string& x_sub);
bool contains(const string& x, const string& x_sub);

bool starts_with(const vector<string>& x, const vector<string>& x_sub);
bool ends_with(const vector<string>& x, const vector<string>& x_sub);

vector<string> split(const string& s, const string& tok);
string join(const vector<string>& v, const string& tok);
string lstrip_all(const string& s, const string& tok);
string rstrip_all(const string& s, const string& tok);
string strip_all(const string& s, const string& tok);


template<class Integral> Integral string_to_int_check(string s, int64_t base = 10) {
  size_t n_proc = 0;
  Integral v = static_cast<Integral>(stoull(s, &n_proc, base));
  ASSERT_EQ(n_proc, s.size());
  return v;
}


#endif
