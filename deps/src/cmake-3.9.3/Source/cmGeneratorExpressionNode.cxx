/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionNode.h"

#include "cmAlgorithms.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorExpressionContext.h"
#include "cmGeneratorExpressionDAGChecker.h"
#include "cmGeneratorExpressionEvaluator.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLinkItem.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

#include "cmConfigure.h"
#include "cmsys/RegularExpression.hxx"
#include "cmsys/String.h"
#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <map>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <utility>

std::string cmGeneratorExpressionNode::EvaluateDependentExpression(
  std::string const& prop, cmLocalGenerator* lg,
  cmGeneratorExpressionContext* context, cmGeneratorTarget const* headTarget,
  cmGeneratorTarget const* currentTarget,
  cmGeneratorExpressionDAGChecker* dagChecker)
{
  cmGeneratorExpression ge(context->Backtrace);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(prop);
  cge->SetEvaluateForBuildsystem(context->EvaluateForBuildsystem);
  std::string result =
    cge->Evaluate(lg, context->Config, context->Quiet, headTarget,
                  currentTarget, dagChecker, context->Language);
  if (cge->GetHadContextSensitiveCondition()) {
    context->HadContextSensitiveCondition = true;
  }
  if (cge->GetHadHeadSensitiveCondition()) {
    context->HadHeadSensitiveCondition = true;
  }
  return result;
}

static const struct ZeroNode : public cmGeneratorExpressionNode
{
  ZeroNode() {}

  bool GeneratesContent() const CM_OVERRIDE { return false; }

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return std::string();
  }
} zeroNode;

static const struct OneNode : public cmGeneratorExpressionNode
{
  OneNode() {}

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return parameters.front();
  }
} oneNode;

static const struct OneNode buildInterfaceNode;

static const struct ZeroNode installInterfaceNode;

#define BOOLEAN_OP_NODE(OPNAME, OP, SUCCESS_VALUE, FAILURE_VALUE)             \
  static const struct OP##Node : public cmGeneratorExpressionNode             \
  {                                                                           \
    OP##Node() {}                                                             \
    virtual int NumExpectedParameters() const { return OneOrMoreParameters; } \
                                                                              \
    std::string Evaluate(const std::vector<std::string>& parameters,          \
                         cmGeneratorExpressionContext* context,               \
                         const GeneratorExpressionContent* content,           \
                         cmGeneratorExpressionDAGChecker*) const              \
    {                                                                         \
      std::vector<std::string>::const_iterator it = parameters.begin();       \
      const std::vector<std::string>::const_iterator end = parameters.end();  \
      for (; it != end; ++it) {                                               \
        if (*it == #FAILURE_VALUE) {                                          \
          return #FAILURE_VALUE;                                              \
        }                                                                     \
        if (*it != #SUCCESS_VALUE) {                                          \
          reportError(context, content->GetOriginalExpression(),              \
                      "Parameters to $<" #OP                                  \
                      "> must resolve to either '0' or '1'.");                \
          return std::string();                                               \
        }                                                                     \
      }                                                                       \
      return #SUCCESS_VALUE;                                                  \
    }                                                                         \
  } OPNAME;

BOOLEAN_OP_NODE(andNode, AND, 1, 0)
BOOLEAN_OP_NODE(orNode, OR, 0, 1)

#undef BOOLEAN_OP_NODE

static const struct NotNode : public cmGeneratorExpressionNode
{
  NotNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    if (*parameters.begin() != "0" && *parameters.begin() != "1") {
      reportError(
        context, content->GetOriginalExpression(),
        "$<NOT> parameter must resolve to exactly one '0' or '1' value.");
      return std::string();
    }
    return *parameters.begin() == "0" ? "1" : "0";
  }
} notNode;

static const struct BoolNode : public cmGeneratorExpressionNode
{
  BoolNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 1; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return !cmSystemTools::IsOff(parameters.begin()->c_str()) ? "1" : "0";
  }
} boolNode;

static const struct IfNode : public cmGeneratorExpressionNode
{
  IfNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 3; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker*) const CM_OVERRIDE
  {
    if (parameters[0] != "1" && parameters[0] != "0") {
      reportError(context, content->GetOriginalExpression(),
                  "First parameter to $<IF> must resolve to exactly one '0' "
                  "or '1' value.");
      return std::string();
    }
    return parameters[0] == "1" ? parameters[1] : parameters[2];
  }
} ifNode;

static const struct StrEqualNode : public cmGeneratorExpressionNode
{
  StrEqualNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return *parameters.begin() == parameters[1] ? "1" : "0";
  }
} strEqualNode;

static const struct EqualNode : public cmGeneratorExpressionNode
{
  EqualNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    char* pEnd;

    int base = 0;
    bool flipSign = false;

    const char* lhs = parameters[0].c_str();
    if (cmHasLiteralPrefix(lhs, "0b") || cmHasLiteralPrefix(lhs, "0B")) {
      base = 2;
      lhs += 2;
    }
    if (cmHasLiteralPrefix(lhs, "-0b") || cmHasLiteralPrefix(lhs, "-0B")) {
      base = 2;
      lhs += 3;
      flipSign = true;
    }
    if (cmHasLiteralPrefix(lhs, "+0b") || cmHasLiteralPrefix(lhs, "+0B")) {
      base = 2;
      lhs += 3;
    }

    long lnum = strtol(lhs, &pEnd, base);
    if (pEnd == lhs || *pEnd != '\0' || errno == ERANGE) {
      reportError(context, content->GetOriginalExpression(),
                  "$<EQUAL> parameter " + parameters[0] +
                    " is not a valid integer.");
      return std::string();
    }

    if (flipSign) {
      lnum = -lnum;
    }

    base = 0;
    flipSign = false;

    const char* rhs = parameters[1].c_str();
    if (cmHasLiteralPrefix(rhs, "0b") || cmHasLiteralPrefix(rhs, "0B")) {
      base = 2;
      rhs += 2;
    }
    if (cmHasLiteralPrefix(rhs, "-0b") || cmHasLiteralPrefix(rhs, "-0B")) {
      base = 2;
      rhs += 3;
      flipSign = true;
    }
    if (cmHasLiteralPrefix(rhs, "+0b") || cmHasLiteralPrefix(rhs, "+0B")) {
      base = 2;
      rhs += 3;
    }

    long rnum = strtol(rhs, &pEnd, base);
    if (pEnd == rhs || *pEnd != '\0' || errno == ERANGE) {
      reportError(context, content->GetOriginalExpression(),
                  "$<EQUAL> parameter " + parameters[1] +
                    " is not a valid integer.");
      return std::string();
    }

    if (flipSign) {
      rnum = -rnum;
    }

    return lnum == rnum ? "1" : "0";
  }
} equalNode;

static const struct LowerCaseNode : public cmGeneratorExpressionNode
{
  LowerCaseNode() {}

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::LowerCase(parameters.front());
  }
} lowerCaseNode;

