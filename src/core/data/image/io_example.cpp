/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/data/image/io.hpp>
#include <iostream>
#include <core/logging/assertions.hpp>
#include <boost/algorithm/string.hpp>

void usage() {
  std::cerr << "./io_example sample_in.[jpg | png] out.[jpg | png]" << std::endl;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    usage();
    exit(0);
  }

  std::string input(argv[1]);
  std::string output(argv[2]);

  std::cout << "Input: " << input << "\t"
            << "Output: " << output << std::endl;

  size_t raw_size = 0;
  size_t width = 0, height = 0, channels = 0;
  char* data = NULL;
  turi::Format format;

  turi::read_raw_image(input, &data, raw_size, width, height, channels, format, "");
  std::cout << "Width: " << width << "\t Height: " << height
            <<  "\t channels: " << channels << std::endl;

  if (data != NULL) {
    if (boost::algorithm::ends_with(input, "jpg") ||
        boost::algorithm::ends_with(input, "jpeg")) {
      char* out_data;
      size_t out_length;
      turi::decode_jpeg(data, raw_size, &out_data, out_length);
      turi::write_image(output, out_data, width, height, channels, turi::Format::JPG);
      delete[] out_data;
    } else if (boost::algorithm::ends_with(input, "png")){
      char* out_data;
      size_t out_length;
      turi::decode_png(data, raw_size, &out_data, out_length);
      turi::write_image(output, out_data, width, height, channels, turi::Format::PNG);
      delete[] out_data;
    } else {
      std::cerr << "Unsupported format" << std::endl;
    }
    delete[] data;
  }
}
