/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTXZGenerator_h
#define cmCPackTXZGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

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
  ~cmCPackTXZGenerator() override;

protected:
  const char* GetOutputExtension() override { return ".tar.xz"; }
};

#endif
