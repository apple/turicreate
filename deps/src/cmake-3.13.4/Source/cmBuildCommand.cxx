/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmBuildCommand.h"

#include <sstream>

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmake.h"

class cmExecutionStatus;

bool cmBuildCommand::InitialPass(std::vector<std::string> const& args,
                                 cmExecutionStatus&)
{
  // Support the legacy signature of the command:
  //
  if (2 == args.size()) {
    return this->TwoArgsSignature(args);
  }

  return this->MainSignature(args);
}

bool cmBuildCommand::MainSignature(std::vector<std::string> const& args)
{
  if (args.empty()) {
    this->SetError("requires at least one argument naming a CMake variable");
    return false;
  }

  // The cmake variable in which to store the result.
  std::string const& variable = args[0];

  // Parse remaining arguments.
  std::string configuration;
  std::string project_name;
  std::string target;
  enum Doing
  {
    DoingNone,
    DoingConfiguration,
    DoingProjectName,
    DoingTarget
  };
  Doing doing = DoingNone;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "CONFIGURATION") {
      doing = DoingConfiguration;
    } else if (args[i] == "PROJECT_NAME") {
      doing = DoingProjectName;
    } else if (args[i] == "TARGET") {
      doing = DoingTarget;
    } else if (doing == DoingConfiguration) {
      doing = DoingNone;
      configuration = args[i];
    } else if (doing == DoingProjectName) {
      doing = DoingNone;
      project_name = args[i];
    } else if (doing == DoingTarget) {
      doing = DoingNone;
      target = args[i];
    } else {
      std::ostringstream e;
      e << "unknown argument \"" << args[i] << "\"";
      this->SetError(e.str());
      return false;
    }
  }

  // If null/empty CONFIGURATION argument, cmake --build uses 'Debug'
  // in the currently implemented multi-configuration global generators...
  // so we put this code here to end up with the same default configuration
  // as the original 2-arg build_command signature:
  //
  if (configuration.empty()) {
    cmSystemTools::GetEnv("CMAKE_CONFIG_TYPE", configuration);
  }
  if (configuration.empty()) {
    configuration = "Release";
  }

  if (!project_name.empty()) {
    this->Makefile->IssueMessage(
      cmake::AUTHOR_WARNING,
      "Ignoring PROJECT_NAME option because it has no effect.");
  }

  std::string makecommand =
    this->Makefile->GetGlobalGenerator()->GenerateCMakeBuildCommand(
      target, configuration, "", this->Makefile->IgnoreErrorsCMP0061());

  this->Makefile->AddDefinition(variable, makecommand.c_str());

  return true;
}

bool cmBuildCommand::TwoArgsSignature(std::vector<std::string> const& args)
{
  if (args.size() < 2) {
    this->SetError("called with less than two arguments");
    return false;
  }

  std::string const& define = args[0];
  const char* cacheValue = this->Makefile->GetDefinition(define);

  std::string configType;
  if (!cmSystemTools::GetEnv("CMAKE_CONFIG_TYPE", configType) ||
      configType.empty()) {
    configType = "Release";
  }

  std::string makecommand =
    this->Makefile->GetGlobalGenerator()->GenerateCMakeBuildCommand(
      "", configType, "", this->Makefile->IgnoreErrorsCMP0061());

  if (cacheValue) {
    return true;
  }
  this->Makefile->AddCacheDefinition(define, makecommand.c_str(),
                                     "Command used to build entire project "
                                     "from the command line.",
                                     cmStateEnums::STRING);
  return true;
}