static const struct UpperCaseNode : public cmGeneratorExpressionNode
{
  UpperCaseNode() {}

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::UpperCase(parameters.front());
  }
} upperCaseNode;

static const struct MakeCIdentifierNode : public cmGeneratorExpressionNode
{
  MakeCIdentifierNode() {}

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::MakeCidentifier(parameters.front());
  }
} makeCIdentifierNode;

static const struct Angle_RNode : public cmGeneratorExpressionNode
{
  Angle_RNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 0; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return ">";
  }
} angle_rNode;

static const struct CommaNode : public cmGeneratorExpressionNode
{
  CommaNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 0; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return ",";
  }
} commaNode;

static const struct SemicolonNode : public cmGeneratorExpressionNode
{
  SemicolonNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 0; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return ";";
  }
} semicolonNode;

struct CompilerIdNode : public cmGeneratorExpressionNode
{
  CompilerIdNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrZeroParameters; }

  std::string EvaluateWithLanguage(const std::vector<std::string>& parameters,
                                   cmGeneratorExpressionContext* context,
                                   const GeneratorExpressionContent* content,
                                   cmGeneratorExpressionDAGChecker* /*unused*/,
                                   const std::string& lang) const
  {
    const char* compilerId = context->LG->GetMakefile()->GetSafeDefinition(
      "CMAKE_" + lang + "_COMPILER_ID");
    if (parameters.empty()) {
      return compilerId ? compilerId : "";
    }
    static cmsys::RegularExpression compilerIdValidator("^[A-Za-z0-9_]*$");
    if (!compilerIdValidator.find(*parameters.begin())) {
      reportError(context, content->GetOriginalExpression(),
                  "Expression syntax not recognized.");
      return std::string();
    }
    if (!compilerId) {
      return parameters.front().empty() ? "1" : "0";
    }

    if (strcmp(parameters.begin()->c_str(), compilerId) == 0) {
      return "1";
    }

    if (cmsysString_strcasecmp(parameters.begin()->c_str(), compilerId) == 0) {
      switch (context->LG->GetPolicyStatus(cmPolicies::CMP0044)) {
        case cmPolicies::WARN: {
          std::ostringstream e;
          e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0044);
          context->LG->GetCMakeInstance()->IssueMessage(
            cmake::AUTHOR_WARNING, e.str(), context->Backtrace);
          CM_FALLTHROUGH;
        }
        case cmPolicies::OLD:
          return "1";
        case cmPolicies::NEW:
        case cmPolicies::REQUIRED_ALWAYS:
        case cmPolicies::REQUIRED_IF_USED:
          break;
      }
    }
    return "0";
  }
};

static const struct CCompilerIdNode : public CompilerIdNode
{
  CCompilerIdNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (!context->HeadTarget) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<C_COMPILER_ID> may only be used with binary targets.  It may "
        "not be used with add_custom_command or add_custom_target.");
      return std::string();
    }
    return this->EvaluateWithLanguage(parameters, context, content, dagChecker,
                                      "C");
  }
} cCompilerIdNode;

static const struct CXXCompilerIdNode : public CompilerIdNode
{
  CXXCompilerIdNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (!context->HeadTarget) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<CXX_COMPILER_ID> may only be used with binary targets.  It may "
        "not be used with add_custom_command or add_custom_target.");
      return std::string();
    }
    return this->EvaluateWithLanguage(parameters, context, content, dagChecker,
                                      "CXX");
  }
} cxxCompilerIdNode;

struct CompilerVersionNode : public cmGeneratorExpressionNode
{
  CompilerVersionNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrZeroParameters; }

  std::string EvaluateWithLanguage(const std::vector<std::string>& parameters,
                                   cmGeneratorExpressionContext* context,
                                   const GeneratorExpressionContent* content,
                                   cmGeneratorExpressionDAGChecker* /*unused*/,
                                   const std::string& lang) const
  {
    const char* compilerVersion =
      context->LG->GetMakefile()->GetSafeDefinition("CMAKE_" + lang +
                                                    "_COMPILER_VERSION");
    if (parameters.empty()) {
      return compilerVersion ? compilerVersion : "";
    }

    static cmsys::RegularExpression compilerIdValidator("^[0-9\\.]*$");
    if (!compilerIdValidator.find(*parameters.begin())) {
      reportError(context, content->GetOriginalExpression(),
                  "Expression syntax not recognized.");
      return std::string();
    }
    if (!compilerVersion) {
      return parameters.front().empty() ? "1" : "0";
    }

    return cmSystemTools::VersionCompare(cmSystemTools::OP_EQUAL,
                                         parameters.begin()->c_str(),
                                         compilerVersion)
      ? "1"
      : "0";
  }
};

static const struct CCompilerVersionNode : public CompilerVersionNode
{
  CCompilerVersionNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (!context->HeadTarget) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<C_COMPILER_VERSION> may only be used with binary targets.  It "
        "may not be used with add_custom_command or add_custom_target.");
      return std::string();
    }
    return this->EvaluateWithLanguage(parameters, context, content, dagChecker,
                                      "C");
  }
} cCompilerVersionNode;

static const struct CxxCompilerVersionNode : public CompilerVersionNode
{
  CxxCompilerVersionNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (!context->HeadTarget) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<CXX_COMPILER_VERSION> may only be used with binary targets.  It "
        "may not be used with add_custom_command or add_custom_target.");
      return std::string();
    }
    return this->EvaluateWithLanguage(parameters, context, content, dagChecker,
                                      "CXX");
  }
} cxxCompilerVersionNode;

struct PlatformIdNode : public cmGeneratorExpressionNode
{
  PlatformIdNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrZeroParameters; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    const char* platformId =
      context->LG->GetMakefile()->GetSafeDefinition("CMAKE_SYSTEM_NAME");
    if (parameters.empty()) {
      return platformId ? platformId : "";
    }

    if (!platformId) {
      return parameters.front().empty() ? "1" : "0";
    }

    if (strcmp(parameters.begin()->c_str(), platformId) == 0) {
      return "1";
    }
    return "0";
  }
} platformIdNode;

static const struct VersionGreaterNode : public cmGeneratorExpressionNode
{
  VersionGreaterNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::VersionCompare(cmSystemTools::OP_GREATER,
                                         parameters.front().c_str(),
                                         parameters[1].c_str())
      ? "1"
      : "0";
  }
} versionGreaterNode;

static const struct VersionGreaterEqNode : public cmGeneratorExpressionNode
{
  VersionGreaterEqNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::VersionCompare(cmSystemTools::OP_GREATER_EQUAL,
                                         parameters.front().c_str(),
                                         parameters[1].c_str())
      ? "1"
      : "0";
  }
} versionGreaterEqNode;

