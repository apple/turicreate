/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionParser_h
#define cmGeneratorExpressionParser_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <vector>

#include "cmGeneratorExpressionLexer.h"

struct cmGeneratorExpressionEvaluator;

struct cmGeneratorExpressionParser
{
  cmGeneratorExpressionParser(
    const std::vector<cmGeneratorExpressionToken>& tokens);

  void Parse(std::vector<cmGeneratorExpressionEvaluator*>& result);

private:
  void ParseContent(std::vector<cmGeneratorExpressionEvaluator*>&);
  void ParseGeneratorExpression(std::vector<cmGeneratorExpressionEvaluator*>&);

private:
  std::vector<cmGeneratorExpressionToken>::const_iterator it;
  const std::vector<cmGeneratorExpressionToken> Tokens;
  unsigned int NestingLevel;
};

#endif
