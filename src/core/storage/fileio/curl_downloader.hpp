/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_CURL_DOWNLOADER_HPP
#define TURI_UTIL_CURL_DOWNLOADER_HPP
#include <tuple>
#include <string>
#include <cstdlib>

namespace turi {
/**
 * \ingroup fileio
 * Downloads a given URL into a given output file
 * \code
 * retcode = download_url("http://google.com", "google.html");
 * \endcode
 * Returns 0 on success, non-zero (a curl error code) on failure.
 */
int download_url(std::string url, std::string output_file);


/**
 * \ingroup fileio
 * Downlaods a given URL returning the local filename it has been downloaded to.
 * If the url is a remote URL, the URL will be downloaded to a temporary local
 * file (created using tmpnam), and the local file name returned. If the url is
 * a local file, the local filename will be returned directly.
 *
 * \returns A tuple of (error_code, is_temporary, local_file_name)
 * Error_code is non-zero on failure, in which case the other arguments
 * should be ignored. is_temporary is true if the URL is a remote URL.
 * local_file_name contains the local file name in which the data can be
 * accessed.
 *
 * \code
 * std::tie(status, is_temporary, filename) = download_url("http://google.com");
 * \endcode
 * Returns 0 on success, non-zero (a curl error code) on failure.
 */
std::tuple<int, bool, std::string> download_url(std::string url);

/**
 * Returns the curl error string for a curl error code returned by download_url.
 */
std::string get_curl_error_string(int status);
}

#endif
