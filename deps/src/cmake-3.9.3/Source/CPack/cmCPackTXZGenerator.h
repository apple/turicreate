/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTXZGenerator_h
#define cmCPackTXZGenerator_h

#include "cmConfigure.h"

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackTXZGenerator
 * \brief A generator for TXZ files
 *
 */
class cmCPackTXZGenerator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPackTXZGenerator, cmCPackArchiveGenerator);
  /**
   * Construct generator
   */
  cmCPackTXZGenerator();
  ~cmCPackTXZGenerator() CM_OVERRIDE;

protected:
  const char* GetOutputExtension() CM_OVERRIDE { return ".tar.xz"; }
};

#endif
