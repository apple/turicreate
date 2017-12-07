/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackSTGZGenerator_h
#define cmCPackSTGZGenerator_h

#include "cmConfigure.h"

#include "cmCPackGenerator.h"
#include "cmCPackTGZGenerator.h"

#include <iosfwd>

/** \class cmCPackSTGZGenerator
 * \brief A generator for Self extractable TGZ files
 *
 */
class cmCPackSTGZGenerator : public cmCPackTGZGenerator
{
public:
  cmCPackTypeMacro(cmCPackSTGZGenerator, cmCPackTGZGenerator);

  /**
   * Construct generator
   */
  cmCPackSTGZGenerator();
  ~cmCPackSTGZGenerator() CM_OVERRIDE;

protected:
  int PackageFiles() CM_OVERRIDE;
  int InitializeInternal() CM_OVERRIDE;
  int GenerateHeader(std::ostream* os) CM_OVERRIDE;
  const char* GetOutputExtension() CM_OVERRIDE { return ".sh"; }
};

#endif
