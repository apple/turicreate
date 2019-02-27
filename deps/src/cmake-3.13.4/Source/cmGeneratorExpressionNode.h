/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionNode_h
#define cmGeneratorExpressionNode_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

class cmGeneratorTarget;
class cmLocalGenerator;
struct GeneratorExpressionContent;
struct cmGeneratorExpressionContext;
struct cmGeneratorExpressionDAGChecker;

struct cmGeneratorExpressionNode
{
  enum
  {
    DynamicParameters = 0,
    OneOrMoreParameters = -1,
    OneOrZeroParameters = -2
  };
  virtual ~cmGeneratorExpressionNode() {}

  virtual bool GeneratesContent() const { return true; }

  virtual bool RequiresLiteralInput() const { return false; }

  virtual bool AcceptsArbitraryContentParameter() const { return false; }

  virtual int NumExpectedParameters() const { return 1; }

  virtual std::string Evaluate(
    const std::vector<std::string>& parameters,
    cmGeneratorExpressionContext* context,
    const GeneratorExpressionContent* content,
    cmGeneratorExpressionDAGChecker* dagChecker) const = 0;

  static std::string EvaluateDependentExpression(
    std::string const& prop, cmLocalGenerator* lg,
    cmGeneratorExpressionContext* context, const cmGeneratorTarget* headTarget,
    const cmGeneratorTarget* currentTarget,
    cmGeneratorExpressionDAGChecker* dagChecker);

  static const cmGeneratorExpressionNode* GetNode(
    const std::string& identifier);
};

void reportError(cmGeneratorExpressionContext* context,
                 const std::string& expr, const std::string& result);

#endif
