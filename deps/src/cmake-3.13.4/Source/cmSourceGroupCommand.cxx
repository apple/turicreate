/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSourceGroupCommand.h"

#include <algorithm>
#include <set>
#include <stddef.h>
#include <utility>

#include "cmMakefile.h"
#include "cmSourceGroup.h"
#include "cmSystemTools.h"

namespace {
const std::string kTreeOptionName = "TREE";
const std::string kPrefixOptionName = "PREFIX";
const std::string kFilesOptionName = "FILES";
const std::string kRegexOptionName = "REGULAR_EXPRESSION";
const std::string kSourceGroupOptionName = "<sg_name>";

std::vector<std::string> tokenizePath(const std::string& path)
{
  return cmSystemTools::tokenize(path, "\\/");
}

std::string getFullFilePath(const std::string& currentPath,
                            const std::string& path)
{
  std::string fullPath = path;

  if (!cmSystemTools::FileIsFullPath(path)) {
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

  for (std::string const& file : files) {
    ret.insert(file.substr(rootLength + 1)); // +1 to also omnit last '/'
  }

  return ret;
}

bool rootIsPrefix(const std::string& root,
                  const std::vector<std::string>& files, std::string& error)
{
  for (std::string const& file : files) {
    if (!cmSystemTools::StringStartsWith(file, root.c_str())) {
      error = "ROOT: " + root + " is not a prefix of file: " + file;
      return false;
    }
  }

  return true;
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
  const std::vector<std::string>& filesPaths,
  const std::string& currentSourceDir)
{
  std::vector<std::string> prepared;

  for (auto const& filePath : filesPaths) {
    prepared.push_back(prepareFilePathForTree(filePath, currentSourceDir));
  }

  return prepared;
}

bool addFilesToItsSourceGroups(const std::string& root,
                               const std::set<std::string>& sgFilesPaths,
                               const std::string& prefix, cmMakefile& makefile,
                               std::string& errorMsg)
{
  cmSourceGroup* sg;

  for (std::string const& sgFilesPath : sgFilesPaths) {

    std::vector<std::string> tokenizedPath;
    if (!prefix.empty()) {
      tokenizedPath = tokenizePath(prefix + '/' + sgFilesPath);
    } else {
      tokenizedPath = tokenizePath(sgFilesPath);
    }

    if (!tokenizedPath.empty()) {
      tokenizedPath.pop_back();

      if (tokenizedPath.empty()) {
        tokenizedPath.push_back("");
      }

      sg = makefile.GetOrCreateSourceGroup(tokenizedPath);

      if (!sg) {
        errorMsg = "Could not create source group for file: " + sgFilesPath;
        return false;
      }
      const std::string fullPath = getFullFilePath(root, sgFilesPath);
      sg->AddGroupFile(fullPath);
    }
  }

  return true;
}
}

class cmExecutionStatus;

// cmSourceGroupCommand
cmSourceGroupCommand::ExpectedOptions
cmSourceGroupCommand::getExpectedOptions() const
{
  ExpectedOptions options;

  options.push_back(kTreeOptionName);
  options.push_back(kPrefixOptionName);
  options.push_back(kFilesOptionName);
  options.push_back(kRegexOptionName);

  return options;
}

bool cmSourceGroupCommand::isExpectedOption(
  const std::string& argument, const ExpectedOptions& expectedOptions)
{
  return std::find(expectedOptions.begin(), expectedOptions.end(), argument) !=
    expectedOptions.end();
}

void cmSourceGroupCommand::parseArguments(
  const std::vector<std::string>& args,
  cmSourceGroupCommand::ParsedArguments& parsedArguments)
{
  const ExpectedOptions expectedOptions = getExpectedOptions();
  size_t i = 0;

  // at this point we know that args vector is not empty

  // if first argument is not one of expected options it's source group name
  if (!isExpectedOption(args[0], expectedOptions)) {
    // get source group name and go to next argument
    parsedArguments[kSourceGroupOptionName].push_back(args[0]);
    ++i;
  }

  for (; i < args.size();) {
    // get current option and increment index to go to next argument
    const std::string& currentOption = args[i++];

    // create current option entry in parsed arguments
    std::vector<std::string>& currentOptionArguments =
      parsedArguments[currentOption];

    // collect option arguments while we won't find another expected option
    while (i < args.size() && !isExpectedOption(args[i], expectedOptions)) {
      currentOptionArguments.push_back(args[i++]);
    }
  }
}

bool cmSourceGroupCommand::InitialPass(std::vector<std::string> const& args,
                                       cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // If only two arguments are given, the pre-1.8 version of the
  // command is being invoked.
  if (args.size() == 2 && args[1] != "FILES") {
    cmSourceGroup* sg = this->Makefile->GetOrCreateSourceGroup(args[0]);

    if (!sg) {
      this->SetError("Could not create or find source group");
      return false;
    }

    sg->SetGroupRegex(args[1].c_str());
    return true;
  }

  ParsedArguments parsedArguments;
  std::string errorMsg;

  parseArguments(args, parsedArguments);

  if (!checkArgumentsPreconditions(parsedArguments, errorMsg)) {
    return false;
  }

  if (parsedArguments.find(kTreeOptionName) != parsedArguments.end()) {
    if (!processTree(parsedArguments, errorMsg)) {
      this->SetError(errorMsg);
      return false;
    }
  } else {
    if (parsedArguments.find(kSourceGroupOptionName) ==
        parsedArguments.end()) {
      this->SetError("Missing source group name.");
      return false;
    }

    cmSourceGroup* sg = this->Makefile->GetOrCreateSourceGroup(args[0]);

    if (!sg) {
      this->SetError("Could not create or find source group");
      return false;
    }

    // handle regex
    if (parsedArguments.find(kRegexOptionName) != parsedArguments.end()) {
      const std::string& sgRegex = parsedArguments[kRegexOptionName].front();
      sg->SetGroupRegex(sgRegex.c_str());
    }

    // handle files
    const std::vector<std::string>& filesArguments =
      parsedArguments[kFilesOptionName];
    for (auto const& filesArg : filesArguments) {
      std::string src = filesArg;
      if (!cmSystemTools::FileIsFullPath(src)) {
        src = this->Makefile->GetCurrentSourceDirectory();
        src += "/";
        src += filesArg;
      }
      src = cmSystemTools::CollapseFullPath(src);
      sg->AddGroupFile(src);
    }
  }

  return true;
}

bool cmSourceGroupCommand::checkArgumentsPreconditions(
  const ParsedArguments& parsedArguments, std::string& errorMsg) const
{
  if (!checkSingleParameterArgumentPreconditions(kPrefixOptionName,
                                                 parsedArguments, errorMsg) ||
      !checkSingleParameterArgumentPreconditions(kTreeOptionName,
                                                 parsedArguments, errorMsg) ||
      !checkSingleParameterArgumentPreconditions(kRegexOptionName,
                                                 parsedArguments, errorMsg)) {
    return false;
  }

  return true;
}

bool cmSourceGroupCommand::processTree(ParsedArguments& parsedArguments,
                                       std::string& errorMsg)
{
  const std::string root =
    cmSystemTools::CollapseFullPath(parsedArguments[kTreeOptionName].front());
  std::string prefix = parsedArguments[kPrefixOptionName].empty()
    ? ""
    : parsedArguments[kPrefixOptionName].front();

  const std::vector<std::string> filesVector =
    prepareFilesPathsForTree(parsedArguments[kFilesOptionName],
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

bool cmSourceGroupCommand::checkSingleParameterArgumentPreconditions(
  const std::string& argument, const ParsedArguments& parsedArguments,
  std::string& errorMsg) const
{
  ParsedArguments::const_iterator foundArgument =
    parsedArguments.find(argument);
  if (foundArgument != parsedArguments.end()) {
    const std::vector<std::string>& optionArguments = foundArgument->second;

    if (optionArguments.empty()) {
      errorMsg = argument + " argument given without an argument.";
      return false;
    }
    if (optionArguments.size() > 1) {
      errorMsg = "too many arguments passed to " + argument + ".";
      return false;
    }
  }

  return true;
}
