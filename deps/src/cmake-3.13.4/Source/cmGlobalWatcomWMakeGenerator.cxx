/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalWatcomWMakeGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmake.h"

#include <ostream>

cmGlobalWatcomWMakeGenerator::cmGlobalWatcomWMakeGenerator(cmake* cm)
  : cmGlobalUnixMakefileGenerator3(cm)
{
  this->FindMakeProgramFile = "CMakeFindWMake.cmake";
#ifdef _WIN32
  this->ForceUnixPaths = false;
#endif
  this->ToolSupportsColor = true;
  this->NeedSymbolicMark = true;
  this->EmptyRuleHackCommand = "@cd .";
#ifdef _WIN32
  cm->GetState()->SetWindowsShell(true);
#endif
  cm->GetState()->SetWatcomWMake(true);
  this->IncludeDirective = "!include";
  this->DefineWindowsNULL = true;
  this->UnixCD = false;
  this->MakeSilentFlag = "-h";
}

void cmGlobalWatcomWMakeGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
  // pick a default
  mf->AddDefinition("WATCOM", "1");
  mf->AddDefinition("CMAKE_QUOTE_INCLUDE_PATHS", "1");
  mf->AddDefinition("CMAKE_MANGLE_OBJECT_FILE_NAMES", "1");
  mf->AddDefinition("CMAKE_MAKE_LINE_CONTINUE", "&");
  mf->AddDefinition("CMAKE_MAKE_SYMBOLIC_RULE", ".SYMBOLIC");
  mf->AddDefinition("CMAKE_GENERATOR_CC", "wcl386");
  mf->AddDefinition("CMAKE_GENERATOR_CXX", "wcl386");
  this->cmGlobalUnixMakefileGenerator3::EnableLanguage(l, mf, optional);
}

void cmGlobalWatcomWMakeGenerator::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalWatcomWMakeGenerator::GetActualName();
  entry.Brief = "Generates Watcom WMake makefiles.";
}

void cmGlobalWatcomWMakeGenerator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& projectDir,
  const std::string& targetName, const std::string& config, bool fast,
  int /*jobs*/, bool verbose, std::vector<std::string> const& makeOptions)
{
  this->cmGlobalUnixMakefileGenerator3::GenerateBuildCommand(
    makeCommand, makeProgram, projectName, projectDir, targetName, config,
    fast, cmake::NO_BUILD_PARALLEL_LEVEL, verbose, makeOptions);
}

void cmGlobalWatcomWMakeGenerator::PrintBuildCommandAdvice(std::ostream& os,
                                                           int jobs) const
{
  if (jobs != cmake::NO_BUILD_PARALLEL_LEVEL) {
    // wmake does not support parallel build level

    /* clang-format off */
    os <<
      "Warning: Watcom's WMake does not support parallel builds. "
      "Ignoring parallel build command line option.\n";
    /* clang-format on */
  }

  this->cmGlobalUnixMakefileGenerator3::PrintBuildCommandAdvice(
    os, cmake::NO_BUILD_PARALLEL_LEVEL);
}
