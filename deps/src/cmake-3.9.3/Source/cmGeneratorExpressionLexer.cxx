/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionLexer.h"

cmGeneratorExpressionLexer::cmGeneratorExpressionLexer()
  : SawBeginExpression(false)
  , SawGeneratorExpression(false)
{
}

static void InsertText(const char* upto, const char* c,
                       std::vector<cmGeneratorExpressionToken>& result)
{
  if (upto != c) {
    result.push_back(cmGeneratorExpressionToken(
      cmGeneratorExpressionToken::Text, upto, c - upto));
  }
}

std::vector<cmGeneratorExpressionToken> cmGeneratorExpressionLexer::Tokenize(
  const std::string& input)
{
  std::vector<cmGeneratorExpressionToken> result;

  const char* c = input.c_str();
  const char* upto = c;

  for (; *c; ++c) {
    switch (*c) {
      case '$':
        if (c[1] == '<') {
          InsertText(upto, c, result);
          result.push_back(cmGeneratorExpressionToken(
            cmGeneratorExpressionToken::BeginExpression, c, 2));
          upto = c + 2;
          ++c;
          SawBeginExpression = true;
        }
        break;
      case '>':
        InsertText(upto, c, result);
        result.push_back(cmGeneratorExpressionToken(
          cmGeneratorExpressionToken::EndExpression, c, 1));
        upto = c + 1;
        SawGeneratorExpression = SawBeginExpression;
        break;
      case ':':
        InsertText(upto, c, result);
        result.push_back(cmGeneratorExpressionToken(
          cmGeneratorExpressionToken::ColonSeparator, c, 1));
        upto = c + 1;
        break;
      case ',':
        InsertText(upto, c, result);
        result.push_back(cmGeneratorExpressionToken(
          cmGeneratorExpressionToken::CommaSeparator, c, 1));
        upto = c + 1;
        break;
      default:
        break;
    }
  }
  InsertText(upto, c, result);

  return result;
}