static const struct VersionLessNode : public cmGeneratorExpressionNode
{
  VersionLessNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::VersionCompare(cmSystemTools::OP_LESS,
                                         parameters.front().c_str(),
                                         parameters[1].c_str())
      ? "1"
      : "0";
  }
} versionLessNode;

static const struct VersionLessEqNode : public cmGeneratorExpressionNode
{
  VersionLessEqNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::VersionCompare(cmSystemTools::OP_LESS_EQUAL,
                                         parameters.front().c_str(),
                                         parameters[1].c_str())
      ? "1"
      : "0";
  }
} versionLessEqNode;

static const struct VersionEqualNode : public cmGeneratorExpressionNode
{
  VersionEqualNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return cmSystemTools::VersionCompare(cmSystemTools::OP_EQUAL,
                                         parameters.front().c_str(),
                                         parameters[1].c_str())
      ? "1"
      : "0";
  }
} versionEqualNode;

static const struct LinkOnlyNode : public cmGeneratorExpressionNode
{
  LinkOnlyNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (!dagChecker) {
      reportError(context, content->GetOriginalExpression(),
                  "$<LINK_ONLY:...> may only be used for linking");
      return std::string();
    }
    if (!dagChecker->GetTransitivePropertiesOnly()) {
      return parameters.front();
    }
    return std::string();
  }
} linkOnlyNode;

static const struct ConfigurationNode : public cmGeneratorExpressionNode
{
  ConfigurationNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 0; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    context->HadContextSensitiveCondition = true;
    return context->Config;
  }
} configurationNode;

static const struct ConfigurationTestNode : public cmGeneratorExpressionNode
{
  ConfigurationTestNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrZeroParameters; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    if (parameters.empty()) {
      return configurationNode.Evaluate(parameters, context, content,
                                        CM_NULLPTR);
    }
    static cmsys::RegularExpression configValidator("^[A-Za-z0-9_]*$");
    if (!configValidator.find(*parameters.begin())) {
      reportError(context, content->GetOriginalExpression(),
                  "Expression syntax not recognized.");
      return std::string();
    }
    context->HadContextSensitiveCondition = true;
    if (context->Config.empty()) {
      return parameters.front().empty() ? "1" : "0";
    }

    if (cmsysString_strcasecmp(parameters.begin()->c_str(),
                               context->Config.c_str()) == 0) {
      return "1";
    }

    if (context->CurrentTarget && context->CurrentTarget->IsImported()) {
      const char* loc = CM_NULLPTR;
      const char* imp = CM_NULLPTR;
      std::string suffix;
      if (context->CurrentTarget->Target->GetMappedConfig(
            context->Config, &loc, &imp, suffix)) {
        // This imported target has an appropriate location
        // for this (possibly mapped) config.
        // Check if there is a proper config mapping for the tested config.
        std::vector<std::string> mappedConfigs;
        std::string mapProp = "MAP_IMPORTED_CONFIG_";
        mapProp += cmSystemTools::UpperCase(context->Config);
        if (const char* mapValue =
              context->CurrentTarget->GetProperty(mapProp)) {
          cmSystemTools::ExpandListArgument(cmSystemTools::UpperCase(mapValue),
                                            mappedConfigs);
          return std::find(mappedConfigs.begin(), mappedConfigs.end(),
                           cmSystemTools::UpperCase(parameters.front())) !=
              mappedConfigs.end()
            ? "1"
            : "0";
        }
      }
    }
    return "0";
  }
} configurationTestNode;

static const struct JoinNode : public cmGeneratorExpressionNode
{
  JoinNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 2; }

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    std::vector<std::string> list;
    cmSystemTools::ExpandListArgument(parameters.front(), list);
    return cmJoin(list, parameters[1]);
  }
} joinNode;

static const struct CompileLanguageNode : public cmGeneratorExpressionNode
{
  CompileLanguageNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrZeroParameters; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    if (context->Language.empty()) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<COMPILE_LANGUAGE:...> may only be used to specify include "
        "directories compile definitions, compile options and to evaluate "
        "components of the file(GENERATE) command.");
      return std::string();
    }

    std::vector<std::string> enabledLanguages;
    cmGlobalGenerator* gg = context->LG->GetGlobalGenerator();
    gg->GetEnabledLanguages(enabledLanguages);
    if (!parameters.empty() &&
        std::find(enabledLanguages.begin(), enabledLanguages.end(),
                  parameters.front()) == enabledLanguages.end()) {
      reportError(context, content->GetOriginalExpression(),
                  "$<COMPILE_LANGUAGE:...> Unknown language.");
      return std::string();
    }
    std::string genName = gg->GetName();
    if (genName.find("Visual Studio") != std::string::npos) {
      reportError(context, content->GetOriginalExpression(),
                  "$<COMPILE_LANGUAGE:...> may not be used with Visual Studio "
                  "generators.");
      return std::string();
    }
    if (genName.find("Xcode") != std::string::npos) {
      if (dagChecker && (dagChecker->EvaluatingCompileDefinitions() ||
                         dagChecker->EvaluatingIncludeDirectories())) {
        reportError(
          context, content->GetOriginalExpression(),
          "$<COMPILE_LANGUAGE:...> may only be used with COMPILE_OPTIONS "
          "with the Xcode generator.");
        return std::string();
      }
    } else {
      if (genName.find("Makefiles") == std::string::npos &&
          genName.find("Ninja") == std::string::npos &&
          genName.find("Watcom WMake") == std::string::npos) {
        reportError(
          context, content->GetOriginalExpression(),
          "$<COMPILE_LANGUAGE:...> not supported for this generator.");
        return std::string();
      }
    }
    if (parameters.empty()) {
      return context->Language;
    }
    return context->Language == parameters.front() ? "1" : "0";
  }
} languageNode;

#define TRANSITIVE_PROPERTY_NAME(PROPERTY) , "INTERFACE_" #PROPERTY

static const char* targetPropertyTransitiveWhitelist[] = {
  CM_NULLPTR CM_FOR_EACH_TRANSITIVE_PROPERTY_NAME(TRANSITIVE_PROPERTY_NAME)
};

#undef TRANSITIVE_PROPERTY_NAME

template <typename T>
std::string getLinkedTargetsContent(
  std::vector<T> const& libraries, cmGeneratorTarget const* target,
  cmGeneratorTarget const* headTarget, cmGeneratorExpressionContext* context,
  cmGeneratorExpressionDAGChecker* dagChecker,
  const std::string& interfacePropertyName)
{
  std::string linkedTargetsContent;
  std::string sep;
  std::string depString;
  for (typename std::vector<T>::const_iterator it = libraries.begin();
       it != libraries.end(); ++it) {
    // Broken code can have a target in its own link interface.
    // Don't follow such link interface entries so as not to create a
    // self-referencing loop.
    if (it->Target && it->Target != target) {
      depString += sep + "$<TARGET_PROPERTY:" + it->Target->GetName() + "," +
        interfacePropertyName + ">";
      sep = ";";
    }
  }
  if (!depString.empty()) {
    linkedTargetsContent =
      cmGeneratorExpressionNode::EvaluateDependentExpression(
        depString, target->GetLocalGenerator(), context, headTarget, target,
        dagChecker);
  }
  linkedTargetsContent =
    cmGeneratorExpression::StripEmptyListElements(linkedTargetsContent);
  return linkedTargetsContent;
}

