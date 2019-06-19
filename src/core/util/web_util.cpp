/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/web_util.hpp>
#include <core/util/stl_util.hpp>



namespace turi {
  namespace web_util {

    std::string url_decode(const std::string& url) {
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')
      std::string ret_str;
      for (size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '%' &&
            (i+1 < url.size() && isxdigit(url[i+1])) &&
            (i+1 < url.size() && isxdigit(url[i+2]))) {
          const char a = tolower(url[i+1]);
          const char b = tolower(url[i+2]);
          const char new_char = ((HEXTOI(a) << 4) | HEXTOI(b));
          i += 2;
          ret_str.push_back(new_char);
        } else if (url[i] == '+') {
          ret_str.push_back(' ');
        } else {
          ret_str.push_back(url[i]);
        }
      }
#undef HEXTOI
      return ret_str;
    } // end of url decode



    std::map<std::string, std::string> parse_query(const std::string& query) {
      std::vector<std::string> pairs = turi::strsplit(query, ",=", true);
      std::map<std::string, std::string> map;
      for(size_t i = 0; i+1 < pairs.size(); i+=2)
        map[url_decode(pairs[i])] = url_decode(pairs[i+1]);
      return map;
    } // end of parse url query

  } // end of namespace web_util

}; // end of namespace Turi
