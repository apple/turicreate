/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackPropertiesGenerator_h
#define cmCPackPropertiesGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmScriptGenerator.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmInstalledFile;
class cmLocalGenerator;

/** \class cmCPackPropertiesGenerator
 * \brief Support class for generating CPackProperties.cmake.
 *
 */
class cmCPackPropertiesGenerator : public cmScriptGenerator
{
  CM_DISABLE_COPY(cmCPackPropertiesGenerator)

public:
  cmCPackPropertiesGenerator(cmLocalGenerator* lg,
                             cmInstalledFile const& installedFile,
                             std::vector<std::string> const& configurations);

protected:
  void GenerateScriptForConfig(std::ostream& os, const std::string& config,
                               Indent indent) override;

  cmLocalGenerator* LG;
  cmInstalledFile const& InstalledFile;
};

#endif