static const struct TargetPropertyNode : public cmGeneratorExpressionNode
{
  TargetPropertyNode() {}

  // This node handles errors on parameter count itself.
  int NumExpectedParameters() const CM_OVERRIDE { return OneOrMoreParameters; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagCheckerParent) const
    CM_OVERRIDE
  {
    if (parameters.size() != 1 && parameters.size() != 2) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<TARGET_PROPERTY:...> expression requires one or two parameters");
      return std::string();
    }
    static cmsys::RegularExpression propertyNameValidator("^[A-Za-z0-9_]+$");

    cmGeneratorTarget const* target = context->HeadTarget;
    std::string propertyName = *parameters.begin();

    if (parameters.size() == 1) {
      context->HadHeadSensitiveCondition = true;
    }
    if (!target && parameters.size() == 1) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<TARGET_PROPERTY:prop>  may only be used with binary targets.  "
        "It may not be used with add_custom_command or add_custom_target.  "
        "Specify the target to read a property from using the "
        "$<TARGET_PROPERTY:tgt,prop> signature instead.");
      return std::string();
    }

    if (parameters.size() == 2) {
      if (parameters.begin()->empty() && parameters[1].empty()) {
        reportError(
          context, content->GetOriginalExpression(),
          "$<TARGET_PROPERTY:tgt,prop> expression requires a non-empty "
          "target name and property name.");
        return std::string();
      }
      if (parameters.begin()->empty()) {
        reportError(
          context, content->GetOriginalExpression(),
          "$<TARGET_PROPERTY:tgt,prop> expression requires a non-empty "
          "target name.");
        return std::string();
      }

      std::string targetName = parameters.front();
      propertyName = parameters[1];
      if (!cmGeneratorExpression::IsValidTargetName(targetName)) {
        if (!propertyNameValidator.find(propertyName.c_str())) {
          ::reportError(context, content->GetOriginalExpression(),
                        "Target name and property name not supported.");
          return std::string();
        }
        ::reportError(context, content->GetOriginalExpression(),
                      "Target name not supported.");
        return std::string();
      }
      if (propertyName == "ALIASED_TARGET") {
        if (context->LG->GetMakefile()->IsAlias(targetName)) {
          if (cmGeneratorTarget* tgt =
                context->LG->FindGeneratorTargetToUse(targetName)) {
            return tgt->GetName();
          }
        }
        return "";
      }
      target = context->LG->FindGeneratorTargetToUse(targetName);

      if (!target) {
        std::ostringstream e;
        e << "Target \"" << targetName << "\" not found.";
        reportError(context, content->GetOriginalExpression(), e.str());
        return std::string();
      }
      context->AllTargets.insert(target);
    }

    if (target == context->HeadTarget) {
      // Keep track of the properties seen while processing.
      // The evaluation of the LINK_LIBRARIES generator expressions
      // will check this to ensure that properties have one consistent
      // value for all evaluations.
      context->SeenTargetProperties.insert(propertyName);
    }
    if (propertyName == "SOURCES") {
      context->SourceSensitiveTargets.insert(target);
    }

    if (propertyName.empty()) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<TARGET_PROPERTY:...> expression requires a non-empty property "
        "name.");
      return std::string();
    }

    if (!propertyNameValidator.find(propertyName)) {
      ::reportError(context, content->GetOriginalExpression(),
                    "Property name not supported.");
      return std::string();
    }

    assert(target);

    if (propertyName == "LINKER_LANGUAGE") {
      if (target->LinkLanguagePropagatesToDependents() && dagCheckerParent &&
          (dagCheckerParent->EvaluatingLinkLibraries() ||
           dagCheckerParent->EvaluatingSources())) {
        reportError(
          context, content->GetOriginalExpression(),
          "LINKER_LANGUAGE target property can not be used while evaluating "
          "link libraries for a static library");
        return std::string();
      }
      return target->GetLinkerLanguage(context->Config);
    }

    cmGeneratorExpressionDAGChecker dagChecker(context->Backtrace,
                                               target->GetName(), propertyName,
                                               content, dagCheckerParent);

    switch (dagChecker.Check()) {
      case cmGeneratorExpressionDAGChecker::SELF_REFERENCE:
        dagChecker.ReportError(context, content->GetOriginalExpression());
        return std::string();
      case cmGeneratorExpressionDAGChecker::CYCLIC_REFERENCE:
        // No error. We just skip cyclic references.
        return std::string();
      case cmGeneratorExpressionDAGChecker::ALREADY_SEEN:
        for (size_t i = 1; i < cmArraySize(targetPropertyTransitiveWhitelist);
             ++i) {
          if (targetPropertyTransitiveWhitelist[i] == propertyName) {
            // No error. We're not going to find anything new here.
            return std::string();
          }
        }
      case cmGeneratorExpressionDAGChecker::DAG:
        break;
    }

    const char* prop = target->GetProperty(propertyName);

    if (dagCheckerParent) {
      if (dagCheckerParent->EvaluatingLinkLibraries()) {
#define TRANSITIVE_PROPERTY_COMPARE(PROPERTY)                                 \
  (#PROPERTY == propertyName || "INTERFACE_" #PROPERTY == propertyName) ||
        if (CM_FOR_EACH_TRANSITIVE_PROPERTY_NAME(
              TRANSITIVE_PROPERTY_COMPARE) false) { // NOLINT(clang-tidy)
          reportError(
            context, content->GetOriginalExpression(),
            "$<TARGET_PROPERTY:...> expression in link libraries "
            "evaluation depends on target property which is transitive "
            "over the link libraries, creating a recursion.");
          return std::string();
        }
#undef TRANSITIVE_PROPERTY_COMPARE

        if (!prop) {
          return std::string();
        }
      } else {
#define ASSERT_TRANSITIVE_PROPERTY_METHOD(METHOD) dagCheckerParent->METHOD() ||

        assert(CM_FOR_EACH_TRANSITIVE_PROPERTY_METHOD(
          ASSERT_TRANSITIVE_PROPERTY_METHOD) false); // NOLINT(clang-tidy)
#undef ASSERT_TRANSITIVE_PROPERTY_METHOD
      }
    }

    std::string linkedTargetsContent;

    std::string interfacePropertyName;
    bool isInterfaceProperty = false;

#define POPULATE_INTERFACE_PROPERTY_NAME(prop)                                \
  if (propertyName == #prop) {                                                \
    interfacePropertyName = "INTERFACE_" #prop;                               \
  } else if (propertyName == "INTERFACE_" #prop) {                            \
    interfacePropertyName = "INTERFACE_" #prop;                               \
    isInterfaceProperty = true;                                               \
  } else

    CM_FOR_EACH_TRANSITIVE_PROPERTY_NAME(POPULATE_INTERFACE_PROPERTY_NAME)
    // Note that the above macro terminates with an else
    /* else */ if (cmHasLiteralPrefix(propertyName.c_str(),
                                      "COMPILE_DEFINITIONS_")) {
      cmPolicies::PolicyStatus polSt =
        context->LG->GetPolicyStatus(cmPolicies::CMP0043);
      if (polSt == cmPolicies::WARN || polSt == cmPolicies::OLD) {
        interfacePropertyName = "INTERFACE_COMPILE_DEFINITIONS";
      }
    }
#undef POPULATE_INTERFACE_PROPERTY_NAME
    cmGeneratorTarget const* headTarget =
      context->HeadTarget && isInterfaceProperty ? context->HeadTarget
                                                 : target;

    if (isInterfaceProperty) {
      if (cmLinkInterfaceLibraries const* iface =
            target->GetLinkInterfaceLibraries(context->Config, headTarget,
                                              true)) {
        linkedTargetsContent =
          getLinkedTargetsContent(iface->Libraries, target, headTarget,
                                  context, &dagChecker, interfacePropertyName);
      }
    } else if (!interfacePropertyName.empty()) {
      if (cmLinkImplementationLibraries const* impl =
            target->GetLinkImplementationLibraries(context->Config)) {
        linkedTargetsContent =
          getLinkedTargetsContent(impl->Libraries, target, target, context,
                                  &dagChecker, interfacePropertyName);
      }
    }

    if (!prop) {
      if (target->IsImported() ||
          target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        return linkedTargetsContent;
      }
      if (target->IsLinkInterfaceDependentBoolProperty(propertyName,
                                                       context->Config)) {
        context->HadContextSensitiveCondition = true;
        return target->GetLinkInterfaceDependentBoolProperty(propertyName,
                                                             context->Config)
          ? "1"
          : "0";
      }
      if (target->IsLinkInterfaceDependentStringProperty(propertyName,
                                                         context->Config)) {
        context->HadContextSensitiveCondition = true;
        const char* propContent =
          target->GetLinkInterfaceDependentStringProperty(propertyName,
                                                          context->Config);
        return propContent ? propContent : "";
      }
      if (target->IsLinkInterfaceDependentNumberMinProperty(propertyName,
                                                            context->Config)) {
        context->HadContextSensitiveCondition = true;
        const char* propContent =
          target->GetLinkInterfaceDependentNumberMinProperty(propertyName,
                                                             context->Config);
        return propContent ? propContent : "";
      }
      if (target->IsLinkInterfaceDependentNumberMaxProperty(propertyName,
                                                            context->Config)) {
        context->HadContextSensitiveCondition = true;
        const char* propContent =
          target->GetLinkInterfaceDependentNumberMaxProperty(propertyName,
                                                             context->Config);
        return propContent ? propContent : "";
      }

      return linkedTargetsContent;
    }

    if (!target->IsImported() && dagCheckerParent &&
        !dagCheckerParent->EvaluatingLinkLibraries()) {
      if (target->IsLinkInterfaceDependentNumberMinProperty(propertyName,
                                                            context->Config)) {
        context->HadContextSensitiveCondition = true;
        const char* propContent =
          target->GetLinkInterfaceDependentNumberMinProperty(propertyName,
                                                             context->Config);
        return propContent ? propContent : "";
      }
      if (target->IsLinkInterfaceDependentNumberMaxProperty(propertyName,
                                                            context->Config)) {
        context->HadContextSensitiveCondition = true;
        const char* propContent =
          target->GetLinkInterfaceDependentNumberMaxProperty(propertyName,
                                                             context->Config);
        return propContent ? propContent : "";
      }
    }
    if (!interfacePropertyName.empty()) {
      std::string result = this->EvaluateDependentExpression(
        prop, context->LG, context, headTarget, target, &dagChecker);
      if (!linkedTargetsContent.empty()) {
        result += (result.empty() ? "" : ";") + linkedTargetsContent;
      }
      return result;
    }
    return prop;
  }
} targetPropertyNode;

