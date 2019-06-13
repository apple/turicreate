/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <tuple>
#include <iostream>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/curl_downloader.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>
#include <core/storage/fileio/set_curl_options.hpp>

#ifndef TC_NO_CURL
extern "C" {
#include <curl/curl.h>
}
#endif

namespace fs = boost::filesystem;

namespace turi {

size_t download_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  FILE *output_file = (FILE *)userdata;
  size_t ret = fwrite(ptr, size, nmemb, output_file);

  if(cppipc::must_cancel()) {
    ret = 0;
    logprogress_stream << "Download cancelled by user.\n";
  }

  return ret;
}

int download_url(std::string url, std::string output_file) {
#ifndef TC_NO_CURL
  // source modified from libcurl code example
  CURL *curl = curl_easy_init();
  logprogress_stream << "Downloading " << url << " to " << output_file << "\n";
  if(curl) {
    FILE* f = fopen(output_file.c_str(), "wb");
    if (f == NULL) return errno;
    CURLcode res;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &download_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)(f));
    turi::fileio::set_curl_options(curl);
    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
      logprogress_stream << "Failed to download " << url << ": " << curl_easy_strerror(res);
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    fclose(f);
    return res;
  }
#else
    log_and_throw("Downloading files not supported when compiled with remote fs turned off.");
#endif
  return -1;
}

std::tuple<int, bool, std::string> download_url(std::string url) {
  // If it is a native path, then return immediately.
  if (!boost::algorithm::contains(url, "://")) {
    return std::make_tuple(0, false, url);
  }

  std::ifstream fin(url.c_str(), std::ifstream::binary);
  // now, check for the file:// protocol header and see if we can access
  // it as a local file
  if (boost::starts_with(url, "file://")) {
    std::string stripped = url.substr(strlen("file://"));
    if (fs::is_directory(fs::path(stripped))) {
      return std::make_tuple(0, false, stripped);
    } else {
      fin.open(stripped.c_str());
      if (fin.good()) {
        return std::make_tuple(0, false, stripped); // strip the file://
      }
      // if we cannot open, it could simply be because the file name has escape
      // characters in it which we do not understand. For instance:
      // file:///home/user/test%20x.txt (where %20 should be a space character).
      // In this case, curl will still be able to understand that it.
      // Thus even if we can't interpret the file, we should try handing it
      // off to curl to see what happens.
    }
  }

  // All local access failed. Now get curl to download it
  std::string tempname = get_temp_name();
  if (tempname == "") return std::make_tuple(-1, false, "");
  // attach the trailing file extension if any
  size_t lastdot = url.find_last_of(".");
  // the last dot must appear after the last directory separator
  size_t last_separator = url.find_last_of("/");
  if (lastdot != std::string::npos && lastdot > last_separator) {
    tempname = tempname + url.substr(lastdot);
  }
  int status = download_url(url, tempname);
  // failed to download
  // delete the temporary file and return failure
  if (status != 0) {
#ifndef TC_NO_CURL
    delete_temp_file(tempname);
#else
  log_and_throw("Remote FS disabled but functionality called.");
#endif
    return std::make_tuple(status, false, "");
  } else {
    return std::make_tuple(status, true, tempname);
  }
}

std::string get_curl_error_string(int status) {
#ifndef TC_NO_CURL
  return curl_easy_strerror(CURLcode(status));
#else
  log_and_throw("Remote FS disabled but functionality called.");
#endif
}

} // namespace turi
