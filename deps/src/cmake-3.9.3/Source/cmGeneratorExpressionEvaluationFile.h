/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorExpressionEvaluationFile_h
#define cmGeneratorExpressionEvaluationFile_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

#include "cmGeneratorExpression.h"
#include "cm_auto_ptr.hxx"
#include "cm_sys_stat.h"

class cmLocalGenerator;

class cmGeneratorExpressionEvaluationFile
{
public:
  cmGeneratorExpressionEvaluationFile(
    const std::string& input,
    CM_AUTO_PTR<cmCompiledGeneratorExpression> outputFileExpr,
    CM_AUTO_PTR<cmCompiledGeneratorExpression> condition, bool inputIsContent);

  void Generate(cmLocalGenerator* lg);

  std::vector<std::string> GetFiles() const { return this->Files; }

  void CreateOutputFile(cmLocalGenerator* lg, std::string const& config);

private:
  void Generate(cmLocalGenerator* lg, const std::string& config,
                const std::string& lang,
                cmCompiledGeneratorExpression* inputExpression,
                std::map<std::string, std::string>& outputFiles, mode_t perm);

private:
  const std::string Input;
  const CM_AUTO_PTR<cmCompiledGeneratorExpression> OutputFileExpr;
  const CM_AUTO_PTR<cmCompiledGeneratorExpression> Condition;
  std::vector<std::string> Files;
  const bool InputIsContent;
};

#endif
