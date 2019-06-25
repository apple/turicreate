/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/export.hpp>
#ifdef TC_ENABLE_REMOTEFS
#include <core/storage/fileio/s3_api.hpp>
#endif

namespace turi {

EXPORT std::string sanitize_url(std::string url) {
#ifdef TC_ENABLE_REMOTEFS
  if (boost::algorithm::starts_with(url, "s3://")) {
#ifdef TC_BUILD_IOS
    log_and_throw_io_failure("Not implemented: compiled without support for s3:// URLs.");
#else
    return sanitize_s3_url(url);
#endif
  } else {
    return url;
  }
#else
    return url;
#endif
}

}
