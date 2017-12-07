/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGetTestPropertyCommand.h"

#include "cmMakefile.h"
#include "cmTest.h"

class cmExecutionStatus;

// cmGetTestPropertyCommand
bool cmGetTestPropertyCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() < 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  std::string const& testName = args[0];
  std::string const& var = args[2];
  cmTest* test = this->Makefile->GetTest(testName);
  if (test) {
    const char* prop = CM_NULLPTR;
    if (!args[1].empty()) {
      prop = test->GetProperty(args[1]);
    }
    if (prop) {
      this->Makefile->AddDefinition(var, prop);
      return true;
    }
  }
  this->Makefile->AddDefinition(var, "NOTFOUND");
  return true;
}
