/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmIncludeRegularExpressionCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// cmIncludeRegularExpressionCommand
bool cmIncludeRegularExpressionCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  if ((args.empty()) || (args.size() > 2)) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }
  this->Makefile->SetIncludeRegularExpression(args[0].c_str());

  if (args.size() > 1) {
    this->Makefile->SetComplainRegularExpression(args[1]);
  }

  return true;
}
