/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalWatcomWMakeGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmake.h"

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
