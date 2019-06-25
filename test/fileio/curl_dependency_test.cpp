/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <tuple>
#include <fstream>
#include <core/storage/fileio/curl_downloader.hpp>

std::ofstream fout;
  
size_t write_callback(void *buffer, size_t size, size_t nmemb, void *stream) {
  fout.write((char*)buffer, size * nmemb);
  return size * nmemb;
}
 

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " [webpage]\n";
    exit(0);
  }
  int status; bool is_temp; std::string filename;
  std::tie(status, is_temp, filename) = turi::download_url(argv[1]);
  if (status != 0) {
    std::cout << "Failed to download file\n";
  } else if (is_temp) {
    std::cout << "Temporary file saved to " << filename << "\n";
  } else {
    std::cout << "Is local file at " << filename << "\n";
  }
}

