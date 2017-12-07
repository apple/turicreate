/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionParser.h"

#include "cmGeneratorExpressionEvaluator.h"

#include <assert.h>
#include <stddef.h>

cmGeneratorExpressionParser::cmGeneratorExpressionParser(
  const std::vector<cmGeneratorExpressionToken>& tokens)
  : Tokens(tokens)
  , NestingLevel(0)
{
}

void cmGeneratorExpressionParser::Parse(
  std::vector<cmGeneratorExpressionEvaluator*>& result)
{
  it = this->Tokens.begin();

  while (this->it != this->Tokens.end()) {
    this->ParseContent(result);
  }
}

static void extendText(
  std::vector<cmGeneratorExpressionEvaluator*>& result,
  std::vector<cmGeneratorExpressionToken>::const_iterator it)
{
  if (!result.empty() &&
      (*(result.end() - 1))->GetType() ==
        cmGeneratorExpressionEvaluator::Text) {
    TextContent* textContent = static_cast<TextContent*>(*(result.end() - 1));
    textContent->Extend(it->Length);
  } else {
    TextContent* textContent = new TextContent(it->Content, it->Length);
    result.push_back(textContent);
  }
}

static void extendResult(
  std::vector<cmGeneratorExpressionEvaluator*>& result,
  const std::vector<cmGeneratorExpressionEvaluator*>& contents)
{
  if (!result.empty() &&
      (*(result.end() - 1))->GetType() ==
        cmGeneratorExpressionEvaluator::Text &&
      (*contents.begin())->GetType() == cmGeneratorExpressionEvaluator::Text) {
    TextContent* textContent = static_cast<TextContent*>(*(result.end() - 1));
    textContent->Extend(
      static_cast<TextContent*>(*contents.begin())->GetLength());
    delete *contents.begin();
    result.insert(result.end(), contents.begin() + 1, contents.end());
  } else {
    result.insert(result.end(), contents.begin(), contents.end());
  }
}

