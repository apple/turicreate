/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGetSourceFilePropertyCommand.h"

#include "cmMakefile.h"
#include "cmSourceFile.h"

class cmExecutionStatus;

// cmSetSourceFilePropertyCommand
bool cmGetSourceFilePropertyCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if (args.size() != 3) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  std::string const& var = args[0];
  std::string const& file = args[1];
  cmSourceFile* sf = this->Makefile->GetSource(file);

  // for the location we must create a source file first
  if (!sf && args[2] == "LOCATION") {
    sf = this->Makefile->CreateSource(file);
  }
  if (sf) {
    if (args[2] == "LANGUAGE") {
      this->Makefile->AddDefinition(var, sf->GetLanguage().c_str());
      return true;
    }
    const char* prop = nullptr;
    if (!args[2].empty()) {
      prop = sf->GetPropertyForUser(args[2]);
    }
    if (prop) {
      this->Makefile->AddDefinition(var, prop);
      return true;
    }
  }

  this->Makefile->AddDefinition(var, "NOTFOUND");
  return true;
}
