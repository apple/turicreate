/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTestGenerator_h
#define cmTestGenerator_h

#include "cmConfigure.h"

#include "cmScriptGenerator.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmLocalGenerator;
class cmTest;

/** \class cmTestGenerator
 * \brief Support class for generating install scripts.
 *
 */
class cmTestGenerator : public cmScriptGenerator
{
  CM_DISABLE_COPY(cmTestGenerator)

public:
  cmTestGenerator(cmTest* test,
                  std::vector<std::string> const& configurations =
                    std::vector<std::string>());
  ~cmTestGenerator() CM_OVERRIDE;

  void Compute(cmLocalGenerator* lg);

protected:
  void GenerateScriptConfigs(std::ostream& os, Indent indent) CM_OVERRIDE;
  void GenerateScriptActions(std::ostream& os, Indent indent) CM_OVERRIDE;
  void GenerateScriptForConfig(std::ostream& os, const std::string& config,
                               Indent indent) CM_OVERRIDE;
  void GenerateScriptNoConfig(std::ostream& os, Indent indent) CM_OVERRIDE;
  bool NeedsScriptNoConfig() const CM_OVERRIDE;
  void GenerateOldStyle(std::ostream& os, Indent indent);

  cmLocalGenerator* LG;
  cmTest* Test;
  bool TestGenerated;
};

#endif
