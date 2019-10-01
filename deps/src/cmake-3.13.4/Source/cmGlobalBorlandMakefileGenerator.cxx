/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalBorlandMakefileGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmake.h"

cmGlobalBorlandMakefileGenerator::cmGlobalBorlandMakefileGenerator(cmake* cm)
  : cmGlobalUnixMakefileGenerator3(cm)
{
  this->EmptyRuleHackDepends = "NUL";
  this->FindMakeProgramFile = "CMakeBorlandFindMake.cmake";
  this->ForceUnixPaths = false;
  this->ToolSupportsColor = true;
  this->UseLinkScript = false;
  cm->GetState()->SetWindowsShell(true);
  this->IncludeDirective = "!include";
  this->DefineWindowsNULL = true;
  this->PassMakeflags = true;
  this->UnixCD = false;
}

void cmGlobalBorlandMakefileGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
  std::string outdir = this->CMakeInstance->GetHomeOutputDirectory();
  mf->AddDefinition("BORLAND", "1");
  mf->AddDefinition("CMAKE_GENERATOR_CC", "bcc32");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "bcc32");
  this->cmGlobalUnixMakefileGenerator3::EnableLanguage(l, mf, optional);
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator* cmGlobalBorlandMakefileGenerator::CreateLocalGenerator(
  cmMakefile* mf)
{
  cmLocalUnixMakefileGenerator3* lg =
    new cmLocalUnixMakefileGenerator3(this, mf);
  lg->SetMakefileVariableSize(32);
  lg->SetMakeCommandEscapeTargetTwice(true);
  lg->SetBorlandMakeCurlyHack(true);
  return lg;
}

void cmGlobalBorlandMakefileGenerator::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalBorlandMakefileGenerator::GetActualName();
  entry.Brief = "Generates Borland makefiles.";
}

void cmGlobalBorlandMakefileGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& projectDir,
  const std::string& targetName, const std::string& config, bool fast,
  int /*jobs*/, bool verbose, std::vector<std::string> const& makeOptions)
{
  this->cmGlobalUnixMakefileGenerator3::GenerateBuildCommand(
    makeCommand, makeProgram, projectName, projectDir, targetName, config,
    fast, cmake::NO_BUILD_PARALLEL_LEVEL, verbose, makeOptions);
}

void cmGlobalBorlandMakefileGenerator::PrintBuildCommandAdvice(
  std::ostream& os, int jobs) const
{
  if (jobs != cmake::NO_BUILD_PARALLEL_LEVEL) {
    // Borland's make does not support parallel builds
    // see http://docwiki.embarcadero.com/RADStudio/Tokyo/en/Make

    /* clang-format off */
    os <<
      "Warning: Borland's make does not support parallel builds. "
      "Ignoring parallel build command line option.\n";
    /* clang-format on */
  }

  this->cmGlobalUnixMakefileGenerator3::PrintBuildCommandAdvice(
    os, cmake::NO_BUILD_PARALLEL_LEVEL);
}
