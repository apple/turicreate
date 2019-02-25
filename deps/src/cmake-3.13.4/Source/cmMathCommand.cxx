/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmMathCommand.h"

#include "cmExprParserHelper.h"
#include "cmMakefile.h"
#include "cm_kwiml.h"
#include "cmake.h"

#include <stdio.h>

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
  if ((args.size() != 3) && (args.size() != 5)) {
    this->SetError("EXPR called with incorrect arguments.");
    return false;
  }

  enum class NumericFormat
  {
    UNINITIALIZED,
    DECIMAL,
    HEXADECIMAL,
  };

  const std::string& outputVariable = args[1];
  const std::string& expression = args[2];
  size_t argumentIndex = 3;
  NumericFormat outputFormat = NumericFormat::UNINITIALIZED;

  this->Makefile->AddDefinition(outputVariable, "ERROR");

  if (argumentIndex < args.size()) {
    const std::string messageHint = "sub-command EXPR ";
    const std::string option = args[argumentIndex++];
    if (option == "OUTPUT_FORMAT") {
      if (argumentIndex < args.size()) {
        const std::string argument = args[argumentIndex++];
        if (argument == "DECIMAL") {
          outputFormat = NumericFormat::DECIMAL;
        } else if (argument == "HEXADECIMAL") {
          outputFormat = NumericFormat::HEXADECIMAL;
        } else {
          std::string error = messageHint + "value \"" + argument +
            "\" for option \"" + option + "\" is invalid.";
          this->SetError(error);
          return false;
        }
      } else {
        std::string error =
          messageHint + "missing argument for option \"" + option + "\".";
        this->SetError(error);
        return false;
      }
    } else {
      std::string error =
        messageHint + "option \"" + option + "\" is unknown.";
      this->SetError(error);
      return false;
    }
  }

  if (outputFormat == NumericFormat::UNINITIALIZED) {
    outputFormat = NumericFormat::DECIMAL;
  }

  cmExprParserHelper helper;
  if (!helper.ParseString(expression.c_str(), 0)) {
    this->SetError(helper.GetError());
    return false;
  }

  char buffer[1024];
  const char* fmt;
  switch (outputFormat) {
    case NumericFormat::HEXADECIMAL:
      fmt = "0x%" KWIML_INT_PRIx64;
      break;
    case NumericFormat::DECIMAL:
      CM_FALLTHROUGH;
    default:
      fmt = "%" KWIML_INT_PRId64;
      break;
  }
  sprintf(buffer, fmt, helper.GetResult());

  std::string const& w = helper.GetWarning();
  if (!w.empty()) {
    this->Makefile->IssueMessage(cmake::AUTHOR_WARNING, w);
  }

  this->Makefile->AddDefinition(outputVariable, buffer);
  return true;
}
