/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWriteFileCommand.h"

#include "cmsys/FStream.hxx"

#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cm_sys_stat.h"

class cmExecutionStatus;

// cmLibraryCommand
bool cmWriteFileCommand::InitialPass(std::vector<std::string> const& args,
                                     cmExecutionStatus&)
{
  if (args.size() < 2) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::string message;
  std::vector<std::string>::const_iterator i = args.begin();

  std::string const& fileName = *i;
  bool overwrite = true;
  i++;

  for (; i != args.end(); ++i) {
    if (*i == "APPEND") {
      overwrite = false;
    } else {
      message += *i;
    }
  }

  if (!this->Makefile->CanIWriteThisFile(fileName.c_str())) {
    std::string e =
      "attempted to write a file: " + fileName + " into a source directory.";
    this->SetError(e);
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }

  std::string dir = cmSystemTools::GetFilenamePath(fileName);
  cmSystemTools::MakeDirectory(dir.c_str());

  mode_t mode = 0;

  // Set permissions to writable
  if (cmSystemTools::GetPermissions(fileName.c_str(), mode)) {
    cmSystemTools::SetPermissions(fileName.c_str(),
#if defined(_MSC_VER) || defined(__MINGW32__)
                                  mode | S_IWRITE
#else
                                  mode | S_IWUSR | S_IWGRP
#endif
                                  );
  }
  // If GetPermissions fails, pretend like it is ok. File open will fail if
  // the file is not writable
  cmsys::ofstream file(fileName.c_str(),
                       overwrite ? std::ios::out : std::ios::app);
  if (!file) {
    std::string error = "Internal CMake error when trying to open file: ";
    error += fileName;
    error += " for writing.";
    this->SetError(error);
    return false;
  }
  file << message << std::endl;
  file.close();
  if (mode) {
    cmSystemTools::SetPermissions(fileName.c_str(), mode);
  }

  return true;
}
