/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpression.h"

#include "cmsys/RegularExpression.hxx"
#include <utility>

#include "assert.h"
#include "cmAlgorithms.h"
#include "cmGeneratorExpressionContext.h"
#include "cmGeneratorExpressionEvaluator.h"
#include "cmGeneratorExpressionLexer.h"
#include "cmGeneratorExpressionParser.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"

cmGeneratorExpression::cmGeneratorExpression(
  const cmListFileBacktrace& backtrace)
  : Backtrace(backtrace)
{
}

CM_AUTO_PTR<cmCompiledGeneratorExpression> cmGeneratorExpression::Parse(
  std::string const& input)
{
  return CM_AUTO_PTR<cmCompiledGeneratorExpression>(
    new cmCompiledGeneratorExpression(this->Backtrace, input));
}

CM_AUTO_PTR<cmCompiledGeneratorExpression> cmGeneratorExpression::Parse(
  const char* input)
{
  return this->Parse(std::string(input ? input : ""));
}

cmGeneratorExpression::~cmGeneratorExpression()
{
}

const char* cmCompiledGeneratorExpression::Evaluate(
  cmLocalGenerator* lg, const std::string& config, bool quiet,
  const cmGeneratorTarget* headTarget,
  cmGeneratorExpressionDAGChecker* dagChecker,
  std::string const& language) const
{
  return this->Evaluate(lg, config, quiet, headTarget, headTarget, dagChecker,
                        language);
}

const char* cmCompiledGeneratorExpression::Evaluate(
  cmLocalGenerator* lg, const std::string& config, bool quiet,
  const cmGeneratorTarget* headTarget, const cmGeneratorTarget* currentTarget,
  cmGeneratorExpressionDAGChecker* dagChecker,
  std::string const& language) const
{
  cmGeneratorExpressionContext context(
    lg, config, quiet, headTarget, currentTarget ? currentTarget : headTarget,
    this->EvaluateForBuildsystem, this->Backtrace, language);

  return this->EvaluateWithContext(context, dagChecker);
}

const char* cmCompiledGeneratorExpression::EvaluateWithContext(
  cmGeneratorExpressionContext& context,
  cmGeneratorExpressionDAGChecker* dagChecker) const
{
  if (!this->NeedsEvaluation) {
    return this->Input.c_str();
  }

  this->Output = "";

  std::vector<cmGeneratorExpressionEvaluator*>::const_iterator it =
    this->Evaluators.begin();
  const std::vector<cmGeneratorExpressionEvaluator*>::const_iterator end =
    this->Evaluators.end();

  for (; it != end; ++it) {
    this->Output += (*it)->Evaluate(&context, dagChecker);

    this->SeenTargetProperties.insert(context.SeenTargetProperties.begin(),
                                      context.SeenTargetProperties.end());
    if (context.HadError) {
      this->Output = "";
      break;
    }
  }

  this->MaxLanguageStandard = context.MaxLanguageStandard;

  if (!context.HadError) {
    this->HadContextSensitiveCondition = context.HadContextSensitiveCondition;
    this->HadHeadSensitiveCondition = context.HadHeadSensitiveCondition;
    this->SourceSensitiveTargets = context.SourceSensitiveTargets;
  }

  this->DependTargets = context.DependTargets;
  this->AllTargetsSeen = context.AllTargets;
  // TODO: Return a std::string from here instead?
  return this->Output.c_str();
}

cmCompiledGeneratorExpression::cmCompiledGeneratorExpression(
  cmListFileBacktrace const& backtrace, const std::string& input)
  : Backtrace(backtrace)
  , Input(input)
  , HadContextSensitiveCondition(false)
  , HadHeadSensitiveCondition(false)
  , EvaluateForBuildsystem(false)
{
  cmGeneratorExpressionLexer l;
  std::vector<cmGeneratorExpressionToken> tokens = l.Tokenize(this->Input);
  this->NeedsEvaluation = l.GetSawGeneratorExpression();

  if (this->NeedsEvaluation) {
    cmGeneratorExpressionParser p(tokens);
    p.Parse(this->Evaluators);
  }
}

