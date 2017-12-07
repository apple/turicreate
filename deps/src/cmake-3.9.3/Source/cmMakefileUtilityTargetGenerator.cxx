/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMakefileUtilityTargetGenerator.h"

#include <ostream>
#include <string>
#include <vector>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalUnixMakefileGenerator3.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmOSXBundleGenerator.h"
#include "cmSystemTools.h"

cmMakefileUtilityTargetGenerator::cmMakefileUtilityTargetGenerator(
  cmGeneratorTarget* target)
  : cmMakefileTargetGenerator(target)
{
  this->CustomCommandDriver = OnUtility;
  this->OSXBundleGenerator =
    new cmOSXBundleGenerator(target, this->ConfigName);
  this->OSXBundleGenerator->SetMacContentFolders(&this->MacContentFolders);
}

cmMakefileUtilityTargetGenerator::~cmMakefileUtilityTargetGenerator()
{
  delete this->OSXBundleGenerator;
}

void cmMakefileUtilityTargetGenerator::WriteRuleFiles()
{
  this->CreateRuleFile();

  *this->BuildFileStream << "# Utility rule file for "
                         << this->GeneratorTarget->GetName() << ".\n\n";

  if (!this->NoRuleMessages) {
    const char* root = (this->Makefile->IsOn("CMAKE_MAKE_INCLUDE_FROM_ROOT")
                          ? "$(CMAKE_BINARY_DIR)/"
                          : "");
    // Include the progress variables for the target.
    *this->BuildFileStream
      << "# Include the progress variables for this target.\n"
      << this->GlobalGenerator->IncludeDirective << " " << root
      << cmSystemTools::ConvertToOutputPath(
           this->LocalGenerator
             ->MaybeConvertToRelativePath(
               this->LocalGenerator->GetBinaryDirectory(),
               this->ProgressFileNameFull)
             .c_str())
      << "\n\n";
  }

  // write the custom commands for this target
  this->WriteTargetBuildRules();

  // Collect the commands and dependencies.
  std::vector<std::string> commands;
  std::vector<std::string> depends;

  // Utility targets store their rules in pre- and post-build commands.
  this->LocalGenerator->AppendCustomDepends(
    depends, this->GeneratorTarget->GetPreBuildCommands());

  this->LocalGenerator->AppendCustomDepends(
    depends, this->GeneratorTarget->GetPostBuildCommands());

  this->LocalGenerator->AppendCustomCommands(
    commands, this->GeneratorTarget->GetPreBuildCommands(),
    this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());

  // Depend on all custom command outputs for sources
  this->DriveCustomCommands(depends);

  this->LocalGenerator->AppendCustomCommands(
    commands, this->GeneratorTarget->GetPostBuildCommands(),
    this->GeneratorTarget, this->LocalGenerator->GetBinaryDirectory());

  // Add dependencies on targets that must be built first.
  this->AppendTargetDepends(depends);

  // Add a dependency on the rule file itself.
  this->LocalGenerator->AppendRuleDepend(depends,
                                         this->BuildFileNameFull.c_str());

  // If the rule is empty add the special empty rule dependency needed
  // by some make tools.
  if (depends.empty() && commands.empty()) {
    std::string hack = this->GlobalGenerator->GetEmptyRuleHackDepends();
    if (!hack.empty()) {
      depends.push_back(hack);
    }
  }

  // Write the rule.
  this->LocalGenerator->WriteMakeRule(*this->BuildFileStream, CM_NULLPTR,
                                      this->GeneratorTarget->GetName(),
                                      depends, commands, true);

  // Write the main driver rule to build everything in this target.
  this->WriteTargetDriverRule(this->GeneratorTarget->GetName(), false);

  // Write clean target
  this->WriteTargetCleanRules();

  // Write the dependency generation rule.  This must be done last so
  // that multiple output pair information is available.
  this->WriteTargetDependRules();

  // close the streams
  this->CloseFileStreams();
}
