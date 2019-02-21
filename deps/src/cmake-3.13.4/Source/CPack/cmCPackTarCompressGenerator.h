/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTarCompressGenerator_h
#define cmCPackTarCompressGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackTarCompressGenerator
 * \brief A generator for TarCompress files
 */
class cmCPackTarCompressGenerator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPackTarCompressGenerator, cmCPackArchiveGenerator);
  /**
   * Construct generator
   */
  cmCPackTarCompressGenerator();
  ~cmCPackTarCompressGenerator() override;

protected:
  const char* GetOutputExtension() override { return ".tar.Z"; }
};

#endif