cmCompiledGeneratorExpression::~cmCompiledGeneratorExpression()
{
  cmDeleteAll(this->Evaluators);
}

std::string cmGeneratorExpression::StripEmptyListElements(
  const std::string& input)
{
  if (input.find(';') == std::string::npos) {
    return input;
  }
  std::string result;
  result.reserve(input.size());

  const char* c = input.c_str();
  const char* last = c;
  bool skipSemiColons = true;
  for (; *c; ++c) {
    if (*c == ';') {
      if (skipSemiColons) {
        result.append(last, c - last);
        last = c + 1;
      }
      skipSemiColons = true;
    } else {
      skipSemiColons = false;
    }
  }
  result.append(last);

  if (!result.empty() && *(result.end() - 1) == ';') {
    result.resize(result.size() - 1);
  }

  return result;
}

static std::string stripAllGeneratorExpressions(const std::string& input)
{
  std::string result;
  std::string::size_type pos = 0;
  std::string::size_type lastPos = pos;
  int nestingLevel = 0;
  while ((pos = input.find("$<", lastPos)) != std::string::npos) {
    result += input.substr(lastPos, pos - lastPos);
    pos += 2;
    nestingLevel = 1;
    const char* c = input.c_str() + pos;
    const char* const cStart = c;
    for (; *c; ++c) {
      if (c[0] == '$' && c[1] == '<') {
        ++nestingLevel;
        ++c;
        continue;
      }
      if (c[0] == '>') {
        --nestingLevel;
        if (nestingLevel == 0) {
          break;
        }
      }
    }
    const std::string::size_type traversed = (c - cStart) + 1;
    if (!*c) {
      result += "$<" + input.substr(pos, traversed);
    }
    pos += traversed;
    lastPos = pos;
  }
  if (nestingLevel == 0) {
    result += input.substr(lastPos);
  }
  return cmGeneratorExpression::StripEmptyListElements(result);
}

static void prefixItems(const std::string& content, std::string& result,
                        const std::string& prefix)
{
  std::vector<std::string> entries;
  cmGeneratorExpression::Split(content, entries);
  const char* sep = "";
  for (std::vector<std::string>::const_iterator ei = entries.begin();
       ei != entries.end(); ++ei) {
    result += sep;
    sep = ";";
    if (!cmSystemTools::FileIsFullPath(ei->c_str()) &&
        cmGeneratorExpression::Find(*ei) != 0) {
      result += prefix;
    }
    result += *ei;
  }
}

static std::string stripExportInterface(
  const std::string& input, cmGeneratorExpression::PreprocessContext context,
  bool resolveRelative)
{
  std::string result;

  int nestingLevel = 0;
  std::string::size_type pos = 0;
  std::string::size_type lastPos = pos;
  while (true) {
    std::string::size_type bPos = input.find("$<BUILD_INTERFACE:", lastPos);
    std::string::size_type iPos = input.find("$<INSTALL_INTERFACE:", lastPos);

    if (bPos == std::string::npos && iPos == std::string::npos) {
      break;
    }

    if (bPos == std::string::npos) {
      pos = iPos;
    } else if (iPos == std::string::npos) {
      pos = bPos;
    } else {
      pos = (bPos < iPos) ? bPos : iPos;
    }

    result += input.substr(lastPos, pos - lastPos);
    const bool gotInstallInterface = input[pos + 2] == 'I';
    pos += gotInstallInterface ? sizeof("$<INSTALL_INTERFACE:") - 1
                               : sizeof("$<BUILD_INTERFACE:") - 1;
    nestingLevel = 1;
    const char* c = input.c_str() + pos;
    const char* const cStart = c;
    for (; *c; ++c) {
      if (c[0] == '$' && c[1] == '<') {
        ++nestingLevel;
        ++c;
        continue;
      }
      if (c[0] == '>') {
        --nestingLevel;
        if (nestingLevel != 0) {
          continue;
        }
        if (context == cmGeneratorExpression::BuildInterface &&
            !gotInstallInterface) {
          result += input.substr(pos, c - cStart);
        } else if (context == cmGeneratorExpression::InstallInterface &&
                   gotInstallInterface) {
          const std::string content = input.substr(pos, c - cStart);
          if (resolveRelative) {
            prefixItems(content, result, "${_IMPORT_PREFIX}/");
          } else {
            result += content;
          }
        }
        break;
      }
    }
    const std::string::size_type traversed = (c - cStart) + 1;
    if (!*c) {
      result += std::string(gotInstallInterface ? "$<INSTALL_INTERFACE:"
                                                : "$<BUILD_INTERFACE:") +
        input.substr(pos, traversed);
    }
    pos += traversed;
    lastPos = pos;
  }
  if (nestingLevel == 0) {
    result += input.substr(lastPos);
  }

  return cmGeneratorExpression::StripEmptyListElements(result);
}

