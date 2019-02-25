/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmInstallFilesCommand.h"

#include "cmGeneratorExpression.h"
#include "cmGlobalGenerator.h"
#include "cmInstallFilesGenerator.h"
#include "cmInstallGenerator.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmExecutableCommand
bool cmInstallFilesCommand::InitialPass(std::vector<std::string> const& args,
                                        cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // Enable the install target.
  this->Makefile->GetGlobalGenerator()->EnableInstallTarget();

  this->Destination = args[0];

  if ((args.size() > 1) && (args[1] == "FILES")) {
    this->IsFilesForm = true;
    for (std::vector<std::string>::const_iterator s = args.begin() + 2;
         s != args.end(); ++s) {
      // Find the source location for each file listed.
      this->Files.push_back(this->FindInstallSource(s->c_str()));
    }
    this->CreateInstallGenerator();
  } else {
    this->IsFilesForm = false;
    this->FinalArgs.insert(this->FinalArgs.end(), args.begin() + 1,
                           args.end());
  }

  this->Makefile->GetGlobalGenerator()->AddInstallComponent(
    this->Makefile->GetSafeDefinition("CMAKE_INSTALL_DEFAULT_COMPONENT_NAME"));

  return true;
}

void cmInstallFilesCommand::FinalPass()
{
  // No final pass for "FILES" form of arguments.
  if (this->IsFilesForm) {
    return;
  }

  std::string testf;
  std::string const& ext = this->FinalArgs[0];

  // two different options
  if (this->FinalArgs.size() > 1) {
    // now put the files into the list
    std::vector<std::string>::iterator s = this->FinalArgs.begin();
    ++s;
    // for each argument, get the files
    for (; s != this->FinalArgs.end(); ++s) {
      // replace any variables
      std::string const& temps = *s;
      if (!cmSystemTools::GetFilenamePath(temps).empty()) {
        testf = cmSystemTools::GetFilenamePath(temps) + "/" +
          cmSystemTools::GetFilenameWithoutLastExtension(temps) + ext;
      } else {
        testf = cmSystemTools::GetFilenameWithoutLastExtension(temps) + ext;
      }

      // add to the result
      this->Files.push_back(this->FindInstallSource(testf.c_str()));
    }
  } else // reg exp list
  {
    std::vector<std::string> files;
    std::string const& regex = this->FinalArgs[0];
    cmSystemTools::Glob(this->Makefile->GetCurrentSourceDirectory(), regex,
                        files);

    std::vector<std::string>::iterator s = files.begin();
    // for each argument, get the files
    for (; s != files.end(); ++s) {
      this->Files.push_back(this->FindInstallSource(s->c_str()));
    }
  }

  this->CreateInstallGenerator();
}

void cmInstallFilesCommand::CreateInstallGenerator() const
{
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
    this->Files, destination.c_str(), false, no_permissions, no_configurations,
    no_component.c_str(), message, no_exclude_from_all, no_rename));
}

/**
 * Find a file in the build or source tree for installation given a
 * relative path from the CMakeLists.txt file.  This will favor files
 * present in the build tree.  If a full path is given, it is just
 * returned.
 */
std::string cmInstallFilesCommand::FindInstallSource(const char* name) const
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
