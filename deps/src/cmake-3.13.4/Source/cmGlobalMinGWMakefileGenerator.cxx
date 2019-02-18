/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalMinGWMakefileGenerator.h"

#include "cmDocumentationEntry.h"
#include "cmLocalUnixMakefileGenerator3.h"
#include "cmMakefile.h"
#include "cmState.h"

cmGlobalMinGWMakefileGenerator::cmGlobalMinGWMakefileGenerator(cmake* cm)
  : cmGlobalUnixMakefileGenerator3(cm)
{
  this->FindMakeProgramFile = "CMakeMinGWFindMake.cmake";
  this->ForceUnixPaths = true;
  this->ToolSupportsColor = true;
  this->UseLinkScript = true;
  cm->GetState()->SetWindowsShell(true);
  cm->GetState()->SetMinGWMake(true);
}

void cmGlobalMinGWMakefileGenerator::EnableLanguage(
  std::vector<std::string> const& l, cmMakefile* mf, bool optional)
{
  this->FindMakeProgram(mf);
  std::string makeProgram = mf->GetRequiredDefinition("CMAKE_MAKE_PROGRAM");
  std::vector<std::string> locations;
  locations.push_back(cmSystemTools::GetProgramPath(makeProgram));
  locations.push_back("/mingw/bin");
  locations.push_back("c:/mingw/bin");
  std::string tgcc = cmSystemTools::FindProgram("gcc", locations);
  std::string gcc = "gcc.exe";
  if (!tgcc.empty()) {
    gcc = tgcc;
  }
  std::string tgxx = cmSystemTools::FindProgram("g++", locations);
  std::string gxx = "g++.exe";
  if (!tgxx.empty()) {
    gxx = tgxx;
  }
  std::string trc = cmSystemTools::FindProgram("windres", locations);
  std::string rc = "windres.exe";
  if (!trc.empty()) {
    rc = trc;
  }
  mf->AddDefinition("CMAKE_GENERATOR_CC", gcc.c_str());
  mf->AddDefinition("CMAKE_GENERATOR_CXX", gxx.c_str());
  mf->AddDefinition("CMAKE_GENERATOR_RC", rc.c_str());
  this->cmGlobalUnixMakefileGenerator3::EnableLanguage(l, mf, optional);
}

void cmGlobalMinGWMakefileGenerator::GetDocumentation(
  cmDocumentationEntry& entry)
{
  entry.Name = cmGlobalMinGWMakefileGenerator::GetActualName();
  entry.Brief = "Generates a make file for use with mingw32-make.";
}
