/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4786)
#endif

#include KWSYS_HEADER(FStream.hxx)
#include <string.h>
#ifdef __BORLANDC__
#include <mem.h> /* memcmp */
#endif

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#include "FStream.hxx.in"
#endif

#include <iostream>

static int testNoFile()
{
  kwsys::ifstream in_file("NoSuchFile.txt");
  if (in_file) {
    return 1;
  }

  return 0;
}

static const int num_test_files = 7;
static const int max_test_file_size = 45;

static kwsys::FStream::BOM expected_bom[num_test_files] = {
  kwsys::FStream::BOM_None,    kwsys::FStream::BOM_None,
  kwsys::FStream::BOM_UTF8,    kwsys::FStream::BOM_UTF16LE,
  kwsys::FStream::BOM_UTF16BE, kwsys::FStream::BOM_UTF32LE,
  kwsys::FStream::BOM_UTF32BE
};

static unsigned char expected_bom_data[num_test_files][5] = {
  { 0 },
  { 0 },
  { 3, 0xEF, 0xBB, 0xBF },
  { 2, 0xFF, 0xFE },
  { 2, 0xFE, 0xFF },
  { 4, 0xFF, 0xFE, 0x00, 0x00 },
  { 4, 0x00, 0x00, 0xFE, 0xFF },
};

static unsigned char file_data[num_test_files][max_test_file_size] = {
  { 1, 'H' },
  { 11, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' },
  { 11, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' },
  { 22,   0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x20,
    0x00, 0x57, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x6C, 0x00, 0x64, 0x00 },
  { 22,   0x00, 0x48, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x6F, 0x00,
    0x20, 0x00, 0x57, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x6C, 0x00, 0x64 },
  { 44,   0x48, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x6C, 0x00, 0x00,
    0x00, 0x6C, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
    0x00, 0x57, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00, 0x72, 0x00, 0x00,
    0x00, 0x6C, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00 },
  { 44,   0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00,
    0x6C, 0x00, 0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00,
    0x72, 0x00, 0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x64 },
};

static int testBOM()
{
  // test various encodings in binary mode
  for (int i = 0; i < num_test_files; i++) {
    {
      kwsys::ofstream out("bom.txt", kwsys::ofstream::binary);
      out.write(reinterpret_cast<const char*>(expected_bom_data[i] + 1),
                *expected_bom_data[i]);
      out.write(reinterpret_cast<const char*>(file_data[i] + 1),
                file_data[i][0]);
    }

    kwsys::ifstream in("bom.txt", kwsys::ofstream::binary);
    kwsys::FStream::BOM bom = kwsys::FStream::ReadBOM(in);
    if (bom != expected_bom[i]) {
      std::cout << "Unexpected BOM " << i << std::endl;
      return 1;
    }
    char data[max_test_file_size];
    in.read(data, file_data[i][0]);
    if (!in.good()) {
      std::cout << "Unable to read data " << i << std::endl;
      return 1;
    }

    if (memcmp(data, file_data[i] + 1, file_data[i][0]) != 0) {
      std::cout << "Incorrect read data " << i << std::endl;
      return 1;
    }
  }

  return 0;
}

int testFStream(int, char* [])
{
  int ret = 0;

  ret |= testNoFile();
  ret |= testBOM();

  return ret;
}
