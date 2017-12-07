/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmNinjaUtilityTargetGenerator.h"

#include "cmCustomCommand.h"
#include "cmCustomCommandGenerator.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalNinjaGenerator.h"
#include "cmLocalNinjaGenerator.h"
#include "cmMakefile.h"
#include "cmNinjaTypes.h"
#include "cmOutputConverter.h"
#include "cmSourceFile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

cmNinjaUtilityTargetGenerator::cmNinjaUtilityTargetGenerator(
  cmGeneratorTarget* target)
  : cmNinjaTargetGenerator(target)
{
}

cmNinjaUtilityTargetGenerator::~cmNinjaUtilityTargetGenerator()
{
}

void cmNinjaUtilityTargetGenerator::Generate()
{
  std::string utilCommandName =
    this->GetLocalGenerator()->GetCurrentBinaryDirectory();
  utilCommandName += cmake::GetCMakeFilesDirectory();
  utilCommandName += "/";
  utilCommandName += this->GetTargetName() + ".util";
  utilCommandName = this->ConvertToNinjaPath(utilCommandName);

  std::vector<std::string> commands;
  cmNinjaDeps deps, outputs, util_outputs(1, utilCommandName);

  const std::vector<cmCustomCommand>* cmdLists[2] = {
    &this->GetGeneratorTarget()->GetPreBuildCommands(),
    &this->GetGeneratorTarget()->GetPostBuildCommands()
  };

  bool uses_terminal = false;

  for (unsigned i = 0; i != 2; ++i) {
    for (std::vector<cmCustomCommand>::const_iterator ci =
           cmdLists[i]->begin();
         ci != cmdLists[i]->end(); ++ci) {
      cmCustomCommandGenerator ccg(*ci, this->GetConfigName(),
                                   this->GetLocalGenerator());
      this->GetLocalGenerator()->AppendCustomCommandDeps(ccg, deps);
      this->GetLocalGenerator()->AppendCustomCommandLines(ccg, commands);
      std::vector<std::string> const& ccByproducts = ccg.GetByproducts();
      std::transform(ccByproducts.begin(), ccByproducts.end(),
                     std::back_inserter(util_outputs), MapToNinjaPath());
      if (ci->GetUsesTerminal()) {
        uses_terminal = true;
      }
    }
  }

  std::vector<cmSourceFile*> sources;
  std::string config =
    this->GetMakefile()->GetSafeDefinition("CMAKE_BUILD_TYPE");
  this->GetGeneratorTarget()->GetSourceFiles(sources, config);
  for (std::vector<cmSourceFile*>::const_iterator source = sources.begin();
       source != sources.end(); ++source) {
    if (cmCustomCommand* cc = (*source)->GetCustomCommand()) {
      cmCustomCommandGenerator ccg(*cc, this->GetConfigName(),
                                   this->GetLocalGenerator());
      this->GetLocalGenerator()->AddCustomCommandTarget(
        cc, this->GetGeneratorTarget());

      // Depend on all custom command outputs.
      const std::vector<std::string>& ccOutputs = ccg.GetOutputs();
      const std::vector<std::string>& ccByproducts = ccg.GetByproducts();
      std::transform(ccOutputs.begin(), ccOutputs.end(),
                     std::back_inserter(deps), MapToNinjaPath());
      std::transform(ccByproducts.begin(), ccByproducts.end(),
                     std::back_inserter(deps), MapToNinjaPath());
    }
  }

  this->GetLocalGenerator()->AppendTargetOutputs(this->GetGeneratorTarget(),
                                                 outputs);
  this->GetLocalGenerator()->AppendTargetDepends(this->GetGeneratorTarget(),
                                                 deps);

  if (commands.empty()) {
    this->GetGlobalGenerator()->WritePhonyBuild(
      this->GetBuildFileStream(),
      "Utility command for " + this->GetTargetName(), outputs, deps);
  } else {
    std::string command =
      this->GetLocalGenerator()->BuildCommandLine(commands);
    const char* echoStr =
      this->GetGeneratorTarget()->GetProperty("EchoString");
    std::string desc;
    if (echoStr) {
      desc = echoStr;
    } else {
      desc = "Running utility command for " + this->GetTargetName();
    }

    // TODO: fix problematic global targets.  For now, search and replace the
    // makefile vars.
    cmSystemTools::ReplaceString(
      command, "$(CMAKE_SOURCE_DIR)",
      this->GetLocalGenerator()
        ->ConvertToOutputFormat(
          this->GetLocalGenerator()->GetSourceDirectory(),
          cmOutputConverter::SHELL)
        .c_str());
    cmSystemTools::ReplaceString(
      command, "$(CMAKE_BINARY_DIR)",
      this->GetLocalGenerator()
        ->ConvertToOutputFormat(
          this->GetLocalGenerator()->GetBinaryDirectory(),
          cmOutputConverter::SHELL)
        .c_str());
    cmSystemTools::ReplaceString(command, "$(ARGS)", "");

    if (command.find('$') != std::string::npos) {
      return;
    }

    for (cmNinjaDeps::const_iterator oi = util_outputs.begin(),
                                     oe = util_outputs.end();
         oi != oe; ++oi) {
      this->GetGlobalGenerator()->SeenCustomCommandOutput(*oi);
    }

    this->GetGlobalGenerator()->WriteCustomCommandBuild(
      command, desc, "Utility command for " + this->GetTargetName(),
      /*depfile*/ "", uses_terminal,
      /*restat*/ true, util_outputs, deps);

    this->GetGlobalGenerator()->WritePhonyBuild(
      this->GetBuildFileStream(), "", outputs,
      cmNinjaDeps(1, utilCommandName));
  }

  // Add an alias for the logical target name regardless of what directory
  // contains it.  Skip this for GLOBAL_TARGET because they are meant to
  // be per-directory and have one at the top-level anyway.
  if (this->GetGeneratorTarget()->GetType() != cmStateEnums::GLOBAL_TARGET) {
    this->GetGlobalGenerator()->AddTargetAlias(this->GetTargetName(),
                                               this->GetGeneratorTarget());
  }
}
