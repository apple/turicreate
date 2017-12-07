/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSourceGroupCommand.h"

#include <set>
#include <sstream>
#include <stddef.h>

#include "cmMakefile.h"
#include "cmSourceGroup.h"
#include "cmSystemTools.h"

namespace {
const size_t RootIndex = 1;
const size_t FilesWithoutPrefixKeywordIndex = 2;
const size_t FilesWithPrefixKeywordIndex = 4;
const size_t PrefixKeywordIndex = 2;

std::vector<std::string> tokenizePath(const std::string& path)
{
  return cmSystemTools::tokenize(path, "\\/");
}

std::string getFullFilePath(const std::string& currentPath,
                            const std::string& path)
{
  std::string fullPath = path;

  if (!cmSystemTools::FileIsFullPath(path.c_str())) {
    fullPath = currentPath;
    fullPath += "/";
    fullPath += path;
  }

  return cmSystemTools::CollapseFullPath(fullPath);
}

std::set<std::string> getSourceGroupFilesPaths(
  const std::string& root, const std::vector<std::string>& files)
{
  std::set<std::string> ret;
  const std::string::size_type rootLength = root.length();

  for (size_t i = 0; i < files.size(); ++i) {
    ret.insert(files[i].substr(rootLength + 1)); // +1 to also omnit last '/'
  }

  return ret;
}

bool rootIsPrefix(const std::string& root,
                  const std::vector<std::string>& files, std::string& error)
{
  for (size_t i = 0; i < files.size(); ++i) {
    if (!cmSystemTools::StringStartsWith(files[i], root.c_str())) {
      error = "ROOT: " + root + " is not a prefix of file: " + files[i];
      return false;
    }
  }

  return true;
}

cmSourceGroup* addSourceGroup(const std::vector<std::string>& tokenizedPath,
                              cmMakefile& makefile)
{
  cmSourceGroup* sg;

  sg = makefile.GetSourceGroup(tokenizedPath);
  if (!sg) {
    makefile.AddSourceGroup(tokenizedPath);
    sg = makefile.GetSourceGroup(tokenizedPath);
    if (!sg) {
      return CM_NULLPTR;
    }
  }

  return sg;
}

std::string prepareFilePathForTree(const std::string& path,
                                   const std::string& currentSourceDir)
{
  if (!cmSystemTools::FileIsFullPath(path)) {
    return cmSystemTools::CollapseFullPath(currentSourceDir + "/" + path);
  }
  return cmSystemTools::CollapseFullPath(path);
}

std::vector<std::string> prepareFilesPathsForTree(
  std::vector<std::string>::const_iterator begin,
  std::vector<std::string>::const_iterator end,
  const std::string& currentSourceDir)
{
  std::vector<std::string> prepared;

  for (; begin != end; ++begin) {
    prepared.push_back(prepareFilePathForTree(*begin, currentSourceDir));
  }

  return prepared;
}

bool addFilesToItsSourceGroups(const std::string& root,
                               const std::set<std::string>& sgFilesPaths,
                               const std::string& prefix, cmMakefile& makefile,
                               std::string& errorMsg)
{
  cmSourceGroup* sg;

  for (std::set<std::string>::const_iterator it = sgFilesPaths.begin();
       it != sgFilesPaths.end(); ++it) {

    std::vector<std::string> tokenizedPath;
    if (!prefix.empty()) {
      tokenizedPath = tokenizePath(prefix + '/' + *it);
    } else {
      tokenizedPath = tokenizePath(*it);
    }

    if (tokenizedPath.size() > 1) {
      tokenizedPath.pop_back();

      sg = addSourceGroup(tokenizedPath, makefile);

      if (!sg) {
        errorMsg = "Could not create source group for file: " + *it;
        return false;
      }
      const std::string fullPath = getFullFilePath(root, *it);
      sg->AddGroupFile(fullPath);
    }
  }

  return true;
}
}

class cmExecutionStatus;

