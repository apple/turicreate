/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionEvaluator_h
#define cmGeneratorExpressionEvaluator_h

#include "cmConfigure.h"

#include <stddef.h>
#include <string>
#include <vector>

struct cmGeneratorExpressionContext;
struct cmGeneratorExpressionDAGChecker;
struct cmGeneratorExpressionNode;

struct cmGeneratorExpressionEvaluator
{
  cmGeneratorExpressionEvaluator() {}
  virtual ~cmGeneratorExpressionEvaluator() {}

  enum Type
  {
    Text,
    Generator
  };

  virtual Type GetType() const = 0;

  virtual std::string Evaluate(cmGeneratorExpressionContext* context,
                               cmGeneratorExpressionDAGChecker*) const = 0;

private:
  CM_DISABLE_COPY(cmGeneratorExpressionEvaluator)
};

struct TextContent : public cmGeneratorExpressionEvaluator
{
  TextContent(const char* start, size_t length)
    : Content(start)
    , Length(length)
  {
  }

  std::string Evaluate(cmGeneratorExpressionContext*,
                       cmGeneratorExpressionDAGChecker*) const CM_OVERRIDE
  {
    return std::string(this->Content, this->Length);
  }

  Type GetType() const CM_OVERRIDE
  {
    return cmGeneratorExpressionEvaluator::Text;
  }

  void Extend(size_t length) { this->Length += length; }

  size_t GetLength() { return this->Length; }

private:
  const char* Content;
  size_t Length;
};

struct GeneratorExpressionContent : public cmGeneratorExpressionEvaluator
{
  GeneratorExpressionContent(const char* startContent, size_t length);
  void SetIdentifier(
    std::vector<cmGeneratorExpressionEvaluator*> const& identifier)
  {
    this->IdentifierChildren = identifier;
  }

  void SetParameters(
    std::vector<std::vector<cmGeneratorExpressionEvaluator*> > const&
      parameters)
  {
    this->ParamChildren = parameters;
  }

  Type GetType() const CM_OVERRIDE
  {
    return cmGeneratorExpressionEvaluator::Generator;
  }

  std::string Evaluate(cmGeneratorExpressionContext* context,
                       cmGeneratorExpressionDAGChecker*) const CM_OVERRIDE;

  std::string GetOriginalExpression() const;

  ~GeneratorExpressionContent() CM_OVERRIDE;

private:
  std::string EvaluateParameters(const cmGeneratorExpressionNode* node,
                                 const std::string& identifier,
                                 cmGeneratorExpressionContext* context,
                                 cmGeneratorExpressionDAGChecker* dagChecker,
                                 std::vector<std::string>& parameters) const;

  std::string ProcessArbitraryContent(
    const cmGeneratorExpressionNode* node, const std::string& identifier,
    cmGeneratorExpressionContext* context,
    cmGeneratorExpressionDAGChecker* dagChecker,
    std::vector<std::vector<cmGeneratorExpressionEvaluator*> >::const_iterator
      pit) const;

private:
  std::vector<cmGeneratorExpressionEvaluator*> IdentifierChildren;
  std::vector<std::vector<cmGeneratorExpressionEvaluator*> > ParamChildren;
  const char* StartContent;
  size_t ContentLength;
};

#endif
