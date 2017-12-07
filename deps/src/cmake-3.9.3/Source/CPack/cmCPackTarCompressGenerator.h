/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTarCompressGenerator_h
#define cmCPackTarCompressGenerator_h

#include "cmConfigure.h"

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
  ~cmCPackTarCompressGenerator() CM_OVERRIDE;

protected:
  const char* GetOutputExtension() CM_OVERRIDE { return ".tar.Z"; }
};

#endif
