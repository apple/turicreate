/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(FStream.hxx)

// Work-around CMake dependency scanning limitation.  This must
// duplicate the above list of headers.
#if 0
#  include "FStream.hxx.in"
#endif

namespace KWSYS_NAMESPACE {
namespace FStream {

BOM ReadBOM(std::istream& in)
{
  if (!in.good()) {
    return BOM_None;
  }
  unsigned long orig = in.tellg();
  unsigned char bom[4];
  in.read(reinterpret_cast<char*>(bom), 2);
  if (!in.good()) {
    in.clear();
    in.seekg(orig);
    return BOM_None;
  }
  if (bom[0] == 0xEF && bom[1] == 0xBB) {
    in.read(reinterpret_cast<char*>(bom + 2), 1);
    if (in.good() && bom[2] == 0xBF) {
      return BOM_UTF8;
    }
  } else if (bom[0] == 0xFE && bom[1] == 0xFF) {
    return BOM_UTF16BE;
  } else if (bom[0] == 0x00 && bom[1] == 0x00) {
    in.read(reinterpret_cast<char*>(bom + 2), 2);
    if (in.good() && bom[2] == 0xFE && bom[3] == 0xFF) {
      return BOM_UTF32BE;
    }
  } else if (bom[0] == 0xFF && bom[1] == 0xFE) {
    unsigned long p = in.tellg();
    in.read(reinterpret_cast<char*>(bom + 2), 2);
    if (in.good() && bom[2] == 0x00 && bom[3] == 0x00) {
      return BOM_UTF32LE;
    }
    in.seekg(p);
    return BOM_UTF16LE;
  }
  in.clear();
  in.seekg(orig);
  return BOM_None;
}

} // FStream namespace
} // KWSYS_NAMESPACE
