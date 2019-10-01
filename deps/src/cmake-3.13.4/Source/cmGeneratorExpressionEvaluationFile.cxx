/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorExpressionEvaluationFile.h"

#include "cmsys/FStream.hxx"
#include <memory> // IWYU pragma: keep
#include <sstream>
#include <utility>

#include "cmGeneratedFileStream.h"
#include "cmGlobalGenerator.h"
#include "cmListFileCache.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmSourceFileLocationKind.h"
#include "cmSystemTools.h"
#include "cmake.h"

cmGeneratorExpressionEvaluationFile::cmGeneratorExpressionEvaluationFile(
  const std::string& input,
  std::unique_ptr<cmCompiledGeneratorExpression> outputFileExpr,
  std::unique_ptr<cmCompiledGeneratorExpression> condition,
  bool inputIsContent, cmPolicies::PolicyStatus policyStatusCMP0070)
  : Input(input)
  , OutputFileExpr(std::move(outputFileExpr))
  , Condition(std::move(condition))
  , InputIsContent(inputIsContent)
  , PolicyStatusCMP0070(policyStatusCMP0070)
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
      lg, config, false, nullptr, nullptr, nullptr, lang);
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

  std::string outputFileName = this->OutputFileExpr->Evaluate(
    lg, config, false, nullptr, nullptr, nullptr, lang);
  const std::string& outputContent = inputExpression->Evaluate(
    lg, config, false, nullptr, nullptr, nullptr, lang);

  if (cmSystemTools::FileIsFullPath(outputFileName)) {
    outputFileName = cmSystemTools::CollapseFullPath(outputFileName);
  } else {
    outputFileName = this->FixRelativePath(outputFileName, PathForOutput, lg);
  }

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

  cmGeneratedFileStream fout(outputFileName);
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

  for (std::string const& le : enabledLanguages) {
    std::string name = this->OutputFileExpr->Evaluate(
      lg, config, false, nullptr, nullptr, nullptr, le);
    cmSourceFile* sf = lg->GetMakefile()->GetOrCreateSource(
      name, false, cmSourceFileLocationKind::Known);
    // Tell TraceDependencies that the file is not expected to exist
    // on disk yet.  We generate it after that runs.
    sf->SetProperty("GENERATED", "1");

    // Tell the build system generators that there is no build rule
    // to generate the file.
    sf->SetProperty("__CMAKE_GENERATED_BY_CMAKE", "1");

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
    std::string inputFileName = this->Input;
    if (cmSystemTools::FileIsFullPath(inputFileName)) {
      inputFileName = cmSystemTools::CollapseFullPath(inputFileName);
    } else {
      inputFileName = this->FixRelativePath(inputFileName, PathForInput, lg);
    }
    lg->GetMakefile()->AddCMakeDependFile(inputFileName);
    cmSystemTools::GetPermissions(inputFileName.c_str(), perm);
    cmsys::ifstream fin(inputFileName.c_str());
    if (!fin) {
      std::ostringstream e;
      e << "Evaluation file \"" << inputFileName << "\" cannot be read.";
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
  std::unique_ptr<cmCompiledGeneratorExpression> inputExpression =
    contentGE.Parse(inputContent);

  std::map<std::string, std::string> outputFiles;

  std::vector<std::string> allConfigs;
  lg->GetMakefile()->GetConfigurations(allConfigs);

  if (allConfigs.empty()) {
    allConfigs.emplace_back();
  }

  std::vector<std::string> enabledLanguages;
  cmGlobalGenerator* gg = lg->GetGlobalGenerator();
  gg->GetEnabledLanguages(enabledLanguages);

  for (std::string const& le : enabledLanguages) {
    for (std::string const& li : allConfigs) {
      this->Generate(lg, li, le, inputExpression.get(), outputFiles, perm);
      if (cmSystemTools::GetFatalErrorOccured()) {
        return;
      }
    }
  }
}

std::string cmGeneratorExpressionEvaluationFile::FixRelativePath(
  std::string const& relativePath, PathRole role, cmLocalGenerator* lg)
{
  std::string resultPath;
  switch (this->PolicyStatusCMP0070) {
    case cmPolicies::WARN: {
      std::string arg;
      switch (role) {
        case PathForInput:
          arg = "INPUT";
          break;
        case PathForOutput:
          arg = "OUTPUT";
          break;
      }
      std::ostringstream w;
      /* clang-format off */
      w <<
        cmPolicies::GetPolicyWarning(cmPolicies::CMP0070) << "\n"
        "file(GENERATE) given relative " << arg << " path:\n"
        "  " << relativePath << "\n"
        "This is not defined behavior unless CMP0070 is set to NEW.  "
        "For compatibility with older versions of CMake, the previous "
        "undefined behavior will be used."
        ;
      /* clang-format on */
      lg->IssueMessage(cmake::AUTHOR_WARNING, w.str());
    }
      CM_FALLTHROUGH;
    case cmPolicies::OLD:
      // OLD behavior is to use the relative path unchanged,
      // which ends up being used relative to the working dir.
      resultPath = relativePath;
      break;
    case cmPolicies::REQUIRED_IF_USED:
    case cmPolicies::REQUIRED_ALWAYS:
    case cmPolicies::NEW:
      // NEW behavior is to interpret the relative path with respect
      // to the current source or binary directory.
      switch (role) {
        case PathForInput:
          resultPath = cmSystemTools::CollapseFullPath(
            relativePath, lg->GetCurrentSourceDirectory());
          break;
        case PathForOutput:
          resultPath = cmSystemTools::CollapseFullPath(
            relativePath, lg->GetCurrentBinaryDirectory());
          break;
      }
      break;
  }
  return resultPath;
}
