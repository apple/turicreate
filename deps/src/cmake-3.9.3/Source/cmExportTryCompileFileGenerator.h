/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportTryCompileFileGenerator_h
#define cmExportTryCompileFileGenerator_h

#include "cmConfigure.h"

#include "cmExportFileGenerator.h"

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmMakefile;

class cmExportTryCompileFileGenerator : public cmExportFileGenerator
{
public:
  cmExportTryCompileFileGenerator(cmGlobalGenerator* gg,
                                  std::vector<std::string> const& targets,
                                  cmMakefile* mf);

  /** Set the list of targets to export.  */
  void SetConfig(const std::string& config) { this->Config = config; }
protected:
  // Implement virtual methods from the superclass.
  bool GenerateMainFile(std::ostream& os) CM_OVERRIDE;

  void GenerateImportTargetsConfig(std::ostream&, const std::string&,
                                   std::string const&,
                                   std::vector<std::string>&) CM_OVERRIDE
  {
  }
  void HandleMissingTarget(std::string&, std::vector<std::string>&,
                           cmGeneratorTarget*, cmGeneratorTarget*) CM_OVERRIDE
  {
  }

  void PopulateProperties(cmGeneratorTarget const* target,
                          ImportPropertyMap& properties,
                          std::set<const cmGeneratorTarget*>& emitted);

  std::string InstallNameDir(cmGeneratorTarget* target,
                             const std::string& config) CM_OVERRIDE;

private:
  std::string FindTargets(const std::string& prop,
                          const cmGeneratorTarget* tgt,
                          std::set<const cmGeneratorTarget*>& emitted);

  std::vector<cmGeneratorTarget const*> Exports;
  std::string Config;
};

#endif