static const struct TargetNameNode : public cmGeneratorExpressionNode
{
  TargetNameNode() {}

  bool GeneratesContent() const CM_OVERRIDE { return true; }

  bool AcceptsArbitraryContentParameter() const CM_OVERRIDE { return true; }
  bool RequiresLiteralInput() const CM_OVERRIDE { return true; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* /*context*/,
                       const GeneratorExpressionContent* /*content*/,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    return parameters.front();
  }

  int NumExpectedParameters() const CM_OVERRIDE { return 1; }

} targetNameNode;

static const struct TargetObjectsNode : public cmGeneratorExpressionNode
{
  TargetObjectsNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    std::string tgtName = parameters.front();
    cmGeneratorTarget* gt = context->LG->FindGeneratorTargetToUse(tgtName);
    if (!gt) {
      std::ostringstream e;
      e << "Objects of target \"" << tgtName
        << "\" referenced but no such target exists.";
      reportError(context, content->GetOriginalExpression(), e.str());
      return std::string();
    }
    if (gt->GetType() != cmStateEnums::OBJECT_LIBRARY) {
      std::ostringstream e;
      e << "Objects of target \"" << tgtName
        << "\" referenced but is not an OBJECT library.";
      reportError(context, content->GetOriginalExpression(), e.str());
      return std::string();
    }
    if (!context->EvaluateForBuildsystem) {
      cmGlobalGenerator* gg = context->LG->GetGlobalGenerator();
      std::string reason;
      if (!gg->HasKnownObjectFileLocation(&reason)) {
        std::ostringstream e;
        e << "The evaluation of the TARGET_OBJECTS generator expression "
             "is only suitable for consumption by CMake (limited"
          << reason << ").  "
                       "It is not suitable for writing out elsewhere.";
        reportError(context, content->GetOriginalExpression(), e.str());
        return std::string();
      }
    }

    std::vector<std::string> objects;

    if (gt->IsImported()) {
      const char* loc = CM_NULLPTR;
      const char* imp = CM_NULLPTR;
      std::string suffix;
      if (gt->Target->GetMappedConfig(context->Config, &loc, &imp, suffix)) {
        cmSystemTools::ExpandListArgument(loc, objects);
      }
      context->HadContextSensitiveCondition = true;
    } else {
      gt->GetTargetObjectNames(context->Config, objects);

      std::string obj_dir;
      if (context->EvaluateForBuildsystem) {
        // Use object file directory with buildsystem placeholder.
        obj_dir = gt->ObjectDirectory;
        // Here we assume that the set of object files produced
        // by an object library does not vary with configuration
        // and do not set HadContextSensitiveCondition to true.
      } else {
        // Use object file directory with per-config location.
        obj_dir = gt->GetObjectDirectory(context->Config);
        context->HadContextSensitiveCondition = true;
      }

      for (std::vector<std::string>::iterator oi = objects.begin();
           oi != objects.end(); ++oi) {
        *oi = obj_dir + *oi;
      }
    }

    // Create the cmSourceFile instances in the referencing directory.
    cmMakefile* mf = context->LG->GetMakefile();
    for (std::vector<std::string>::iterator oi = objects.begin();
         oi != objects.end(); ++oi) {
      mf->AddTargetObject(tgtName, *oi);
    }

    return cmJoin(objects, ";");
  }
} targetObjectsNode;

