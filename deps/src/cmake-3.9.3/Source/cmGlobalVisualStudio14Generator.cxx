/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio14Generator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmVS140CLFlagTable.h"
#include "cmVS140CSharpFlagTable.h"
#include "cmVS140LinkFlagTable.h"
#include "cmVS14LibFlagTable.h"
#include "cmVS14MASMFlagTable.h"
#include "cmVS14RCFlagTable.h"

static const char vs14generatorName[] = "Visual Studio 14 2015";

// Map generator name without year to name with year.
static const char* cmVS14GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs14generatorName,
              sizeof(vs14generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs14generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2015")) {
    p += 5;
  }
  genName = std::string(vs14generatorName) + p;
  return p;
}

class cmGlobalVisualStudio14Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE
  {
    std::string genName;
    const char* p = cmVS14GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudio14Generator(cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudio14Generator(cm, genName, "x64");
    }
    if (strcmp(p, "ARM") == 0) {
      return new cmGlobalVisualStudio14Generator(cm, genName, "ARM");
    }
    return 0;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    entry.Name = std::string(vs14generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2015 project files.  "
                  "Optional [arch] can be \"Win64\" or \"ARM\".";
  }

  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(vs14generatorName);
    names.push_back(vs14generatorName + std::string(" ARM"));
    names.push_back(vs14generatorName + std::string(" Win64"));
  }

  bool SupportsToolset() const CM_OVERRIDE { return true; }
  bool SupportsPlatform() const CM_OVERRIDE { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio14Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio14Generator::cmGlobalVisualStudio14Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio12Generator(cm, name, platformName)
{
  std::string vc14Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\14.0\\Setup\\VC;"
    "ProductDir",
    vc14Express, cmSystemTools::KeyWOW64_32);
  this->DefaultPlatformToolset = "v140";
  this->DefaultClFlagTable = cmVS140CLFlagTable;
  this->DefaultCSharpFlagTable = cmVS140CSharpFlagTable;
  this->DefaultLibFlagTable = cmVS14LibFlagTable;
  this->DefaultLinkFlagTable = cmVS140LinkFlagTable;
  this->DefaultMasmFlagTable = cmVS14MASMFlagTable;
  this->DefaultRcFlagTable = cmVS14RCFlagTable;
  this->Version = VS14;
}

bool cmGlobalVisualStudio14Generator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  if (cmVS14GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

bool cmGlobalVisualStudio14Generator::InitializeWindows(cmMakefile* mf)
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    return this->SelectWindows10SDK(mf, false);
  }
  return true;
}

bool cmGlobalVisualStudio14Generator::InitializeWindowsStore(cmMakefile* mf)
{
  std::ostringstream e;
  if (!this->SelectWindowsStoreToolset(this->DefaultPlatformToolset)) {
    if (this->DefaultPlatformToolset.empty()) {
      e << this->GetName() << " supports Windows Store '8.0', '8.1' and "
                              "'10.0', but not '"
        << this->SystemVersion << "'.  Check CMAKE_SYSTEM_VERSION.";
    } else {
      e << "A Windows Store component with CMake requires both the Windows "
        << "Desktop SDK as well as the Windows Store '" << this->SystemVersion
        << "' SDK. Please make sure that you have both installed";
    }
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    return this->SelectWindows10SDK(mf, true);
  }
  return true;
}

bool cmGlobalVisualStudio14Generator::SelectWindows10SDK(cmMakefile* mf,
                                                         bool required)
{
  // Find the default version of the Windows 10 SDK.
  this->WindowsTargetPlatformVersion = this->GetWindows10SDKVersion();
  if (required && this->WindowsTargetPlatformVersion.empty()) {
    std::ostringstream e;
    e << "Could not find an appropriate version of the Windows 10 SDK"
      << " installed on this machine";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }
  mf->AddDefinition("CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION",
                    this->WindowsTargetPlatformVersion.c_str());
  return true;
}

bool cmGlobalVisualStudio14Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (cmHasLiteralPrefix(this->SystemVersion, "10.0")) {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v140";
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio12Generator::SelectWindowsStoreToolset(
    toolset);
}

