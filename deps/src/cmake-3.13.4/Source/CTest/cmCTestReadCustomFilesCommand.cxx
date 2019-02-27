/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestReadCustomFilesCommand.h"

#include "cmCTest.h"

class cmExecutionStatus;

bool cmCTestReadCustomFilesCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus& /*unused*/)
{
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  for (std::string const& arg : args) {
    this->CTest->ReadCustomConfigurationFileTree(arg.c_str(), this->Makefile);
  }

  return true;
}