static const struct CompileFeaturesNode : public cmGeneratorExpressionNode
{
  CompileFeaturesNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return OneOrMoreParameters; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    cmGeneratorTarget const* target = context->HeadTarget;
    if (!target) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<COMPILE_FEATURE> may only be used with binary targets.  It may "
        "not be used with add_custom_command or add_custom_target.");
      return std::string();
    }
    context->HadHeadSensitiveCondition = true;

    typedef std::map<std::string, std::vector<std::string> > LangMap;
    static LangMap availableFeatures;

    LangMap testedFeatures;

    for (std::vector<std::string>::const_iterator it = parameters.begin();
         it != parameters.end(); ++it) {
      std::string error;
      std::string lang;
      if (!context->LG->GetMakefile()->CompileFeatureKnown(
            context->HeadTarget->Target, *it, lang, &error)) {
        reportError(context, content->GetOriginalExpression(), error);
        return std::string();
      }
      testedFeatures[lang].push_back(*it);

      if (availableFeatures.find(lang) == availableFeatures.end()) {
        const char* featuresKnown =
          context->LG->GetMakefile()->CompileFeaturesAvailable(lang, &error);
        if (!featuresKnown) {
          reportError(context, content->GetOriginalExpression(), error);
          return std::string();
        }
        cmSystemTools::ExpandListArgument(featuresKnown,
                                          availableFeatures[lang]);
      }
    }

    bool evalLL = dagChecker && dagChecker->EvaluatingLinkLibraries();

    for (LangMap::const_iterator lit = testedFeatures.begin();
         lit != testedFeatures.end(); ++lit) {
      std::vector<std::string> const& langAvailable =
        availableFeatures[lit->first];
      const char* standardDefault = context->LG->GetMakefile()->GetDefinition(
        "CMAKE_" + lit->first + "_STANDARD_DEFAULT");
      for (std::vector<std::string>::const_iterator it = lit->second.begin();
           it != lit->second.end(); ++it) {
        if (std::find(langAvailable.begin(), langAvailable.end(), *it) ==
            langAvailable.end()) {
          return "0";
        }
        if (standardDefault && !*standardDefault) {
          // This compiler has no notion of language standard levels.
          // All features known for the language are always available.
          continue;
        }
        if (!context->LG->GetMakefile()->HaveStandardAvailable(
              target->Target, lit->first, *it)) {
          if (evalLL) {
            const char* l = target->GetProperty(lit->first + "_STANDARD");
            if (!l) {
              l = standardDefault;
            }
            assert(l);
            context->MaxLanguageStandard[target][lit->first] = l;
          } else {
            return "0";
          }
        }
      }
    }
    return "1";
  }
} compileFeaturesNode;

static const char* targetPolicyWhitelist[] = {
  CM_NULLPTR
#define TARGET_POLICY_STRING(POLICY) , #POLICY

    CM_FOR_EACH_TARGET_POLICY(TARGET_POLICY_STRING)

#undef TARGET_POLICY_STRING
};

