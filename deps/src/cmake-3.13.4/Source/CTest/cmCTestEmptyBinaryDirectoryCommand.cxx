/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCTestEmptyBinaryDirectoryCommand.h"

#include "cmCTestScriptHandler.h"

#include <sstream>

class cmExecutionStatus;

bool cmCTestEmptyBinaryDirectoryCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus& /*unused*/)
{
  if (args.size() != 1) {
    this->SetError("called with incorrect number of arguments");
    return false;
  }

  if (!cmCTestScriptHandler::EmptyBinaryDirectory(args[0].c_str())) {
    std::ostringstream ostr;
    ostr << "problem removing the binary directory: " << args[0];
    this->SetError(ostr.str());
    return false;
  }

  return true;
}
