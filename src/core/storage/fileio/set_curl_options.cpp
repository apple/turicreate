/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/fileio/fileio_constants.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/fileio/fs_utils.hpp>
extern "C" {
#include <curl/curl.h>
}

namespace turi {
namespace fileio {

void set_curl_options(void* ecurl) {
  using turi::fileio::get_alternative_ssl_cert_dir;
  using turi::fileio::get_alternative_ssl_cert_file;
  using turi::fileio::insecure_ssl_cert_checks;
  if (!get_alternative_ssl_cert_dir().empty()) {
    if (get_file_status(get_alternative_ssl_cert_file()) == file_status::DIRECTORY) {
      ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_CAPATH, get_alternative_ssl_cert_dir().c_str()), CURLE_OK);
    }
  }
  if (!get_alternative_ssl_cert_file().empty()) {
    if (get_file_status(get_alternative_ssl_cert_file()) == file_status::REGULAR_FILE) {
      ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_CAINFO, get_alternative_ssl_cert_file().c_str()), CURLE_OK);
    }
  }
  if (insecure_ssl_cert_checks()) {
    ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_SSL_VERIFYPEER, 0l), CURLE_OK);
    ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_SSL_VERIFYHOST, 0l), CURLE_OK);
  }
  ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_LOW_SPEED_LIMIT, 1l), CURLE_OK);
  ASSERT_EQ(curl_easy_setopt((CURL*)ecurl, CURLOPT_LOW_SPEED_TIME, 60l), CURLE_OK);
}

} // fileio
} // turicreate
