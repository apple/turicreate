/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio12Generator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmVS12CLFlagTable.h"
#include "cmVS12CSharpFlagTable.h"
#include "cmVS12LibFlagTable.h"
#include "cmVS12LinkFlagTable.h"
#include "cmVS12MASMFlagTable.h"
#include "cmVS12RCFlagTable.h"

static const char vs12generatorName[] = "Visual Studio 12 2013";

// Map generator name without year to name with year.
static const char* cmVS12GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs12generatorName,
              sizeof(vs12generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs12generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2013")) {
    p += 5;
  }
  genName = std::string(vs12generatorName) + p;
  return p;
}

class cmGlobalVisualStudio12Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE
  {
    std::string genName;
    const char* p = cmVS12GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudio12Generator(cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudio12Generator(cm, genName, "x64");
    }
    if (strcmp(p, "ARM") == 0) {
      return new cmGlobalVisualStudio12Generator(cm, genName, "ARM");
    }
    return 0;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    entry.Name = std::string(vs12generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2013 project files.  "
                  "Optional [arch] can be \"Win64\" or \"ARM\".";
  }

  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(vs12generatorName);
    names.push_back(vs12generatorName + std::string(" ARM"));
    names.push_back(vs12generatorName + std::string(" Win64"));
  }

  bool SupportsToolset() const CM_OVERRIDE { return true; }
  bool SupportsPlatform() const CM_OVERRIDE { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio12Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio12Generator::cmGlobalVisualStudio12Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio11Generator(cm, name, platformName)
{
  std::string vc12Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\12.0\\Setup\\VC;"
    "ProductDir",
    vc12Express, cmSystemTools::KeyWOW64_32);
  this->DefaultPlatformToolset = "v120";
  this->DefaultClFlagTable = cmVS12CLFlagTable;
  this->DefaultCSharpFlagTable = cmVS12CSharpFlagTable;
  this->DefaultLibFlagTable = cmVS12LibFlagTable;
  this->DefaultLinkFlagTable = cmVS12LinkFlagTable;
  this->DefaultMasmFlagTable = cmVS12MASMFlagTable;
  this->DefaultRcFlagTable = cmVS12RCFlagTable;
  this->Version = VS12;
}

bool cmGlobalVisualStudio12Generator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  if (cmVS12GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

bool cmGlobalVisualStudio12Generator::ProcessGeneratorToolsetField(
  std::string const& key, std::string const& value)
{
  if (key == "host" && value == "x64") {
    this->GeneratorToolsetHostArchitecture = "x64";
    return true;
  }
  return this->cmGlobalVisualStudio11Generator::ProcessGeneratorToolsetField(
    key, value);
}

bool cmGlobalVisualStudio12Generator::InitializeWindowsPhone(cmMakefile* mf)
{
  if (!this->SelectWindowsPhoneToolset(this->DefaultPlatformToolset)) {
    std::ostringstream e;
    if (this->DefaultPlatformToolset.empty()) {
      e << this->GetName() << " supports Windows Phone '8.0' and '8.1', but "
                              "not '"
        << this->SystemVersion << "'.  Check CMAKE_SYSTEM_VERSION.";
    } else {
      e << "A Windows Phone component with CMake requires both the Windows "
        << "Desktop SDK as well as the Windows Phone '" << this->SystemVersion
        << "' SDK. Please make sure that you have both installed";
    }
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }
  return true;
}

bool cmGlobalVisualStudio12Generator::InitializeWindowsStore(cmMakefile* mf)
{
  if (!this->SelectWindowsStoreToolset(this->DefaultPlatformToolset)) {
    std::ostringstream e;
    if (this->DefaultPlatformToolset.empty()) {
      e << this->GetName() << " supports Windows Store '8.0' and '8.1', but "
                              "not '"
        << this->SystemVersion << "'.  Check CMAKE_SYSTEM_VERSION.";
    } else {
      e << "A Windows Store component with CMake requires both the Windows "
        << "Desktop SDK as well as the Windows Store '" << this->SystemVersion
        << "' SDK. Please make sure that you have both installed";
    }
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }
  return true;
}

bool cmGlobalVisualStudio12Generator::SelectWindowsPhoneToolset(
  std::string& toolset) const
{
  if (this->SystemVersion == "8.1") {
    if (this->IsWindowsPhoneToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v120_wp81";
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio11Generator::SelectWindowsPhoneToolset(
    toolset);
}

bool cmGlobalVisualStudio12Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  if (this->SystemVersion == "8.1") {
    if (this->IsWindowsStoreToolsetInstalled() &&
        this->IsWindowsDesktopToolsetInstalled()) {
      toolset = "v120";
      return true;
    } else {
      return false;
    }
  }
  return this->cmGlobalVisualStudio11Generator::SelectWindowsStoreToolset(
    toolset);
}

void cmGlobalVisualStudio12Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 12.00\n";
  if (this->ExpressEdition) {
    fout << "# Visual Studio Express 2013 for Windows Desktop\n";
  } else {
    fout << "# Visual Studio 2013\n";
  }
}

bool cmGlobalVisualStudio12Generator::IsWindowsDesktopToolsetInstalled() const
{
  const char desktop81Key[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
                              "VisualStudio\\12.0\\VC\\LibraryDesktop";

  std::vector<std::string> subkeys;
  return cmSystemTools::GetRegistrySubKeys(desktop81Key, subkeys,
                                           cmSystemTools::KeyWOW64_32);
}

bool cmGlobalVisualStudio12Generator::IsWindowsPhoneToolsetInstalled() const
{
  const char wp81Key[] =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
    "Microsoft SDKs\\WindowsPhone\\v8.1\\Install Path;Install Path";

  std::string path;
  cmSystemTools::ReadRegistryValue(wp81Key, path, cmSystemTools::KeyWOW64_32);
  return !path.empty();
}

bool cmGlobalVisualStudio12Generator::IsWindowsStoreToolsetInstalled() const
{
  const char win81Key[] = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
                          "VisualStudio\\12.0\\VC\\Libraries\\Core\\Arm";

  std::vector<std::string> subkeys;
  return cmSystemTools::GetRegistrySubKeys(win81Key, subkeys,
                                           cmSystemTools::KeyWOW64_32);
}
