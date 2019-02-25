/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExprParserHelper.h"

#include "cmExprLexer.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

int cmExpr_yyparse(yyscan_t yyscanner);
//
cmExprParserHelper::cmExprParserHelper()
{
  this->FileLine = -1;
  this->FileName = nullptr;
  this->Result = 0;
}

cmExprParserHelper::~cmExprParserHelper()
{
}

int cmExprParserHelper::ParseString(const char* str, int verb)
{
  if (!str) {
    return 0;
  }
  // printf("Do some parsing: %s\n", str);

  this->Verbose = verb;
  this->InputBuffer = str;
  this->InputBufferPos = 0;
  this->CurrentLine = 0;

  this->Result = 0;

  yyscan_t yyscanner;
  cmExpr_yylex_init(&yyscanner);
  cmExpr_yyset_extra(this, yyscanner);

  try {
    int res = cmExpr_yyparse(yyscanner);
    if (res != 0) {
      std::string e = "cannot parse the expression: \"" + InputBuffer + "\": ";
      e += ErrorString;
      e += ".";
      this->SetError(std::move(e));
    }
  } catch (std::runtime_error const& fail) {
    std::string e =
      "cannot evaluate the expression: \"" + InputBuffer + "\": ";
    e += fail.what();
    e += ".";
    this->SetError(std::move(e));
  } catch (std::out_of_range const&) {
    std::string e = "cannot evaluate the expression: \"" + InputBuffer +
      "\": a numeric value is out of range.";
    this->SetError(std::move(e));
  } catch (...) {
    std::string e = "cannot parse the expression: \"" + InputBuffer + "\".";
    this->SetError(std::move(e));
  }
  cmExpr_yylex_destroy(yyscanner);
  if (!this->ErrorString.empty()) {
    return 0;
  }

  if (Verbose) {
    std::cerr << "Expanding [" << str << "] produced: [" << this->Result << "]"
              << std::endl;
  }
  return 1;
}

int cmExprParserHelper::LexInput(char* buf, int maxlen)
{
  // std::cout << "JPLexInput ";
  // std::cout.write(buf, maxlen);
  // std::cout << std::endl;
  if (maxlen < 1) {
    return 0;
  }
  if (this->InputBufferPos < this->InputBuffer.size()) {
    buf[0] = this->InputBuffer[this->InputBufferPos++];
    if (buf[0] == '\n') {
      this->CurrentLine++;
    }
    return (1);
  }
  buf[0] = '\n';
  return (0);
}

void cmExprParserHelper::Error(const char* str)
{
  unsigned long pos = static_cast<unsigned long>(this->InputBufferPos);
  std::ostringstream ostr;
  ostr << str << " (" << pos << ")";
  this->ErrorString = ostr.str();
}

void cmExprParserHelper::UnexpectedChar(char c)
{
  unsigned long pos = static_cast<unsigned long>(this->InputBufferPos);
  std::ostringstream ostr;
  ostr << "Unexpected character in expression at position " << pos << ": " << c
       << "\n";
  this->WarningString += ostr.str();
}

void cmExprParserHelper::SetResult(KWIML_INT_int64_t value)
{
  this->Result = value;
}

void cmExprParserHelper::SetError(std::string errorString)
{
  this->ErrorString = std::move(errorString);
}
