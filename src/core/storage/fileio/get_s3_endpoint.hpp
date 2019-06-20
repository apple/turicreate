/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_GET_S3_ENDPOINT_HPP
#define TURI_FILEIO_GET_S3_ENDPOINT_HPP
#include <vector>
#include <string>

namespace turi {
namespace fileio {
/**
 * \ingroup fileio
 * Returns a complete list of all available S3 region-specific endpoints.
 */
std::vector<std::string> get_s3_endpoints();

/**
 * \ingroup fileio
 * Get an region name from the endpoint url.
 */
std::string get_region_name_from_endpoint(std::string endpoint);

/**
 * \ingroup fileio
 * Returns a S3 bucket specific path. On regular S3 this returns the
 * virtualhosting style bucket. On other explicitly specified endpoints,
 * this returns $S3_ENDPOINT/[bucket]/
 *
 * For consistency, the returned bucket path will *always* end with a "/"
 */
std::string get_bucket_path(const std::string& bucket);

} // fileio
} // turi
#endif