void cmGlobalVisualStudio14Generator::WriteSLNHeader(std::ostream& fout)
{
  // Visual Studio 14 writes .sln format 12.00
  fout << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
  if (this->ExpressEdition) {
    fout << "# Visual Studio Express 14 for Windows Desktop\n";
  } else {
    fout << "# Visual Studio 14\n";
  }
}

bool cmGlobalVisualStudio14Generator::IsWindowsDesktopToolsetInstalled() const
{
  const char desktop10Key[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
                              "VisualStudio\\14.0\\VC\\Runtimes";

  std::vector<std::string> vc14;
  return cmSystemTools::GetRegistrySubKeys(desktop10Key, vc14,
                                           cmSystemTools::KeyWOW64_32);
}

bool cmGlobalVisualStudio14Generator::IsWindowsStoreToolsetInstalled() const
{
  const char universal10Key[] =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
    "VisualStudio\\14.0\\Setup\\Build Tools for Windows 10;SrcPath";

  std::string win10SDK;
  return cmSystemTools::ReadRegistryValue(universal10Key, win10SDK,
                                          cmSystemTools::KeyWOW64_32);
}

#if defined(_WIN32) && !defined(__CYGWIN__)
struct NoWindowsH
{
  bool operator()(std::string const& p)
  {
    return !cmSystemTools::FileExists(p + "/um/windows.h", true);
  }
};
#endif

std::string cmGlobalVisualStudio14Generator::GetWindows10SDKVersion()
{
#if defined(_WIN32) && !defined(__CYGWIN__)
  std::vector<std::string> win10Roots;

  {
    std::string win10Root;
    if (cmSystemTools::GetEnv("CMAKE_WINDOWS_KITS_10_DIR", win10Root)) {
      cmSystemTools::ConvertToUnixSlashes(win10Root);
      win10Roots.push_back(win10Root);
    }
  }

  {
    // This logic is taken from the vcvarsqueryregistry.bat file from VS2015
    // Try HKLM and then HKCU.
    std::string win10Root;
    if (cmSystemTools::ReadRegistryValue(
          "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
          "Windows Kits\\Installed Roots;KitsRoot10",
          win10Root, cmSystemTools::KeyWOW64_32) ||
        cmSystemTools::ReadRegistryValue(
          "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\"
          "Windows Kits\\Installed Roots;KitsRoot10",
          win10Root, cmSystemTools::KeyWOW64_32)) {
      cmSystemTools::ConvertToUnixSlashes(win10Root);
      win10Roots.push_back(win10Root);
    }
  }

  if (win10Roots.empty()) {
    return std::string();
  }

  std::vector<std::string> sdks;
  // Grab the paths of the different SDKs that are installed
  for (std::vector<std::string>::iterator i = win10Roots.begin();
       i != win10Roots.end(); ++i) {
    std::string path = *i + "/Include/*";
    cmSystemTools::GlobDirs(path, sdks);
  }

  // Skip SDKs that do not contain <um/windows.h> because that indicates that
  // only the UCRT MSIs were installed for them.
  cmEraseIf(sdks, NoWindowsH());

  if (!sdks.empty()) {
    // Only use the filename, which will be the SDK version.
    for (std::vector<std::string>::iterator i = sdks.begin(); i != sdks.end();
         ++i) {
      *i = cmSystemTools::GetFilenameName(*i);
    }

    // Sort the results to make sure we select the most recent one.
    std::sort(sdks.begin(), sdks.end(), cmSystemTools::VersionCompareGreater);

    // Look for a SDK exactly matching the requested target version.
    for (std::vector<std::string>::iterator i = sdks.begin(); i != sdks.end();
         ++i) {
      if (cmSystemTools::VersionCompareEqual(*i, this->SystemVersion)) {
        return *i;
      }
    }

    // Use the latest Windows 10 SDK since the exact version is not available.
    return sdks.at(0);
  }
#endif
  // Return an empty string
  return std::string();
}
