/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFindPathCommand.h"

#include "cmsys/Glob.hxx"

#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

cmFindPathCommand::cmFindPathCommand()
{
  this->EnvironmentPath = "INCLUDE";
  this->IncludeFileInPath = false;
}

// cmFindPathCommand
bool cmFindPathCommand::InitialPass(std::vector<std::string> const& argsIn,
                                    cmExecutionStatus&)
{
  this->VariableDocumentation = "Path to a file.";
  this->CMakePathName = "INCLUDE";
  if (!this->ParseArguments(argsIn)) {
    return false;
  }
  if (this->AlreadyInCache) {
    // If the user specifies the entry on the command line without a
    // type we should add the type and docstring but keep the original
    // value.
    if (this->AlreadyInCacheWithoutMetaInfo) {
      this->Makefile->AddCacheDefinition(
        this->VariableName, "", this->VariableDocumentation.c_str(),
        (this->IncludeFileInPath ? cmStateEnums::FILEPATH
                                 : cmStateEnums::PATH));
    }
    return true;
  }

  std::string result = this->FindHeader();
  if (!result.empty()) {
    this->Makefile->AddCacheDefinition(
      this->VariableName, result.c_str(), this->VariableDocumentation.c_str(),
      (this->IncludeFileInPath) ? cmStateEnums::FILEPATH : cmStateEnums::PATH);
    return true;
  }
  this->Makefile->AddCacheDefinition(
    this->VariableName, (this->VariableName + "-NOTFOUND").c_str(),
    this->VariableDocumentation.c_str(),
    (this->IncludeFileInPath) ? cmStateEnums::FILEPATH : cmStateEnums::PATH);
  return true;
}

std::string cmFindPathCommand::FindHeader()
{
  std::string header;
  if (this->SearchFrameworkFirst || this->SearchFrameworkOnly) {
    header = this->FindFrameworkHeader();
  }
  if (header.empty() && !this->SearchFrameworkOnly) {
    header = this->FindNormalHeader();
  }
  if (header.empty() && this->SearchFrameworkLast) {
    header = this->FindFrameworkHeader();
  }
  return header;
}

std::string cmFindPathCommand::FindHeaderInFramework(std::string const& file,
                                                     std::string const& dir)
{
  std::string fileName = file;
  std::string frameWorkName;
  std::string::size_type pos = fileName.find('/');
  // if there is a / in the name try to find the header as a framework
  // For example bar/foo.h would look for:
  // bar.framework/Headers/foo.h
  if (pos != std::string::npos) {
    // remove the name from the slash;
    fileName = fileName.substr(pos + 1);
    frameWorkName = file;
    frameWorkName =
      frameWorkName.substr(0, frameWorkName.size() - fileName.size() - 1);
    // if the framework has a path in it then just use the filename
    if (frameWorkName.find('/') != std::string::npos) {
      fileName = file;
      frameWorkName = "";
    }
    if (!frameWorkName.empty()) {
      std::string fpath = dir;
      fpath += frameWorkName;
      fpath += ".framework";
      std::string intPath = fpath;
      intPath += "/Headers/";
      intPath += fileName;
      if (cmSystemTools::FileExists(intPath.c_str())) {
        if (this->IncludeFileInPath) {
          return intPath;
        }
        return fpath;
      }
    }
  }
  // if it is not found yet or not a framework header, then do a glob search
  // for all frameworks in the directory: dir/*.framework/Headers/<file>
  std::string glob = dir;
  glob += "*.framework/Headers/";
  glob += file;
  cmsys::Glob globIt;
  globIt.FindFiles(glob);
  std::vector<std::string> files = globIt.GetFiles();
  if (!files.empty()) {
    std::string fheader = cmSystemTools::CollapseFullPath(files[0]);
    if (this->IncludeFileInPath) {
      return fheader;
    }
    fheader.resize(fheader.size() - file.size());
    return fheader;
  }
  return "";
}

std::string cmFindPathCommand::FindNormalHeader()
{
  std::string tryPath;
  for (std::vector<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    for (std::vector<std::string>::const_iterator p =
           this->SearchPaths.begin();
         p != this->SearchPaths.end(); ++p) {
      tryPath = *p;
      tryPath += *ni;
      if (cmSystemTools::FileExists(tryPath.c_str())) {
        if (this->IncludeFileInPath) {
          return tryPath;
        }
        return *p;
      }
    }
  }
  return "";
}

std::string cmFindPathCommand::FindFrameworkHeader()
{
  for (std::vector<std::string>::const_iterator ni = this->Names.begin();
       ni != this->Names.end(); ++ni) {
    for (std::vector<std::string>::const_iterator p =
           this->SearchPaths.begin();
         p != this->SearchPaths.end(); ++p) {
      std::string fwPath = this->FindHeaderInFramework(*ni, *p);
      if (!fwPath.empty()) {
        return fwPath;
      }
    }
  }
  return "";
}
