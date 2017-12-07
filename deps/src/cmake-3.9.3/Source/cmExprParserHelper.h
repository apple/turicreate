/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExprParserHelper_h
#define cmExprParserHelper_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

class cmExprParserHelper
{
public:
  struct ParserType
  {
    int Number;
  };

  cmExprParserHelper();
  ~cmExprParserHelper();

  int ParseString(const char* str, int verb);

  int LexInput(char* buf, int maxlen);
  void Error(const char* str);

  void SetResult(int value);

  int GetResult() { return this->Result; }

  const char* GetError() { return this->ErrorString.c_str(); }

private:
  std::string::size_type InputBufferPos;
  std::string InputBuffer;
  std::vector<char> OutputBuffer;
  int CurrentLine;
  int Verbose;

  void Print(const char* place, const char* str);

  void CleanupParser();

  int Result;
  const char* FileName;
  long FileLine;
  std::string ErrorString;
};

#define YYSTYPE cmExprParserHelper::ParserType
#define YYSTYPE_IS_DECLARED
#define YY_EXTRA_TYPE cmExprParserHelper*
#define YY_DECL int cmExpr_yylex(YYSTYPE* yylvalp, yyscan_t yyscanner)

#endif
