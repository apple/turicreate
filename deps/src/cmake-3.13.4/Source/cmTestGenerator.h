/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTestGenerator_h
#define cmTestGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

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
  ~cmTestGenerator() override;

  void Compute(cmLocalGenerator* lg);

  /** Test if this generator installs the test for a given configuration.  */
  bool TestsForConfig(const std::string& config);

  cmTest* GetTest() const;

protected:
  void GenerateScriptConfigs(std::ostream& os, Indent indent) override;
  void GenerateScriptActions(std::ostream& os, Indent indent) override;
  void GenerateScriptForConfig(std::ostream& os, const std::string& config,
                               Indent indent) override;
  void GenerateScriptNoConfig(std::ostream& os, Indent indent) override;
  bool NeedsScriptNoConfig() const override;
  void GenerateOldStyle(std::ostream& os, Indent indent);

  cmLocalGenerator* LG;
  cmTest* Test;
  bool TestGenerated;
};

#endif
