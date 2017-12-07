/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio15Generator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmVS141CLFlagTable.h"
#include "cmVS141CSharpFlagTable.h"
#include "cmVS141LinkFlagTable.h"
#include "cmVSSetupHelper.h"

static const char vs15generatorName[] = "Visual Studio 15 2017";

// Map generator name without year to name with year.
static const char* cmVS15GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs15generatorName,
              sizeof(vs15generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs15generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2017")) {
    p += 5;
  }
  genName = std::string(vs15generatorName) + p;
  return p;
}

class cmGlobalVisualStudio15Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  virtual cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                                   cmake* cm) const
  {
    std::string genName;
    const char* p = cmVS15GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "x64");
    }
    if (strcmp(p, "ARM") == 0) {
      return new cmGlobalVisualStudio15Generator(cm, genName, "ARM");
    }
    return 0;
  }

  virtual void GetDocumentation(cmDocumentationEntry& entry) const
  {
    entry.Name = std::string(vs15generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2017 project files.  "
                  "Optional [arch] can be \"Win64\" or \"ARM\".";
  }

  virtual void GetGenerators(std::vector<std::string>& names) const
  {
    names.push_back(vs15generatorName);
    names.push_back(vs15generatorName + std::string(" ARM"));
    names.push_back(vs15generatorName + std::string(" Win64"));
  }

  bool SupportsToolset() const CM_OVERRIDE { return true; }
  bool SupportsPlatform() const CM_OVERRIDE { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio15Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio15Generator::cmGlobalVisualStudio15Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio14Generator(cm, name, platformName)
{
  this->ExpressEdition = false;
  this->DefaultPlatformToolset = "v141";
  this->DefaultClFlagTable = cmVS141CLFlagTable;
  this->DefaultCSharpFlagTable = cmVS141CSharpFlagTable;
  this->DefaultLinkFlagTable = cmVS141LinkFlagTable;
  this->Version = VS15;
}

bool cmGlobalVisualStudio15Generator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  if (cmVS15GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

void cmGlobalVisualStudio15Generator::WriteSLNHeader(std::ostream& fout)
{
  // Visual Studio 15 writes .sln format 12.00
  fout << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
  if (this->ExpressEdition) {
    fout << "# Visual Studio Express 15 for Windows Desktop\n";
  } else {
    fout << "# Visual Studio 15\n";
  }
}

bool cmGlobalVisualStudio15Generator::InitializeWindows(cmMakefile* mf)
{
  // If the Win 8.1 SDK is installed then we can select a SDK matching
  // the target Windows version.
  if (this->IsWin81SDKInstalled()) {
    return cmGlobalVisualStudio14Generator::InitializeWindows(mf);
  }
  // Otherwise we must choose a Win 10 SDK even if we are not targeting
  // Windows 10.
  return this->SelectWindows10SDK(mf, false);
}

bool cmGlobalVisualStudio15Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v141"; // VS 15 uses v141 toolset
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio14Generator::SelectWindowsStoreToolset(
    toolset);
}

bool cmGlobalVisualStudio15Generator::IsWindowsDesktopToolsetInstalled() const
{
  return vsSetupAPIHelper.IsVS2017Installed();
}

bool cmGlobalVisualStudio15Generator::IsWindowsStoreToolsetInstalled() const
{
  return vsSetupAPIHelper.IsWin10SDKInstalled();
}

bool cmGlobalVisualStudio15Generator::IsWin81SDKInstalled() const
{
  // Does the VS installer tool know about one?
  if (vsSetupAPIHelper.IsWin81SDKInstalled()) {
    return true;
  }

  // Does the registry know about one (e.g. from VS 2015)?
  std::string win81Root;
  if (cmSystemTools::ReadRegistryValue(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
        "Windows Kits\\Installed Roots;KitsRoot81",
        win81Root, cmSystemTools::KeyWOW64_32) ||
      cmSystemTools::ReadRegistryValue(
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\"
        "Windows Kits\\Installed Roots;KitsRoot81",
        win81Root, cmSystemTools::KeyWOW64_32)) {
    return cmSystemTools::FileExists(win81Root + "/um/windows.h", true);
  }
  return false;
}

std::string cmGlobalVisualStudio15Generator::FindMSBuildCommand()
{
  std::string msbuild;

  // Ask Visual Studio Installer tool.
  std::string vs;
  if (vsSetupAPIHelper.GetVSInstanceInfo(vs)) {
    msbuild = vs + "/MSBuild/15.0/Bin/MSBuild.exe";
    if (cmSystemTools::FileExists(msbuild)) {
      return msbuild;
    }
  }

  msbuild = "MSBuild.exe";
  return msbuild;
}

std::string cmGlobalVisualStudio15Generator::FindDevEnvCommand()
{
  std::string devenv;

  // Ask Visual Studio Installer tool.
  std::string vs;
  if (vsSetupAPIHelper.GetVSInstanceInfo(vs)) {
    devenv = vs + "/Common7/IDE/devenv.com";
    if (cmSystemTools::FileExists(devenv)) {
      return devenv;
    }
  }

  devenv = "devenv.com";
  return devenv;
}
