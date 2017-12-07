/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmAddSubDirectoryCommand.h"

#include <sstream>
#include <string.h>

#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmAddSubDirectoryCommand
bool cmAddSubDirectoryCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  // store the binpath
  std::string const& srcArg = args[0];
  std::string binArg;

  bool excludeFromAll = false;

  // process the rest of the arguments looking for optional args
  std::vector<std::string>::const_iterator i = args.begin();
  ++i;
  for (; i != args.end(); ++i) {
    if (*i == "EXCLUDE_FROM_ALL") {
      excludeFromAll = true;
      continue;
    }
    if (binArg.empty()) {
      binArg = *i;
    } else {
      this->SetError("called with incorrect number of arguments");
      return false;
    }
  }

  // Compute the full path to the specified source directory.
  // Interpret a relative path with respect to the current source directory.
  std::string srcPath;
  if (cmSystemTools::FileIsFullPath(srcArg.c_str())) {
    srcPath = srcArg;
  } else {
    srcPath = this->Makefile->GetCurrentSourceDirectory();
    srcPath += "/";
    srcPath += srcArg;
  }
  if (!cmSystemTools::FileIsDirectory(srcPath)) {
    std::string error = "given source \"";
    error += srcArg;
    error += "\" which is not an existing directory.";
    this->SetError(error);
    return false;
  }
  srcPath = cmSystemTools::CollapseFullPath(srcPath);

  // Compute the full path to the binary directory.
  std::string binPath;
  if (binArg.empty()) {
    // No binary directory was specified.  If the source directory is
    // not a subdirectory of the current directory then it is an
    // error.
    if (!cmSystemTools::IsSubDirectory(
          srcPath, this->Makefile->GetCurrentSourceDirectory())) {
      std::ostringstream e;
      e << "not given a binary directory but the given source directory "
        << "\"" << srcPath << "\" is not a subdirectory of \""
        << this->Makefile->GetCurrentSourceDirectory() << "\".  "
        << "When specifying an out-of-tree source a binary directory "
        << "must be explicitly specified.";
      this->SetError(e.str());
      return false;
    }

    // Remove the CurrentDirectory from the srcPath and replace it
    // with the CurrentOutputDirectory.
    const char* src = this->Makefile->GetCurrentSourceDirectory();
    const char* bin = this->Makefile->GetCurrentBinaryDirectory();
    size_t srcLen = strlen(src);
    size_t binLen = strlen(bin);
    if (srcLen > 0 && src[srcLen - 1] == '/') {
      --srcLen;
    }
    if (binLen > 0 && bin[binLen - 1] == '/') {
      --binLen;
    }
    binPath = std::string(bin, binLen) + srcPath.substr(srcLen);
  } else {
    // Use the binary directory specified.
    // Interpret a relative path with respect to the current binary directory.
    if (cmSystemTools::FileIsFullPath(binArg.c_str())) {
      binPath = binArg;
    } else {
      binPath = this->Makefile->GetCurrentBinaryDirectory();
      binPath += "/";
      binPath += binArg;
    }
  }
  binPath = cmSystemTools::CollapseFullPath(binPath);

  // Add the subdirectory using the computed full paths.
  this->Makefile->AddSubDirectory(srcPath, binPath, excludeFromAll, true);

  return true;
}
