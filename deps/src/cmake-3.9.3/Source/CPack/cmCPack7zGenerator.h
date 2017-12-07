/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPack7zGenerator_h
#define cmCPack7zGenerator_h

#include "cmConfigure.h"

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPack7zGenerator
 * \brief A generator for 7z files
 */
class cmCPack7zGenerator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPack7zGenerator, cmCPackArchiveGenerator);

  /**
   * Construct generator
   */
  cmCPack7zGenerator();
  ~cmCPack7zGenerator() CM_OVERRIDE;

protected:
  const char* GetOutputExtension() CM_OVERRIDE { return ".7z"; }
};

#endif
