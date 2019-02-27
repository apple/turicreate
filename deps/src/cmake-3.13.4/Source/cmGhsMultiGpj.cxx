/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGhsMultiGpj.h"

#include "cmGeneratedFileStream.h"

void GhsMultiGpj::WriteGpjTag(Types const gpjType,
                              cmGeneratedFileStream* const filestream)
{
  char const* tag;
  switch (gpjType) {
    case INTERGRITY_APPLICATION:
      tag = "INTEGRITY Application";
      break;
    case LIBRARY:
      tag = "Library";
      break;
    case PROJECT:
      tag = "Project";
      break;
    case PROGRAM:
      tag = "Program";
      break;
    case REFERENCE:
      tag = "Reference";
      break;
    case SUBPROJECT:
      tag = "Subproject";
      break;
    default:
      tag = "";
  }
  *filestream << "[" << tag << "]" << std::endl;
}