void cmGeneratorExpression::Split(const std::string& input,
                                  std::vector<std::string>& output)
{
  std::string::size_type pos = 0;
  std::string::size_type lastPos = pos;
  while ((pos = input.find("$<", lastPos)) != std::string::npos) {
    std::string part = input.substr(lastPos, pos - lastPos);
    std::string preGenex;
    if (!part.empty()) {
      std::string::size_type startPos = input.rfind(';', pos);
      if (startPos == std::string::npos) {
        preGenex = part;
        part = "";
      } else if (startPos != pos - 1 && startPos >= lastPos) {
        part = input.substr(lastPos, startPos - lastPos);
        preGenex = input.substr(startPos + 1, pos - startPos - 1);
      }
      if (!part.empty()) {
        cmSystemTools::ExpandListArgument(part, output);
      }
    }
    pos += 2;
    int nestingLevel = 1;
    const char* c = input.c_str() + pos;
    const char* const cStart = c;
    for (; *c; ++c) {
      if (c[0] == '$' && c[1] == '<') {
        ++nestingLevel;
        ++c;
        continue;
      }
      if (c[0] == '>') {
        --nestingLevel;
        if (nestingLevel == 0) {
          break;
        }
      }
    }
    for (; *c; ++c) {
      // Capture the part after the genex and before the next ';'
      if (c[0] == ';') {
        --c;
        break;
      }
    }
    const std::string::size_type traversed = (c - cStart) + 1;
    output.push_back(preGenex + "$<" + input.substr(pos, traversed));
    pos += traversed;
    lastPos = pos;
  }
  if (lastPos < input.size()) {
    cmSystemTools::ExpandListArgument(input.substr(lastPos), output);
  }
}

std::string cmGeneratorExpression::Preprocess(const std::string& input,
                                              PreprocessContext context,
                                              bool resolveRelative)
{
  if (context == StripAllGeneratorExpressions) {
    return stripAllGeneratorExpressions(input);
  }
  if (context == BuildInterface || context == InstallInterface) {
    return stripExportInterface(input, context, resolveRelative);
  }

  assert(false &&
         "cmGeneratorExpression::Preprocess called with invalid args");
  return std::string();
}

std::string::size_type cmGeneratorExpression::Find(const std::string& input)
{
  const std::string::size_type openpos = input.find("$<");
  if (openpos != std::string::npos &&
      input.find('>', openpos) != std::string::npos) {
    return openpos;
  }
  return std::string::npos;
}

bool cmGeneratorExpression::IsValidTargetName(const std::string& input)
{
  // The ':' is supported to allow use with IMPORTED targets. At least
  // Qt 4 and 5 IMPORTED targets use ':' as the namespace delimiter.
  static cmsys::RegularExpression targetNameValidator("^[A-Za-z0-9_.:+-]+$");

  return targetNameValidator.find(input);
}

void cmCompiledGeneratorExpression::GetMaxLanguageStandard(
  const cmGeneratorTarget* tgt, std::map<std::string, std::string>& mapping)
{
  typedef std::map<cmGeneratorTarget const*,
                   std::map<std::string, std::string> >
    MapType;
  MapType::const_iterator it = this->MaxLanguageStandard.find(tgt);
  if (it != this->MaxLanguageStandard.end()) {
    mapping = it->second;
  }
}
