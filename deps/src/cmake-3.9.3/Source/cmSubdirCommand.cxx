/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSubdirCommand.h"

#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmSubdirCommand
bool cmSubdirCommand::InitialPass(std::vector<std::string> const& args,
                                  cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  bool res = true;
  bool excludeFromAll = false;

  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    if (*i == "EXCLUDE_FROM_ALL") {
      excludeFromAll = true;
      continue;
    }
    if (*i == "PREORDER") {
      // Ignored
      continue;
    }

    // if they specified a relative path then compute the full
    std::string srcPath =
      std::string(this->Makefile->GetCurrentSourceDirectory()) + "/" + *i;
    if (cmSystemTools::FileIsDirectory(srcPath)) {
      std::string binPath =
        std::string(this->Makefile->GetCurrentBinaryDirectory()) + "/" + *i;
      this->Makefile->AddSubDirectory(srcPath, binPath, excludeFromAll, false);
    }
    // otherwise it is a full path
    else if (cmSystemTools::FileIsDirectory(*i)) {
      // we must compute the binPath from the srcPath, we just take the last
      // element from the source path and use that
      std::string binPath =
        std::string(this->Makefile->GetCurrentBinaryDirectory()) + "/" +
        cmSystemTools::GetFilenameName(*i);
      this->Makefile->AddSubDirectory(*i, binPath, excludeFromAll, false);
    } else {
      std::string error = "Incorrect SUBDIRS command. Directory: ";
      error += *i + " does not exist.";
      this->SetError(error);
      res = false;
    }
  }
  return res;
}
