/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTarBZip2Generator_h
#define cmCPackTarBZip2Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackTarBZip2Generator
 * \brief A generator for TarBZip2 files
 */
class cmCPackTarBZip2Generator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPackTarBZip2Generator, cmCPackArchiveGenerator);
  /**
   * Construct generator
   */
  cmCPackTarBZip2Generator();
  ~cmCPackTarBZip2Generator() override;

protected:
  const char* GetOutputExtension() override { return ".tar.bz2"; }
};

#endif
