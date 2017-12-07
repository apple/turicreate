/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLinkLibrariesCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// cmLinkLibrariesCommand
bool cmLinkLibrariesCommand::InitialPass(std::vector<std::string> const& args,
                                         cmExecutionStatus&)
{
  if (args.empty()) {
    return true;
  }
  // add libraries, nothe that there is an optional prefix
  // of debug and optimized than can be used
  for (std::vector<std::string>::const_iterator i = args.begin();
       i != args.end(); ++i) {
    if (*i == "debug") {
      ++i;
      if (i == args.end()) {
        this->SetError("The \"debug\" argument must be followed by "
                       "a library");
        return false;
      }
      this->Makefile->AppendProperty("LINK_LIBRARIES", "debug");
    } else if (*i == "optimized") {
      ++i;
      if (i == args.end()) {
        this->SetError("The \"optimized\" argument must be followed by "
                       "a library");
        return false;
      }
      this->Makefile->AppendProperty("LINK_LIBRARIES", "optimized");
    }
    this->Makefile->AppendProperty("LINK_LIBRARIES", i->c_str());
  }

  return true;
}
