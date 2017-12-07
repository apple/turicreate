/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTGZGenerator_h
#define cmCPackTGZGenerator_h

#include "cmConfigure.h"

#include "cmCPackArchiveGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackTGZGenerator
 * \brief A generator for TGZ files
 *
 */
class cmCPackTGZGenerator : public cmCPackArchiveGenerator
{
public:
  cmCPackTypeMacro(cmCPackTGZGenerator, cmCPackArchiveGenerator);
  /**
   * Construct generator
   */
  cmCPackTGZGenerator();
  ~cmCPackTGZGenerator() CM_OVERRIDE;

protected:
  const char* GetOutputExtension() CM_OVERRIDE { return ".tar.gz"; }
};

#endif