cmPolicies::PolicyStatus statusForTarget(cmGeneratorTarget const* tgt,
                                         const char* policy)
{
#define RETURN_POLICY(POLICY)                                                 \
  if (strcmp(policy, #POLICY) == 0) {                                         \
    return tgt->GetPolicyStatus##POLICY();                                    \
  }

  CM_FOR_EACH_TARGET_POLICY(RETURN_POLICY)

#undef RETURN_POLICY

  assert(false && "Unreachable code. Not a valid policy");
  return cmPolicies::WARN;
}

cmPolicies::PolicyID policyForString(const char* policy_id)
{
#define RETURN_POLICY_ID(POLICY_ID)                                           \
  if (strcmp(policy_id, #POLICY_ID) == 0) {                                   \
    return cmPolicies::POLICY_ID;                                             \
  }

  CM_FOR_EACH_TARGET_POLICY(RETURN_POLICY_ID)

#undef RETURN_POLICY_ID

  assert(false && "Unreachable code. Not a valid policy");
  return cmPolicies::CMP0002;
}

static const struct TargetPolicyNode : public cmGeneratorExpressionNode
{
  TargetPolicyNode() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 1; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    if (!context->HeadTarget) {
      reportError(
        context, content->GetOriginalExpression(),
        "$<TARGET_POLICY:prop> may only be used with binary targets.  It "
        "may not be used with add_custom_command or add_custom_target.");
      return std::string();
    }

    context->HadContextSensitiveCondition = true;
    context->HadHeadSensitiveCondition = true;

    for (size_t i = 1; i < cmArraySize(targetPolicyWhitelist); ++i) {
      const char* policy = targetPolicyWhitelist[i];
      if (parameters.front() == policy) {
        cmLocalGenerator* lg = context->HeadTarget->GetLocalGenerator();
        switch (statusForTarget(context->HeadTarget, policy)) {
          case cmPolicies::WARN:
            lg->IssueMessage(
              cmake::AUTHOR_WARNING,
              cmPolicies::GetPolicyWarning(policyForString(policy)));
            CM_FALLTHROUGH;
          case cmPolicies::REQUIRED_IF_USED:
          case cmPolicies::REQUIRED_ALWAYS:
          case cmPolicies::OLD:
            return "0";
          case cmPolicies::NEW:
            return "1";
        }
      }
    }
    reportError(
      context, content->GetOriginalExpression(),
      "$<TARGET_POLICY:prop> may only be used with a limited number of "
      "policies.  Currently it may be used with the following policies:\n"

#define STRINGIFY_HELPER(X) #X
#define STRINGIFY(X) STRINGIFY_HELPER(X)

#define TARGET_POLICY_LIST_ITEM(POLICY) " * " STRINGIFY(POLICY) "\n"

      CM_FOR_EACH_TARGET_POLICY(TARGET_POLICY_LIST_ITEM)

#undef TARGET_POLICY_LIST_ITEM
        );
    return std::string();
  }

} targetPolicyNode;

static const struct InstallPrefixNode : public cmGeneratorExpressionNode
{
  InstallPrefixNode() {}

  bool GeneratesContent() const CM_OVERRIDE { return true; }
  int NumExpectedParameters() const CM_OVERRIDE { return 0; }

  std::string Evaluate(const std::vector<std::string>& /*parameters*/,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    reportError(context, content->GetOriginalExpression(),
                "INSTALL_PREFIX is a marker for install(EXPORT) only.  It "
                "should never be evaluated.");
    return std::string();
  }

} installPrefixNode;

class ArtifactDirTag;
class ArtifactLinkerTag;
class ArtifactNameTag;
class ArtifactPathTag;
class ArtifactPdbTag;
class ArtifactSonameTag;
class ArtifactBundleDirTag;
class ArtifactBundleContentDirTag;

template <typename ArtifactT>
struct TargetFilesystemArtifactResultCreator
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content);
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactSonameTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content)
  {
    // The target soname file (.so.1).
    if (target->IsDLLPlatform()) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_SONAME_FILE is not allowed "
                    "for DLL target platforms.");
      return std::string();
    }
    if (target->GetType() != cmStateEnums::SHARED_LIBRARY) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_SONAME_FILE is allowed only for "
                    "SHARED libraries.");
      return std::string();
    }
    std::string result = target->GetDirectory(context->Config);
    result += "/";
    result += target->GetSOName(context->Config);
    return result;
  }
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactPdbTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content)
  {
    if (target->IsImported()) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_PDB_FILE not allowed for IMPORTED targets.");
      return std::string();
    }

    std::string language = target->GetLinkerLanguage(context->Config);

    std::string pdbSupportVar = "CMAKE_" + language + "_LINKER_SUPPORTS_PDB";

    if (!context->LG->GetMakefile()->IsOn(pdbSupportVar)) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_PDB_FILE is not supported by the target linker.");
      return std::string();
    }

    cmStateEnums::TargetType targetType = target->GetType();

    if (targetType != cmStateEnums::SHARED_LIBRARY &&
        targetType != cmStateEnums::MODULE_LIBRARY &&
        targetType != cmStateEnums::EXECUTABLE) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_PDB_FILE is allowed only for "
                    "targets with linker created artifacts.");
      return std::string();
    }

    std::string result = target->GetPDBDirectory(context->Config);
    result += "/";
    result += target->GetPDBName(context->Config);
    return result;
  }
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactLinkerTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content)
  {
    // The file used to link to the target (.so, .lib, .a).
    if (!target->IsLinkable()) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_LINKER_FILE is allowed only for libraries and "
                    "executables with ENABLE_EXPORTS.");
      return std::string();
    }
    cmStateEnums::ArtifactType artifact = target->HasImportLibrary()
      ? cmStateEnums::ImportLibraryArtifact
      : cmStateEnums::RuntimeBinaryArtifact;
    return target->GetFullPath(context->Config, artifact);
  }
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactBundleDirTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content)
  {
    if (target->IsImported()) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_BUNDLE_DIR not allowed for IMPORTED targets.");
      return std::string();
    }
    if (!target->IsBundleOnApple()) {
      ::reportError(context, content->GetOriginalExpression(),
                    "TARGET_BUNDLE_DIR is allowed only for Bundle targets.");
      return std::string();
    }

    std::string outpath = target->GetDirectory(context->Config) + '/';
    return target->BuildBundleDirectory(outpath, context->Config,
                                        cmGeneratorTarget::BundleDirLevel);
  }
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactBundleContentDirTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* content)
  {
    if (target->IsImported()) {
      ::reportError(
        context, content->GetOriginalExpression(),
        "TARGET_BUNDLE_CONTENT_DIR not allowed for IMPORTED targets.");
      return std::string();
    }
    if (!target->IsBundleOnApple()) {
      ::reportError(
        context, content->GetOriginalExpression(),
        "TARGET_BUNDLE_CONTENT_DIR is allowed only for Bundle targets.");
      return std::string();
    }

    std::string outpath = target->GetDirectory(context->Config) + '/';
    return target->BuildBundleDirectory(outpath, context->Config,
                                        cmGeneratorTarget::ContentLevel);
  }
};

template <>
struct TargetFilesystemArtifactResultCreator<ArtifactNameTag>
{
  static std::string Create(cmGeneratorTarget* target,
                            cmGeneratorExpressionContext* context,
                            const GeneratorExpressionContent* /*unused*/)
  {
    return target->GetFullPath(context->Config,
                               cmStateEnums::RuntimeBinaryArtifact, true);
  }
};

template <typename ArtifactT>
struct TargetFilesystemArtifactResultGetter
{
  static std::string Get(const std::string& result);
};

template <>
struct TargetFilesystemArtifactResultGetter<ArtifactNameTag>
{
  static std::string Get(const std::string& result)
  {
    return cmSystemTools::GetFilenameName(result);
  }
};

template <>
struct TargetFilesystemArtifactResultGetter<ArtifactDirTag>
{
  static std::string Get(const std::string& result)
  {
    return cmSystemTools::GetFilenamePath(result);
  }
};

template <>
struct TargetFilesystemArtifactResultGetter<ArtifactPathTag>
{
  static std::string Get(const std::string& result) { return result; }
};

template <typename ArtifactT, typename ComponentT>
struct TargetFilesystemArtifact : public cmGeneratorExpressionNode
{
  TargetFilesystemArtifact() {}

  int NumExpectedParameters() const CM_OVERRIDE { return 1; }

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* dagChecker) const
    CM_OVERRIDE
  {
    // Lookup the referenced target.
    std::string name = *parameters.begin();

    if (!cmGeneratorExpression::IsValidTargetName(name)) {
      ::reportError(context, content->GetOriginalExpression(),
                    "Expression syntax not recognized.");
      return std::string();
    }
    cmGeneratorTarget* target = context->LG->FindGeneratorTargetToUse(name);
    if (!target) {
      ::reportError(context, content->GetOriginalExpression(),
                    "No target \"" + name + "\"");
      return std::string();
    }
    if (target->GetType() >= cmStateEnums::OBJECT_LIBRARY &&
        target->GetType() != cmStateEnums::UNKNOWN_LIBRARY) {
      ::reportError(context, content->GetOriginalExpression(), "Target \"" +
                      name + "\" is not an executable or library.");
      return std::string();
    }
    if (dagChecker && (dagChecker->EvaluatingLinkLibraries(name.c_str()) ||
                       (dagChecker->EvaluatingSources() &&
                        name == dagChecker->TopTarget()))) {
      ::reportError(context, content->GetOriginalExpression(),
                    "Expressions which require the linker language may not "
                    "be used while evaluating link libraries");
      return std::string();
    }
    context->DependTargets.insert(target);
    context->AllTargets.insert(target);

    std::string result =
      TargetFilesystemArtifactResultCreator<ArtifactT>::Create(target, context,
                                                               content);
    if (context->HadError) {
      return std::string();
    }
    return TargetFilesystemArtifactResultGetter<ComponentT>::Get(result);
  }
};

template <typename ArtifactT>
struct TargetFilesystemArtifactNodeGroup
{
  TargetFilesystemArtifactNodeGroup() {}

