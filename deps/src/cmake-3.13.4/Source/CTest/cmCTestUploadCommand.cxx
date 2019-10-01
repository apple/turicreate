/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestUploadCommand.h"

#include <sstream>
#include <vector>

#include "cmCTest.h"
#include "cmCTestGenericHandler.h"
#include "cmCTestUploadHandler.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"
#include "cmake.h"

cmCTestGenericHandler* cmCTestUploadCommand::InitializeHandler()
{
  cmCTestGenericHandler* handler =
    this->CTest->GetInitializedHandler("upload");
  if (!handler) {
    this->SetError("internal CTest error. Cannot instantiate upload handler");
    return nullptr;
  }
  static_cast<cmCTestUploadHandler*>(handler)->SetFiles(this->Files);

  handler->SetQuiet(this->Quiet);
  return handler;
}

bool cmCTestUploadCommand::CheckArgumentKeyword(std::string const& arg)
{
  if (arg == "FILES") {
    this->ArgumentDoing = ArgumentDoingFiles;
    return true;
  }
  if (arg == "QUIET") {
    this->ArgumentDoing = ArgumentDoingNone;
    this->Quiet = true;
    return true;
  }
  if (arg == "CAPTURE_CMAKE_ERROR") {
    this->ArgumentDoing = ArgumentDoingCaptureCMakeError;
    return true;
  }
  return false;
}

bool cmCTestUploadCommand::CheckArgumentValue(std::string const& arg)
{
  if (this->ArgumentDoing == ArgumentDoingCaptureCMakeError) {
    this->Values[ct_CAPTURE_CMAKE_ERROR] = arg.c_str();
    return true;
  }
  if (this->ArgumentDoing == ArgumentDoingFiles) {
    if (cmSystemTools::FileExists(arg)) {
      this->Files.insert(arg);
      return true;
    }
    std::ostringstream e;
    e << "File \"" << arg << "\" does not exist. Cannot submit "
      << "a non-existent file.";
    this->Makefile->IssueMessage(cmake::FATAL_ERROR, e.str());
    this->ArgumentDoing = ArgumentDoingError;
    return false;
  }

  // Look for other arguments.
  return this->Superclass::CheckArgumentValue(arg);
}
