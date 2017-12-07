/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMakeDirectoryCommand.h"

#include "cmMakefile.h"
#include "cmSystemTools.h"

class cmExecutionStatus;

// cmMakeDirectoryCommand
bool cmMakeDirectoryCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  if (args.size() != 1) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  if (!this->Makefile->CanIWriteThisFile(args[0].c_str())) {
    std::string e = "attempted to create a directory: " + args[0] +
      " into a source directory.";
    this->SetError(e);
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
  cmSystemTools::MakeDirectory(args[0].c_str());
  return true;
}
