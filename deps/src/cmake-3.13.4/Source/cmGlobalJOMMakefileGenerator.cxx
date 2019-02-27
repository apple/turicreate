/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalJOMMakefileGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmState.h"

cmGlobalJOMMakefileGenerator::cmGlobalJOMMakefileGenerator(cmake* cm)
  : cmGlobalUnixMakefileGenerator3(cm)
{
  this->FindMakeProgramFile = "CMakeJOMFindMake.cmake";
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

void cmGlobalJOMMakefileGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
  // pick a default
  mf->AddDefinition("CMAKE_GENERATOR_CC", "cl");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "cl");
  this->cmGlobalUnixMakefileGenerator3::EnableLanguage(l, mf, optional);
}

void cmGlobalJOMMakefileGenerator::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalJOMMakefileGenerator::GetActualName();
  entry.Brief = "Generates JOM makefiles.";
}

void cmGlobalJOMMakefileGenerator::PrintCompilerAdvice(
  std::ostream& os, std::string const& lang, const char* envVar) const
{
  if (lang == "CXX" || lang == "C") {
    /* clang-format off */
    os <<
      "To use the JOM generator with Visual C++, cmake must be run from a "
      "shell that can use the compiler cl from the command line. This "
      "environment is unable to invoke the cl compiler. To fix this problem, "
      "run cmake from the Visual Studio Command Prompt (vcvarsall.bat).\n";
    /* clang-format on */
  }
  this->cmGlobalUnixMakefileGenerator3::PrintCompilerAdvice(os, lang, envVar);
}

void cmGlobalJOMMakefileGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& projectDir,
  const std::string& targetName, const std::string& config, bool fast,
  int jobs, bool verbose, std::vector<std::string> const& makeOptions)
{
  std::vector<std::string> jomMakeOptions;

  // Since we have full control over the invocation of JOM, let us
  // make it quiet.
  jomMakeOptions.push_back(this->MakeSilentFlag);
  jomMakeOptions.insert(jomMakeOptions.end(), makeOptions.begin(),
                        makeOptions.end());

  // JOM does parallel builds by default, the -j is only needed if a specific
  // number is given
  // see https://github.com/qt-labs/jom/blob/v1.1.2/src/jomlib/options.cpp
  if (jobs == cmake::DEFAULT_BUILD_PARALLEL_LEVEL) {
    jobs = cmake::NO_BUILD_PARALLEL_LEVEL;
  }

  cmGlobalUnixMakefileGenerator3::GenerateBuildCommand(
    makeCommand, makeProgram, projectName, projectDir, targetName, config,
    fast, jobs, verbose, jomMakeOptions);
}
