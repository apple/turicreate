/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmEnableLanguageCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// cmEnableLanguageCommand
bool cmEnableLanguageCommand::InitialPass(std::vector<std::string> const& args,
                                          cmExecutionStatus&)
{
  bool optional = false;
  std::vector<std::string> languages;
  if (args.empty()) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  for (std::vector<std::string>::const_iterator it = args.begin();
       it != args.end(); ++it) {
    if ((*it) == "OPTIONAL") {
      optional = true;
    } else {
      languages.push_back(*it);
    }
  }

  this->Makefile->EnableLanguage(languages, optional);
  return true;
}
