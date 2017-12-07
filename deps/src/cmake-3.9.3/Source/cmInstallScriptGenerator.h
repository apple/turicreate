/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallScriptGenerator_h
#define cmInstallScriptGenerator_h

#include "cmConfigure.h"

#include "cmInstallGenerator.h"

#include <iosfwd>
#include <string>

/** \class cmInstallScriptGenerator
 * \brief Generate target installation rules.
 */
class cmInstallScriptGenerator : public cmInstallGenerator
{
public:
  cmInstallScriptGenerator(const char* script, bool code,
                           const char* component, bool exclude_from_all);
  ~cmInstallScriptGenerator() CM_OVERRIDE;

protected:
  void GenerateScript(std::ostream& os) CM_OVERRIDE;
  std::string Script;
  bool Code;
};

#endif
