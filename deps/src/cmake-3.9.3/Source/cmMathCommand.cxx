/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMathCommand.h"

#include <stdio.h>

#include "cmExprParserHelper.h"
#include "cmMakefile.h"

class cmExecutionStatus;

bool cmMathCommand::InitialPass(std::vector<std::string> const& args,
                                cmExecutionStatus&)
{
  if (args.empty()) {
    this->SetError("must be called with at least one argument.");
    return false;
  }
  const std::string& subCommand = args[0];
  if (subCommand == "EXPR") {
    return this->HandleExprCommand(args);
  }
  std::string e = "does not recognize sub-command " + subCommand;
  this->SetError(e);
  return false;
}

bool cmMathCommand::HandleExprCommand(std::vector<std::string> const& args)
{
  if (args.size() != 3) {
    this->SetError("EXPR called with incorrect arguments.");
    return false;
  }

  const std::string& outputVariable = args[1];
  const std::string& expression = args[2];

  cmExprParserHelper helper;
  if (!helper.ParseString(expression.c_str(), 0)) {
    std::string e = "cannot parse the expression: \"" + expression + "\": ";
    e += helper.GetError();
    this->SetError(e);
    return false;
  }

  char buffer[1024];
  sprintf(buffer, "%d", helper.GetResult());

  this->Makefile->AddDefinition(outputVariable, buffer);
  return true;
}
