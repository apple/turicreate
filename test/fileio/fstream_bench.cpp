/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <timer/timer.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/cache_stream.hpp>
using namespace turi;
using namespace fileio;
size_t KBYTES = 1024*128;

void copy(volatile char* d, char* s) {
  volatile size_t* ds = reinterpret_cast<volatile size_t*>(d);
  size_t* ss = reinterpret_cast<size_t*>(s);
  for (size_t i = 0;i < 1024 / sizeof(size_t); ++i) {
    ds[i] = ss[i];
  }
}
void bench(std::string fname) {
  general_ofstream fout(fname);
  char c[1024] = {0};
  timer ti;
  for (size_t i = 0;i < KBYTES; ++i) {
    fout.write(c, 1024);
  }
  fout.close();
  std::cout << KBYTES << "KB written in " << ti.current_time() * 1000 << "ms\n";

  ti.start();
  volatile char j[1024] = {0};
  for (size_t i = 0;i < KBYTES; ++i) {
    copy(j, c);
  }
  fout.close();
  std::cout << KBYTES << "memcpy in " << ti.current_time() * 1000 << "ms\n";

  general_ifstream fin(fname);
  ti.start();
  for (size_t i = 0;i < KBYTES; ++i) {
    fin.read(c, 1024);
  }
  std::cout << KBYTES << "KB sequential read in " << ti.current_time() * 1000 << "ms\n";

  ti.start();
  for (size_t i = 0;i < KBYTES; ++i) {
    fin.seekg(1024 * ((i * 991) % KBYTES), std::ios_base::beg);
    fin.read(c, 1024);
  }
  fin.close();
  std::cout << KBYTES << "KB random read in " << ti.current_time() * 1000 << "ms\n";


  if (fname.substr(0, 5) == "cache") {
    std::cout << "direct from icachestream...\n";
    fileio::icache_stream fin(fname);
    ti.start();
    for (size_t i = 0;i < KBYTES; ++i) {
      fin.read(c, 1024);
    }
    std::cout << KBYTES << "KB sequential read in " << ti.current_time() * 1000 << "ms\n";

    ti.start();
    for (size_t i = 0;i < KBYTES; ++i) {
      fin.seekg(1024 * ((i * 991) % KBYTES), std::ios_base::beg);
      fin.read(c, 1024);
    }
    fin.close();
    std::cout << KBYTES << "KB random read in " << ti.current_time() * 1000 << "ms\n";
  }
}
int main(int argc, char** argv) {
  std::cout << "cache://pika\n";
  bench("cache://pika");
  std::cout << "\n\n\n";
  std::cout << "pika\n";
  bench("./pika");
}
