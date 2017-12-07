/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionEvaluationFile.h"

#include "cmConfigure.h"
#include "cmsys/FStream.hxx"
#include <sstream>
#include <utility>

#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmListFileCache.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSystemTools.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

cmGeneratorExpressionEvaluationFile::cmGeneratorExpressionEvaluationFile(
  const std::string& input,
  CM_AUTO_PTR<cmCompiledGeneratorExpression> outputFileExpr,
  CM_AUTO_PTR<cmCompiledGeneratorExpression> condition, bool inputIsContent)
  : Input(input)
  , OutputFileExpr(outputFileExpr)
  , Condition(condition)
  , InputIsContent(inputIsContent)
{
}

void cmGeneratorExpressionEvaluationFile::Generate(
  cmLocalGenerator* lg, const std::string& config, const std::string& lang,
  cmCompiledGeneratorExpression* inputExpression,
  std::map<std::string, std::string>& outputFiles, mode_t perm)
{
  std::string rawCondition = this->Condition->GetInput();
  if (!rawCondition.empty()) {
    std::string condResult = this->Condition->Evaluate(
      lg, config, false, CM_NULLPTR, CM_NULLPTR, CM_NULLPTR, lang);
    if (condResult == "0") {
      return;
    }
    if (condResult != "1") {
      std::ostringstream e;
      e << "Evaluation file condition \"" << rawCondition
        << "\" did "
           "not evaluate to valid content. Got \""
        << condResult << "\".";
      lg->IssueMessage(cmake::FATAL_ERROR, e.str());
      return;
    }
  }

  const std::string outputFileName = this->OutputFileExpr->Evaluate(
    lg, config, false, CM_NULLPTR, CM_NULLPTR, CM_NULLPTR, lang);
  const std::string outputContent = inputExpression->Evaluate(
    lg, config, false, CM_NULLPTR, CM_NULLPTR, CM_NULLPTR, lang);

  std::map<std::string, std::string>::iterator it =
    outputFiles.find(outputFileName);

  if (it != outputFiles.end()) {
    if (it->second == outputContent) {
      return;
    }
    std::ostringstream e;
    e << "Evaluation file to be written multiple times with different "
         "content. "
         "This is generally caused by the content evaluating the "
         "configuration type, language, or location of object files:\n "
      << outputFileName;
    lg->IssueMessage(cmake::FATAL_ERROR, e.str());
    return;
  }

  lg->GetMakefile()->AddCMakeOutputFile(outputFileName);
  this->Files.push_back(outputFileName);
  outputFiles[outputFileName] = outputContent;

  cmGeneratedFileStream fout(outputFileName.c_str());
  fout.SetCopyIfDifferent(true);
  fout << outputContent;
  if (fout.Close() && perm) {
    cmSystemTools::SetPermissions(outputFileName.c_str(), perm);
  }
}

void cmGeneratorExpressionEvaluationFile::CreateOutputFile(
  cmLocalGenerator* lg, std::string const& config)
{
  std::vector<std::string> enabledLanguages;
  cmGlobalGenerator* gg = lg->GetGlobalGenerator();
  gg->GetEnabledLanguages(enabledLanguages);

  for (std::vector<std::string>::const_iterator le = enabledLanguages.begin();
       le != enabledLanguages.end(); ++le) {
    std::string name = this->OutputFileExpr->Evaluate(
      lg, config, false, CM_NULLPTR, CM_NULLPTR, CM_NULLPTR, *le);
    cmSourceFile* sf = lg->GetMakefile()->GetOrCreateSource(name);
    sf->SetProperty("GENERATED", "1");

    gg->SetFilenameTargetDepends(
      sf, this->OutputFileExpr->GetSourceSensitiveTargets());
  }
}

void cmGeneratorExpressionEvaluationFile::Generate(cmLocalGenerator* lg)
{
  mode_t perm = 0;
  std::string inputContent;
  if (this->InputIsContent) {
    inputContent = this->Input;
  } else {
    lg->GetMakefile()->AddCMakeDependFile(this->Input);
    cmSystemTools::GetPermissions(this->Input.c_str(), perm);
    cmsys::ifstream fin(this->Input.c_str());
    if (!fin) {
      std::ostringstream e;
      e << "Evaluation file \"" << this->Input << "\" cannot be read.";
      lg->IssueMessage(cmake::FATAL_ERROR, e.str());
      return;
    }

    std::string line;
    std::string sep;
    while (cmSystemTools::GetLineFromStream(fin, line)) {
      inputContent += sep + line;
      sep = "\n";
    }
    inputContent += sep;
  }

  cmListFileBacktrace lfbt = this->OutputFileExpr->GetBacktrace();
  cmGeneratorExpression contentGE(lfbt);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> inputExpression =
    contentGE.Parse(inputContent);

  std::map<std::string, std::string> outputFiles;

  std::vector<std::string> allConfigs;
  lg->GetMakefile()->GetConfigurations(allConfigs);

  if (allConfigs.empty()) {
    allConfigs.push_back("");
  }

  std::vector<std::string> enabledLanguages;
  cmGlobalGenerator* gg = lg->GetGlobalGenerator();
  gg->GetEnabledLanguages(enabledLanguages);

  for (std::vector<std::string>::const_iterator le = enabledLanguages.begin();
       le != enabledLanguages.end(); ++le) {
    for (std::vector<std::string>::const_iterator li = allConfigs.begin();
         li != allConfigs.end(); ++li) {
      this->Generate(lg, *li, *le, inputExpression.get(), outputFiles, perm);
      if (cmSystemTools::GetFatalErrorOccured()) {
        return;
      }
    }
  }
}
