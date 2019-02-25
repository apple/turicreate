/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestCoverageCommand.h"

#include "cmCTest.h"
#include "cmCTestCoverageHandler.h"

class cmCTestGenericHandler;

cmCTestCoverageCommand::cmCTestCoverageCommand()
{
  this->LabelsMentioned = false;
}

cmCTestGenericHandler* cmCTestCoverageCommand::InitializeHandler()
{
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "CoverageCommand", "CTEST_COVERAGE_COMMAND", this->Quiet);
  this->CTest->SetCTestConfigurationFromCMakeVariable(
    this->Makefile, "CoverageExtraFlags", "CTEST_COVERAGE_EXTRA_FLAGS",
    this->Quiet);
  cmCTestCoverageHandler* handler = static_cast<cmCTestCoverageHandler*>(
    this->CTest->GetInitializedHandler("coverage"));
  if (!handler) {
    this->SetError("internal CTest error. Cannot instantiate test handler");
    return nullptr;
  }

  // If a LABELS option was given, select only files with the labels.
  if (this->LabelsMentioned) {
    handler->SetLabelFilter(this->Labels);
  }

  handler->SetQuiet(this->Quiet);
  return handler;
}

bool cmCTestCoverageCommand::CheckArgumentKeyword(std::string const& arg)
{
  // Look for arguments specific to this command.
  if (arg == "LABELS") {
    this->ArgumentDoing = ArgumentDoingLabels;
    this->LabelsMentioned = true;
    return true;
  }

  // Look for other arguments.
  return this->Superclass::CheckArgumentKeyword(arg);
}

bool cmCTestCoverageCommand::CheckArgumentValue(std::string const& arg)
{
  // Handle states specific to this command.
  if (this->ArgumentDoing == ArgumentDoingLabels) {
    this->Labels.insert(arg);
    return true;
  }

  // Look for other arguments.
  return this->Superclass::CheckArgumentValue(arg);
}
