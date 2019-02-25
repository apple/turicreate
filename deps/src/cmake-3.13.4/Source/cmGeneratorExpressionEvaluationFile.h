/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionEvaluationFile_h
#define cmGeneratorExpressionEvaluationFile_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <memory> // IWYU pragma: keep
#include <string>
#include <vector>

#include "cmGeneratorExpression.h"
#include "cmPolicies.h"
#include "cm_sys_stat.h"

class cmLocalGenerator;

class cmGeneratorExpressionEvaluationFile
{
public:
  cmGeneratorExpressionEvaluationFile(
    const std::string& input,
    std::unique_ptr<cmCompiledGeneratorExpression> outputFileExpr,
    std::unique_ptr<cmCompiledGeneratorExpression> condition,
    bool inputIsContent, cmPolicies::PolicyStatus policyStatusCMP0070);

  void Generate(cmLocalGenerator* lg);

  std::vector<std::string> GetFiles() const { return this->Files; }

  void CreateOutputFile(cmLocalGenerator* lg, std::string const& config);

private:
  void Generate(cmLocalGenerator* lg, const std::string& config,
                const std::string& lang,
                cmCompiledGeneratorExpression* inputExpression,
                std::map<std::string, std::string>& outputFiles, mode_t perm);

  enum PathRole
  {
    PathForInput,
    PathForOutput
  };
  std::string FixRelativePath(std::string const& filePath, PathRole role,
                              cmLocalGenerator* lg);

private:
  const std::string Input;
  const std::unique_ptr<cmCompiledGeneratorExpression> OutputFileExpr;
  const std::unique_ptr<cmCompiledGeneratorExpression> Condition;
  std::vector<std::string> Files;
  const bool InputIsContent;
  cmPolicies::PolicyStatus PolicyStatusCMP0070;
};

#endif
