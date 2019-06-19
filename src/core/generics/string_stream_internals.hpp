/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_STRING_STREAM_INTERNALS_H_
#define TURI_STRING_STREAM_INTERNALS_H_

#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace turi { namespace gl_string_internals {

template<class _Traits>
std::basic_istream<char, _Traits>& stream_in(std::basic_istream<char, _Traits>& is, gl_string& str) {

  using namespace std;

  try {
    typename basic_istream<char, _Traits>::sentry sen(is);

    if (sen) {
      str.clear();
      streamsize n = is.width();
      if (n <= 0)
        n = str.max_size();
      if (n <= 0)
        n = numeric_limits<streamsize>::max();
      streamsize c = 0;
      const ctype<char>& ct = use_facet<ctype<char> >(is.getloc());
      ios_base::iostate err = ios_base::goodbit;
      while (c < n)
      {
        typename _Traits::int_type i = is.rdbuf()->sgetc();
        if (_Traits::eq_int_type(i, _Traits::eof()))
        {
          err |= ios_base::eofbit;
          break;
        }
        char ch = _Traits::to_char_type(i);
        if (ct.is(ct.space, ch))
          break;
        str.push_back(ch);
        ++c;
        is.rdbuf()->sbumpc();
      }
      is.width(0);
      if (c == 0)
        err |= ios_base::failbit;
      is.setstate(err);
    }
    else
      is.setstate(ios_base::failbit);
  }
  catch (...)
  {
    is.setstate(ios_base::badbit);
  }
  return is;
}

template <class _Traits>
std::basic_istream<char, _Traits>&
getline(std::basic_istream<char, _Traits>& is, gl_string& str, char dlm) {

  using namespace std;
  try {
    typename basic_istream<char, _Traits>::sentry sen(is, true);
    if (sen) {
      str.clear();
      ios_base::iostate err = ios_base::goodbit;
      streamsize extr = 0;
      while (true) {
        typename _Traits::int_type i = is.rdbuf()->sbumpc();
        if (_Traits::eq_int_type(i, _Traits::eof())) {
          err |= ios_base::eofbit;
          break;
        }
        ++extr;
        char ch = _Traits::to_char_type(i);
        if (_Traits::eq(ch, dlm))
          break;
        str.push_back(ch);
        if (str.size() == str.max_size()) {
          err |= ios_base::failbit;
          break;
        }
      }
      if (extr == 0)
        err |= ios_base::failbit;
      is.setstate(err);
    }
  } catch (...) {
    is.setstate(ios_base::badbit);
  }
  return is;
}

}}

#endif /* _STRING_STREAM_INTERNALS_H_ */
