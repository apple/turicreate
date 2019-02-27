/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackTGZGenerator_h
#define cmCPackTGZGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

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
  ~cmCPackTGZGenerator() override;

protected:
  const char* GetOutputExtension() override { return ".tar.gz"; }
};

#endif
