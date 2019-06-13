/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <core/logging/assertions.hpp>
#include <core/storage/fileio/general_fstream.hpp>
using namespace turi;

int main() {
  size_t length = 5 * 1024LL * 1024LL * 1024LL;
  char* data = new char[length];

  {
    turi::general_ofstream f("large_temp");
    // write some stuff at the 2GB and 4GB boundary so we can check reads
    for (size_t i = 0; i < 256; ++i) {
      data[i + 2LL * 1024 * 1024 * 1024] = 1;
      data[i + 4LL * 1024 * 1024 * 1024] = 2;
    }
    size_t written = f->write(data, length);
    std::cout << written << " ," << f->good() << std::endl;
    f->close();
  }

  {
    turi::general_ifstream f("large_temp");
    // try *really really big reads*
    // clear data
    memset(data, 0, length);
    f.read(data, length);
    ASSERT_EQ(f.gcount(), length);
    for (size_t i = 0; i < 256; ++i) {
      ASSERT_EQ(data[i + 2LL * 1024 * 1024 * 1024], 1);
      ASSERT_EQ(data[i + 4LL * 1024 * 1024 * 1024], 2);
    }
    
    // test seeks past 4GB boundary
    char c[256];
    f.seekg(2LL*1024*1024*1024, std::ios_base::beg);
    f.read(c, 256);
    ASSERT_EQ(f.gcount(), 256);
    for (size_t i = 0 ;i < 256; ++i) {
      ASSERT_EQ(c[i], 1);
    }

    f.seekg(4LL*1024*1024*1024, std::ios_base::beg);
    f.read(c, 256);
    ASSERT_EQ(f.gcount(), 256);
    for (size_t i = 0 ;i < 256; ++i) {
      ASSERT_EQ(c[i], 2);
    }

    // test read at the end
    f.seekg(4LL*1024*1024*1024 + 1024, std::ios_base::beg);
    f.read(data, 1024LL*1024*1024);
    ASSERT_EQ(f.gcount(), 1024LL*1024*1024 - 1024);
    ASSERT_TRUE(f.eof());
  }
}
