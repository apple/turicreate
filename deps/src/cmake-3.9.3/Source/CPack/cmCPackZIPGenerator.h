/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackZIPGenerator_h
#define cmCPackZIPGenerator_h

#include "cmConfigure.h"

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackZIPGenerator
 * \brief A generator for ZIP files
 */
class cmCPackZIPGenerator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPackZIPGenerator, cmCPackArchiveGenerator);

  /**
   * Construct generator
   */
  cmCPackZIPGenerator();
  ~cmCPackZIPGenerator() CM_OVERRIDE;

protected:
  const char* GetOutputExtension() CM_OVERRIDE { return ".zip"; }
};

#endif
