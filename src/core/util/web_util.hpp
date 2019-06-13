/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_WEB_UTIL_HPP
#define TURI_WEB_UTIL_HPP

#include <string>
#include <map>


namespace turi {
  namespace web_util {

    /**
     * \brief decode a url by converting escape characters
     */
    std::string url_decode(const std::string& url);

    /**
     * \brief convert a query string into a map
     */
    std::map<std::string, std::string> parse_query(const std::string& query);

  } // end of namespace web_util

}; // end of namespace Turi
#endif
