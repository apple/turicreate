/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalNMakeMakefileGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmState.h"

cmGlobalNMakeMakefileGenerator::cmGlobalNMakeMakefileGenerator(cmake* cm)
  : cmGlobalUnixMakefileGenerator3(cm)
{
  this->FindMakeProgramFile = "CMakeNMakeFindMake.cmake";
  this->ForceUnixPaths = false;
  this->ToolSupportsColor = true;
  this->UseLinkScript = false;
  cm->GetState()->SetWindowsShell(true);
  cm->GetState()->SetNMake(true);
  this->DefineWindowsNULL = true;
  this->PassMakeflags = true;
  this->UnixCD = false;
  this->MakeSilentFlag = "/nologo";
}

void cmGlobalNMakeMakefileGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
  // pick a default
  mf->AddDefinition("CMAKE_GENERATOR_CC", "cl");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "cl");
  this->cmGlobalUnixMakefileGenerator3::EnableLanguage(l, mf, optional);
}

void cmGlobalNMakeMakefileGenerator::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalNMakeMakefileGenerator::GetActualName();
  entry.Brief = "Generates NMake makefiles.";
}

void cmGlobalNMakeMakefileGenerator::PrintCompilerAdvice(
  std::ostream& os, std::string const& lang, const char* envVar) const
{
  if (lang == "CXX" || lang == "C") {
    /* clang-format off */
    os <<
      "To use the NMake generator with Visual C++, cmake must be run from a "
      "shell that can use the compiler cl from the command line. This "
      "environment is unable to invoke the cl compiler. To fix this problem, "
      "run cmake from the Visual Studio Command Prompt (vcvarsall.bat).\n";
    /* clang-format on */
  }
  this->cmGlobalUnixMakefileGenerator3::PrintCompilerAdvice(os, lang, envVar);
}

void cmGlobalNMakeMakefileGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& projectDir,
  const std::string& targetName, const std::string& config, bool fast,
  int /*jobs*/, bool verbose, std::vector<std::string> const& makeOptions)
{
  std::vector<std::string> nmakeMakeOptions;

  // Since we have full control over the invocation of nmake, let us
  // make it quiet.
  nmakeMakeOptions.push_back(this->MakeSilentFlag);
  nmakeMakeOptions.insert(nmakeMakeOptions.end(), makeOptions.begin(),
                          makeOptions.end());

  this->cmGlobalUnixMakefileGenerator3::GenerateBuildCommand(
    makeCommand, makeProgram, projectName, projectDir, targetName, config,
    fast, cmake::NO_BUILD_PARALLEL_LEVEL, verbose, nmakeMakeOptions);
}

void cmGlobalNMakeMakefileGenerator::PrintBuildCommandAdvice(std::ostream& os,
                                                             int jobs) const
{
  if (jobs != cmake::NO_BUILD_PARALLEL_LEVEL) {
    // nmake does not support parallel build level
    // see https://msdn.microsoft.com/en-us/library/afyyse50.aspx

    /* clang-format off */
    os <<
      "Warning: NMake does not support parallel builds. "
      "Ignoring parallel build command line option.\n";
    /* clang-format on */
  }

  this->cmGlobalUnixMakefileGenerator3::PrintBuildCommandAdvice(
    os, cmake::NO_BUILD_PARALLEL_LEVEL);
}
