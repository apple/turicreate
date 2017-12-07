/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionEvaluator.h"

#include "cmAlgorithms.h"
#include "cmGeneratorExpressionContext.h"
#include "cmGeneratorExpressionNode.h"

#include <algorithm>
#include <sstream>

GeneratorExpressionContent::GeneratorExpressionContent(
  const char* startContent, size_t length)
  : StartContent(startContent)
  , ContentLength(length)
{
}

std::string GeneratorExpressionContent::GetOriginalExpression() const
{
  return std::string(this->StartContent, this->ContentLength);
}

std::string GeneratorExpressionContent::ProcessArbitraryContent(
  const cmGeneratorExpressionNode* node, const std::string& identifier,
  cmGeneratorExpressionContext* context,
  cmGeneratorExpressionDAGChecker* dagChecker,
  std::vector<std::vector<cmGeneratorExpressionEvaluator*> >::const_iterator
    pit) const
{
  std::string result;

  const std::vector<
    std::vector<cmGeneratorExpressionEvaluator*> >::const_iterator pend =
    this->ParamChildren.end();
  for (; pit != pend; ++pit) {
    std::vector<cmGeneratorExpressionEvaluator*>::const_iterator it =
      pit->begin();
    const std::vector<cmGeneratorExpressionEvaluator*>::const_iterator end =
      pit->end();
    for (; it != end; ++it) {
      if (node->RequiresLiteralInput()) {
        if ((*it)->GetType() != cmGeneratorExpressionEvaluator::Text) {
          reportError(context, this->GetOriginalExpression(), "$<" +
                        identifier + "> expression requires literal input.");
          return std::string();
        }
      }
      result += (*it)->Evaluate(context, dagChecker);
      if (context->HadError) {
        return std::string();
      }
    }
    if ((pit + 1) != pend) {
      result += ",";
    }
  }
  if (node->RequiresLiteralInput()) {
    std::vector<std::string> parameters;
    parameters.push_back(result);
    return node->Evaluate(parameters, context, this, dagChecker);
  }
  return result;
}

std::string GeneratorExpressionContent::Evaluate(
  cmGeneratorExpressionContext* context,
  cmGeneratorExpressionDAGChecker* dagChecker) const
{
  std::string identifier;
  {
    std::vector<cmGeneratorExpressionEvaluator*>::const_iterator it =
      this->IdentifierChildren.begin();
    const std::vector<cmGeneratorExpressionEvaluator*>::const_iterator end =
      this->IdentifierChildren.end();
    for (; it != end; ++it) {
      identifier += (*it)->Evaluate(context, dagChecker);
      if (context->HadError) {
        return std::string();
      }
    }
  }

  const cmGeneratorExpressionNode* node =
    cmGeneratorExpressionNode::GetNode(identifier);

  if (!node) {
    reportError(context, this->GetOriginalExpression(),
                "Expression did not evaluate to a known generator expression");
    return std::string();
  }

  if (!node->GeneratesContent()) {
    if (node->NumExpectedParameters() == 1 &&
        node->AcceptsArbitraryContentParameter()) {
      if (this->ParamChildren.empty()) {
        reportError(context, this->GetOriginalExpression(),
                    "$<" + identifier + "> expression requires a parameter.");
      }
    } else {
      std::vector<std::string> parameters;
      this->EvaluateParameters(node, identifier, context, dagChecker,
                               parameters);
    }
    return std::string();
  }

  std::vector<std::string> parameters;
  this->EvaluateParameters(node, identifier, context, dagChecker, parameters);
  if (context->HadError) {
    return std::string();
  }

  return node->Evaluate(parameters, context, this, dagChecker);
}

std::string GeneratorExpressionContent::EvaluateParameters(
  const cmGeneratorExpressionNode* node, const std::string& identifier,
  cmGeneratorExpressionContext* context,
  cmGeneratorExpressionDAGChecker* dagChecker,
  std::vector<std::string>& parameters) const
{
  const int numExpected = node->NumExpectedParameters();
  {
    std::vector<std::vector<cmGeneratorExpressionEvaluator*> >::const_iterator
      pit = this->ParamChildren.begin();
    const std::vector<
      std::vector<cmGeneratorExpressionEvaluator*> >::const_iterator pend =
      this->ParamChildren.end();
    const bool acceptsArbitraryContent =
      node->AcceptsArbitraryContentParameter();
    int counter = 1;
    for (; pit != pend; ++pit, ++counter) {
      if (acceptsArbitraryContent && counter == numExpected) {
        std::string lastParam = this->ProcessArbitraryContent(
          node, identifier, context, dagChecker, pit);
        parameters.push_back(lastParam);
        return std::string();
      }
      std::string parameter;
      std::vector<cmGeneratorExpressionEvaluator*>::const_iterator it =
        pit->begin();
      const std::vector<cmGeneratorExpressionEvaluator*>::const_iterator end =
        pit->end();
      for (; it != end; ++it) {
        parameter += (*it)->Evaluate(context, dagChecker);
        if (context->HadError) {
          return std::string();
        }
      }
      parameters.push_back(parameter);
    }
  }

  if ((numExpected > cmGeneratorExpressionNode::DynamicParameters &&
       (unsigned int)numExpected != parameters.size())) {
    if (numExpected == 0) {
      reportError(context, this->GetOriginalExpression(),
                  "$<" + identifier + "> expression requires no parameters.");
    } else if (numExpected == 1) {
      reportError(context, this->GetOriginalExpression(), "$<" + identifier +
                    "> expression requires "
                    "exactly one parameter.");
    } else {
      std::ostringstream e;
      e << "$<" + identifier + "> expression requires " << numExpected
        << " comma separated parameters, but got " << parameters.size()
        << " instead.";
      reportError(context, this->GetOriginalExpression(), e.str());
    }
    return std::string();
  }

  if (numExpected == cmGeneratorExpressionNode::OneOrMoreParameters &&
      parameters.empty()) {
    reportError(context, this->GetOriginalExpression(), "$<" + identifier +
                  "> expression requires at least one parameter.");
  }
  if (numExpected == cmGeneratorExpressionNode::OneOrZeroParameters &&
      parameters.size() > 1) {
    reportError(context, this->GetOriginalExpression(), "$<" + identifier +
                  "> expression requires one or zero parameters.");
  }
  return std::string();
}

GeneratorExpressionContent::~GeneratorExpressionContent()
{
  cmDeleteAll(this->IdentifierChildren);
  std::for_each(this->ParamChildren.begin(), this->ParamChildren.end(),
                cmDeleteAll<std::vector<cmGeneratorExpressionEvaluator*> >);
}