void cmGeneratorExpressionParser::ParseGeneratorExpression(
  std::vector<cmGeneratorExpressionEvaluator*>& result)
{
  assert(this->it != this->Tokens.end());
  unsigned int nestedLevel = this->NestingLevel;
  ++this->NestingLevel;

  std::vector<cmGeneratorExpressionToken>::const_iterator startToken =
    this->it - 1;

  std::vector<cmGeneratorExpressionEvaluator*> identifier;
  while (this->it->TokenType != cmGeneratorExpressionToken::EndExpression &&
         this->it->TokenType != cmGeneratorExpressionToken::ColonSeparator) {
    if (this->it->TokenType == cmGeneratorExpressionToken::CommaSeparator) {
      extendText(identifier, this->it);
      ++this->it;
    } else {
      this->ParseContent(identifier);
    }
    if (this->it == this->Tokens.end()) {
      break;
    }
  }
  if (identifier.empty()) {
    // ERROR
  }

  if (this->it != this->Tokens.end() &&
      this->it->TokenType == cmGeneratorExpressionToken::EndExpression) {
    GeneratorExpressionContent* content =
      new GeneratorExpressionContent(startToken->Content, this->it->Content -
                                       startToken->Content + this->it->Length);
    assert(this->it != this->Tokens.end());
    ++this->it;
    --this->NestingLevel;
    content->SetIdentifier(identifier);
    result.push_back(content);
    return;
  }

  std::vector<std::vector<cmGeneratorExpressionEvaluator*> > parameters;
  std::vector<std::vector<cmGeneratorExpressionToken>::const_iterator>
    commaTokens;
  std::vector<cmGeneratorExpressionToken>::const_iterator colonToken;

  bool emptyParamTermination = false;

  if (this->it != this->Tokens.end() &&
      this->it->TokenType == cmGeneratorExpressionToken::ColonSeparator) {
    colonToken = this->it;
    parameters.resize(parameters.size() + 1);
    assert(this->it != this->Tokens.end());
    ++this->it;
    if (this->it == this->Tokens.end()) {
      emptyParamTermination = true;
    }

    while (this->it != this->Tokens.end() &&
           this->it->TokenType == cmGeneratorExpressionToken::CommaSeparator) {
      commaTokens.push_back(this->it);
      parameters.resize(parameters.size() + 1);
      assert(this->it != this->Tokens.end());
      ++this->it;
      if (this->it == this->Tokens.end()) {
        emptyParamTermination = true;
      }
    }
    while (this->it != this->Tokens.end() &&
           this->it->TokenType == cmGeneratorExpressionToken::ColonSeparator) {
      extendText(*(parameters.end() - 1), this->it);
      assert(this->it != this->Tokens.end());
      ++this->it;
    }
    while (this->it != this->Tokens.end() &&
           this->it->TokenType != cmGeneratorExpressionToken::EndExpression) {
      this->ParseContent(*(parameters.end() - 1));
      if (this->it == this->Tokens.end()) {
        break;
      }
      while (this->it != this->Tokens.end() &&
             this->it->TokenType ==
               cmGeneratorExpressionToken::CommaSeparator) {
        commaTokens.push_back(this->it);
        parameters.resize(parameters.size() + 1);
        assert(this->it != this->Tokens.end());
        ++this->it;
        if (this->it == this->Tokens.end()) {
          emptyParamTermination = true;
        }
      }
      while (this->it != this->Tokens.end() &&
             this->it->TokenType ==
               cmGeneratorExpressionToken::ColonSeparator) {
        extendText(*(parameters.end() - 1), this->it);
        assert(this->it != this->Tokens.end());
        ++this->it;
      }
    }
    if (this->it != this->Tokens.end() &&
        this->it->TokenType == cmGeneratorExpressionToken::EndExpression) {
      --this->NestingLevel;
      assert(this->it != this->Tokens.end());
      ++this->it;
    }
  }

  if (nestedLevel != this->NestingLevel) {
    // There was a '$<' in the text, but no corresponding '>'. Rebuild to
    // treat the '$<' as having been plain text, along with the
    // corresponding : and , tokens that might have been found.
    extendText(result, startToken);
    extendResult(result, identifier);
    if (!parameters.empty()) {
      extendText(result, colonToken);

      typedef std::vector<cmGeneratorExpressionEvaluator*> EvaluatorVector;
      typedef std::vector<cmGeneratorExpressionToken> TokenVector;
      std::vector<EvaluatorVector>::const_iterator pit = parameters.begin();
      const std::vector<EvaluatorVector>::const_iterator pend =
        parameters.end();
      std::vector<TokenVector::const_iterator>::const_iterator commaIt =
        commaTokens.begin();
      assert(parameters.size() > commaTokens.size());
      for (; pit != pend; ++pit, ++commaIt) {
        if (!pit->empty() && !emptyParamTermination) {
          extendResult(result, *pit);
        }
        if (commaIt != commaTokens.end()) {
          extendText(result, *commaIt);
        } else {
          break;
        }
      }
    }
    return;
  }

  size_t contentLength =
    ((this->it - 1)->Content - startToken->Content) + (this->it - 1)->Length;
  GeneratorExpressionContent* content =
    new GeneratorExpressionContent(startToken->Content, contentLength);
  content->SetIdentifier(identifier);
  content->SetParameters(parameters);
  result.push_back(content);
}

void cmGeneratorExpressionParser::ParseContent(
  std::vector<cmGeneratorExpressionEvaluator*>& result)
{
  assert(this->it != this->Tokens.end());
  switch (this->it->TokenType) {
    case cmGeneratorExpressionToken::Text: {
      if (this->NestingLevel == 0) {
        if (!result.empty() &&
            (*(result.end() - 1))->GetType() ==
              cmGeneratorExpressionEvaluator::Text) {
          // A comma in 'plain text' could have split text that should
          // otherwise be continuous. Extend the last text content instead of
          // creating a new one.
          TextContent* textContent =
            static_cast<TextContent*>(*(result.end() - 1));
          textContent->Extend(this->it->Length);
          assert(this->it != this->Tokens.end());
          ++this->it;
          return;
        }
      }
      cmGeneratorExpressionEvaluator* n =
        new TextContent(this->it->Content, this->it->Length);
      result.push_back(n);
      assert(this->it != this->Tokens.end());
      ++this->it;
      return;
    }
    case cmGeneratorExpressionToken::BeginExpression:
      assert(this->it != this->Tokens.end());
      ++this->it;
      this->ParseGeneratorExpression(result);
      return;
    case cmGeneratorExpressionToken::EndExpression:
    case cmGeneratorExpressionToken::ColonSeparator:
    case cmGeneratorExpressionToken::CommaSeparator:
      if (this->NestingLevel == 0) {
        extendText(result, this->it);
      } else {
        assert(false && "Got unexpected syntax token.");
      }
      assert(this->it != this->Tokens.end());
      ++this->it;
      return;
  }
  assert(false && "Unhandled token in generator expression.");
}
