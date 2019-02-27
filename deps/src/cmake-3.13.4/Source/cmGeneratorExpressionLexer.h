/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionLexer_h
#define cmGeneratorExpressionLexer_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <stddef.h>
#include <string>
#include <vector>

struct cmGeneratorExpressionToken
{
  cmGeneratorExpressionToken(unsigned type, const char* c, size_t l)
    : TokenType(type)
    , Content(c)
    , Length(l)
  {
  }
  enum
  {
    Text,
    BeginExpression,
    EndExpression,
    ColonSeparator,
    CommaSeparator
  };
  unsigned TokenType;
  const char* Content;
  size_t Length;
};

/** \class cmGeneratorExpressionLexer
 *
 */
class cmGeneratorExpressionLexer
{
public:
  cmGeneratorExpressionLexer();

  std::vector<cmGeneratorExpressionToken> Tokenize(const std::string& input);

  bool GetSawGeneratorExpression() const
  {
    return this->SawGeneratorExpression;
  }

private:
  bool SawBeginExpression;
  bool SawGeneratorExpression;
};

#endif
