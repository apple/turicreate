/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionContext.h"

cmGeneratorExpressionContext::cmGeneratorExpressionContext(
  cmLocalGenerator* lg, std::string const& config, bool quiet,
  cmGeneratorTarget const* headTarget, const cmGeneratorTarget* currentTarget,
  bool evaluateForBuildsystem, cmListFileBacktrace const& backtrace,
  std::string const& language)
  : Backtrace(backtrace)
  , LG(lg)
  , Config(config)
  , Language(language)
  , HeadTarget(headTarget)
  , CurrentTarget(currentTarget)
  , Quiet(quiet)
  , HadError(false)
  , HadContextSensitiveCondition(false)
  , HadHeadSensitiveCondition(false)
  , EvaluateForBuildsystem(evaluateForBuildsystem)
{
}
