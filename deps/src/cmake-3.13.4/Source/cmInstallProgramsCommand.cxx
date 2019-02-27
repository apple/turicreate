/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallProgramsCommand.h"

#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmInstallFilesGenerator.h"
#include "cmInstallGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmExecutableCommand
bool cmInstallProgramsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Enable the install target.
  this->Makefile->GetGlobalGenerator()->EnableInstallTarget();

  this->Destination = args[0];

  this->FinalArgs.insert(this->FinalArgs.end(), args.begin() + 1, args.end());

  this->Makefile->GetGlobalGenerator()->AddInstallComponent(
    this->Makefile->GetSafeDefinition("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME"));

  return true;
}

void cmInstallProgramsCommand::FinalPass()
{
  bool files_mode = false;
  if (!this->FinalArgs.empty() && this->FinalArgs[0] == "FILES") {
    files_mode = true;
  }

  // two different options
  if (this->FinalArgs.size() > 1 || files_mode) {
    // for each argument, get the programs
    std::vector<std::string>::iterator s = this->FinalArgs.begin();
    if (files_mode) {
      // Skip the FILES argument in files mode.
      ++s;
    }
    for (; s != this->FinalArgs.end(); ++s) {
      // add to the result
      this->Files.push_back(this->FindInstallSource(s->c_str()));
    }
  } else // reg exp list
  {
    std::vector<std::string> programs;
    cmSystemTools::Glob(this->Makefile->GetCurrentSourceDirectory(),
                        this->FinalArgs[0], programs);

    std::vector<std::string>::iterator s = programs.begin();
    // for each argument, get the programs
    for (; s != programs.end(); ++s) {
      this->Files.push_back(this->FindInstallSource(s->c_str()));
    }
  }

  // Construct the destination.  This command always installs under
  // the prefix.  We skip the leading slash given by the user.
  std::string destination = this->Destination.substr(1);
  cmSystemTools::ConvertToUnixSlashes(destination);
  if (destination.empty()) {
    destination = ".";
  }

  // Use a file install generator.
  const char* no_permissions = "";
  const char* no_rename = "";
  bool no_exclude_from_all = false;
  std::string no_component =
    this->Makefile->GetSafeDefinition("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME");
  std::vector<std::string> no_configurations;
  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(this->Makefile);
  this->Makefile->AddInstallGenerator(new cmInstallFilesGenerator(
    this->Files, destination.c_str(), true, no_permissions, no_configurations,
    no_component.c_str(), message, no_exclude_from_all, no_rename));
}

/**
 * Find a file in the build or source tree for installation given a
 * relative path from the CMakeLists.txt file.  This will favor files
 * present in the build tree.  If a full path is given, it is just
 * returned.
 */
std::string cmInstallProgramsCommand::FindInstallSource(const char* name) const
{
  if (cmSystemTools::FileIsFullPath(name) ||
      cmGeneratorExpression::Find(name) == 0) {
    // This is a full path.
    return name;
  }

  // This is a relative path.
  std::string tb = this->Makefile->GetCurrentBinaryDirectory();
  tb += "/";
  tb += name;
  std::string ts = this->Makefile->GetCurrentSourceDirectory();
  ts += "/";
  ts += name;

  if (cmSystemTools::FileExists(tb)) {
    // The file exists in the binary tree.  Use it.
    return tb;
  }
  if (cmSystemTools::FileExists(ts)) {
    // The file exists in the source tree.  Use it.
    return ts;
  }
  // The file doesn't exist.  Assume it will be present in the
  // binary tree when the install occurs.
  return tb;
}