// cmSourceGroupCommand
bool cmSourceGroupCommand::InitialPass(std::vector<std::string> const& args,
                                       cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  if (args[0] == "TREE") {
    std::string error;

    if (!processTree(args, error)) {
      this->SetError(error);
      return false;
    }

    return true;
  }

  std::string delimiter = "\\";
  if (this->Makefile->GetDefinition("SOURCE_GROUP_DELIMITER")) {
    delimiter = this->Makefile->GetDefinition("SOURCE_GROUP_DELIMITER");
  }

  std::vector<std::string> folders =
    cmSystemTools::tokenize(args[0], delimiter);

  cmSourceGroup* sg = CM_NULLPTR;
  sg = this->Makefile->GetSourceGroup(folders);
  if (!sg) {
    this->Makefile->AddSourceGroup(folders);
    sg = this->Makefile->GetSourceGroup(folders);
  }

  if (!sg) {
    this->SetError("Could not create or find source group");
    return false;
  }
  // If only two arguments are given, the pre-1.8 version of the
  // command is being invoked.
  if (args.size() == 2 && args[1] != "FILES") {
    sg->SetGroupRegex(args[1].c_str());
    return true;
  }

  // Process arguments.
  bool doingFiles = false;
  for (unsigned int i = 1; i < args.size(); ++i) {
    if (args[i] == "REGULAR_EXPRESSION") {
      // Next argument must specify the regex.
      if (i + 1 < args.size()) {
        ++i;
        sg->SetGroupRegex(args[i].c_str());
      } else {
        this->SetError("REGULAR_EXPRESSION argument given without a regex.");
        return false;
      }
      doingFiles = false;
    } else if (args[i] == "FILES") {
      // Next arguments will specify files.
      doingFiles = true;
    } else if (doingFiles) {
      // Convert name to full path and add to the group's list.
      std::string src = args[i];
      if (!cmSystemTools::FileIsFullPath(src.c_str())) {
        src = this->Makefile->GetCurrentSourceDirectory();
        src += "/";
        src += args[i];
      }
      src = cmSystemTools::CollapseFullPath(src);
      sg->AddGroupFile(src);
    } else {
      std::ostringstream err;
      err << "Unknown argument \"" << args[i] << "\".  "
          << "Perhaps the FILES keyword is missing.\n";
      this->SetError(err.str());
      return false;
    }
  }

  return true;
}

bool cmSourceGroupCommand::checkTreeArgumentsPreconditions(
  const std::vector<std::string>& args, std::string& errorMsg) const
{
  if (args.size() == 1) {
    errorMsg = "TREE argument given without a root.";
    return false;
  }

  if (args.size() < 3) {
    errorMsg = "Missing FILES arguments.";
    return false;
  }

  if (args[FilesWithoutPrefixKeywordIndex] != "FILES" &&
      args[PrefixKeywordIndex] != "PREFIX") {
    errorMsg = "Unknown argument \"" + args[2] +
      "\". Perhaps the FILES keyword is missing.\n";
    return false;
  }

  if (args[PrefixKeywordIndex] == "PREFIX" &&
      (args.size() < 5 || args[FilesWithPrefixKeywordIndex] != "FILES")) {
    errorMsg = "Missing FILES arguments.";
    return false;
  }

  return true;
}

bool cmSourceGroupCommand::processTree(const std::vector<std::string>& args,
                                       std::string& errorMsg)
{
  if (!checkTreeArgumentsPreconditions(args, errorMsg)) {
    return false;
  }

  const std::string root = cmSystemTools::CollapseFullPath(args[RootIndex]);
  std::string prefix;
  size_t filesBegin = FilesWithoutPrefixKeywordIndex + 1;
  if (args[PrefixKeywordIndex] == "PREFIX") {
    prefix = args[PrefixKeywordIndex + 1];
    filesBegin = FilesWithPrefixKeywordIndex + 1;
  }

  const std::vector<std::string> filesVector =
    prepareFilesPathsForTree(args.begin() + filesBegin, args.end(),
                             this->Makefile->GetCurrentSourceDirectory());

  if (!rootIsPrefix(root, filesVector, errorMsg)) {
    return false;
  }

  std::set<std::string> sourceGroupPaths =
    getSourceGroupFilesPaths(root, filesVector);

  if (!addFilesToItsSourceGroups(root, sourceGroupPaths, prefix,
                                 *(this->Makefile), errorMsg)) {
    return false;
  }

  return true;
}