  TargetFilesystemArtifact<ArtifactT, ArtifactPathTag> File;
  TargetFilesystemArtifact<ArtifactT, ArtifactNameTag> FileName;
  TargetFilesystemArtifact<ArtifactT, ArtifactDirTag> FileDir;
};

static const TargetFilesystemArtifactNodeGroup<ArtifactNameTag>
  targetNodeGroup;

static const TargetFilesystemArtifactNodeGroup<ArtifactLinkerTag>
  targetLinkerNodeGroup;

static const TargetFilesystemArtifactNodeGroup<ArtifactSonameTag>
  targetSoNameNodeGroup;

static const TargetFilesystemArtifactNodeGroup<ArtifactPdbTag>
  targetPdbNodeGroup;

static const TargetFilesystemArtifact<ArtifactBundleDirTag, ArtifactPathTag>
  targetBundleDirNode;

static const TargetFilesystemArtifact<ArtifactBundleContentDirTag,
                                      ArtifactPathTag>
  targetBundleContentDirNode;

static const struct ShellPathNode : public cmGeneratorExpressionNode
{
  ShellPathNode() {}

  std::string Evaluate(const std::vector<std::string>& parameters,
                       cmGeneratorExpressionContext* context,
                       const GeneratorExpressionContent* content,
                       cmGeneratorExpressionDAGChecker* /*dagChecker*/) const
    CM_OVERRIDE
  {
    if (!cmSystemTools::FileIsFullPath(parameters.front())) {
      reportError(context, content->GetOriginalExpression(),
                  "\"" + parameters.front() + "\" is not an absolute path.");
      return std::string();
    }
    cmOutputConverter converter(context->LG->GetStateSnapshot());
    return converter.ConvertDirectorySeparatorsForShell(parameters.front());
  }
} shellPathNode;

const cmGeneratorExpressionNode* cmGeneratorExpressionNode::GetNode(
  const std::string& identifier)
{
  typedef std::map<std::string, const cmGeneratorExpressionNode*> NodeMap;
  static NodeMap nodeMap;
  if (nodeMap.empty()) {
    nodeMap["0"] = &zeroNode;
    nodeMap["1"] = &oneNode;
    nodeMap["AND"] = &andNode;
    nodeMap["OR"] = &orNode;
    nodeMap["NOT"] = &notNode;
    nodeMap["C_COMPILER_ID"] = &cCompilerIdNode;
    nodeMap["CXX_COMPILER_ID"] = &cxxCompilerIdNode;
    nodeMap["VERSION_GREATER"] = &versionGreaterNode;
    nodeMap["VERSION_GREATER_EQUAL"] = &versionGreaterEqNode;
    nodeMap["VERSION_LESS"] = &versionLessNode;
    nodeMap["VERSION_LESS_EQUAL"] = &versionLessEqNode;
    nodeMap["VERSION_EQUAL"] = &versionEqualNode;
    nodeMap["C_COMPILER_VERSION"] = &cCompilerVersionNode;
    nodeMap["CXX_COMPILER_VERSION"] = &cxxCompilerVersionNode;
    nodeMap["PLATFORM_ID"] = &platformIdNode;
    nodeMap["COMPILE_FEATURES"] = &compileFeaturesNode;
    nodeMap["CONFIGURATION"] = &configurationNode;
    nodeMap["CONFIG"] = &configurationTestNode;
    nodeMap["TARGET_FILE"] = &targetNodeGroup.File;
    nodeMap["TARGET_LINKER_FILE"] = &targetLinkerNodeGroup.File;
    nodeMap["TARGET_SONAME_FILE"] = &targetSoNameNodeGroup.File;
    nodeMap["TARGET_PDB_FILE"] = &targetPdbNodeGroup.File;
    nodeMap["TARGET_FILE_NAME"] = &targetNodeGroup.FileName;
    nodeMap["TARGET_LINKER_FILE_NAME"] = &targetLinkerNodeGroup.FileName;
    nodeMap["TARGET_SONAME_FILE_NAME"] = &targetSoNameNodeGroup.FileName;
    nodeMap["TARGET_PDB_FILE_NAME"] = &targetPdbNodeGroup.FileName;
    nodeMap["TARGET_FILE_DIR"] = &targetNodeGroup.FileDir;
    nodeMap["TARGET_LINKER_FILE_DIR"] = &targetLinkerNodeGroup.FileDir;
    nodeMap["TARGET_SONAME_FILE_DIR"] = &targetSoNameNodeGroup.FileDir;
    nodeMap["TARGET_PDB_FILE_DIR"] = &targetPdbNodeGroup.FileDir;
    nodeMap["TARGET_BUNDLE_DIR"] = &targetBundleDirNode;
    nodeMap["TARGET_BUNDLE_CONTENT_DIR"] = &targetBundleContentDirNode;
    nodeMap["STREQUAL"] = &strEqualNode;
    nodeMap["EQUAL"] = &equalNode;
    nodeMap["LOWER_CASE"] = &lowerCaseNode;
    nodeMap["UPPER_CASE"] = &upperCaseNode;
    nodeMap["MAKE_C_IDENTIFIER"] = &makeCIdentifierNode;
    nodeMap["BOOL"] = &boolNode;
    nodeMap["IF"] = &ifNode;
    nodeMap["ANGLE-R"] = &angle_rNode;
    nodeMap["COMMA"] = &commaNode;
    nodeMap["SEMICOLON"] = &semicolonNode;
    nodeMap["TARGET_PROPERTY"] = &targetPropertyNode;
    nodeMap["TARGET_NAME"] = &targetNameNode;
    nodeMap["TARGET_OBJECTS"] = &targetObjectsNode;
    nodeMap["TARGET_POLICY"] = &targetPolicyNode;
    nodeMap["BUILD_INTERFACE"] = &buildInterfaceNode;
    nodeMap["INSTALL_INTERFACE"] = &installInterfaceNode;
    nodeMap["INSTALL_PREFIX"] = &installPrefixNode;
    nodeMap["JOIN"] = &joinNode;
    nodeMap["LINK_ONLY"] = &linkOnlyNode;
    nodeMap["COMPILE_LANGUAGE"] = &languageNode;
    nodeMap["SHELL_PATH"] = &shellPathNode;
  }
  NodeMap::const_iterator i = nodeMap.find(identifier);
  if (i == nodeMap.end()) {
    return CM_NULLPTR;
  }
  return i->second;
}

void reportError(cmGeneratorExpressionContext* context,
                 const std::string& expr, const std::string& result)
{
  context->HadError = true;
  if (context->Quiet) {
    return;
  }

  std::ostringstream e;
  /* clang-format off */
  e << "Error evaluating generator expression:\n"
    << "  " << expr << "\n"
    << result;
  /* clang-format on */
  context->LG->GetCMakeInstance()->IssueMessage(cmake::FATAL_ERROR, e.str(),
                                                context->Backtrace);
}
