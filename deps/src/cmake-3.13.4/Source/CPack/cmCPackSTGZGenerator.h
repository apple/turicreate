/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackSTGZGenerator_h
#define cmCPackSTGZGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

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
  ~cmCPackSTGZGenerator() override;

protected:
  int PackageFiles() override;
  int InitializeInternal() override;
  int GenerateHeader(std::ostream* os) override;
  const char* GetOutputExtension() override { return ".sh"; }
};

#endif
