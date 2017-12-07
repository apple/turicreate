/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio10Generator.h"

#include "cmAlgorithms.h"
#include "cmDocumentationEntry.h"
#include "cmGeneratorTarget.h"
#include "cmLocalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmVS10CLFlagTable.h"
#include "cmVS10CSharpFlagTable.h"
#include "cmVS10CudaFlagTable.h"
#include "cmVS10CudaHostFlagTable.h"
#include "cmVS10LibFlagTable.h"
#include "cmVS10LinkFlagTable.h"
#include "cmVS10MASMFlagTable.h"
#include "cmVS10NASMFlagTable.h"
#include "cmVS10RCFlagTable.h"
#include "cmVersion.h"
#include "cmVisualStudioSlnData.h"
#include "cmVisualStudioSlnParser.h"
#include "cmXMLWriter.h"
#include "cmake.h"

#include "cmsys/FStream.hxx"
#include "cmsys/Glob.hxx"
#include "cmsys/RegularExpression.hxx"

#include <algorithm>

static const char vs10generatorName[] = "Visual Studio 10 2010";

// Map generator name without year to name with year.
static const char* cmVS10GenName(const std::string& name, std::string& genName)
{
  if (strncmp(name.c_str(), vs10generatorName,
              sizeof(vs10generatorName) - 6) != 0) {
    return 0;
  }
  const char* p = name.c_str() + sizeof(vs10generatorName) - 6;
  if (cmHasLiteralPrefix(p, " 2010")) {
    p += 5;
  }
  genName = std::string(vs10generatorName) + p;
  return p;
}

class cmGlobalVisualStudio10Generator::Factory
  : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE
  {
    std::string genName;
    const char* p = cmVS10GenName(name, genName);
    if (!p) {
      return 0;
    }
    if (!*p) {
      return new cmGlobalVisualStudio10Generator(cm, genName, "");
    }
    if (*p++ != ' ') {
      return 0;
    }
    if (strcmp(p, "Win64") == 0) {
      return new cmGlobalVisualStudio10Generator(cm, genName, "x64");
    }
    if (strcmp(p, "IA64") == 0) {
      return new cmGlobalVisualStudio10Generator(cm, genName, "Itanium");
    }
    return 0;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    entry.Name = std::string(vs10generatorName) + " [arch]";
    entry.Brief = "Generates Visual Studio 2010 project files.  "
                  "Optional [arch] can be \"Win64\" or \"IA64\".";
  }

  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(vs10generatorName);
    names.push_back(vs10generatorName + std::string(" IA64"));
    names.push_back(vs10generatorName + std::string(" Win64"));
  }

  bool SupportsToolset() const CM_OVERRIDE { return true; }
  bool SupportsPlatform() const CM_OVERRIDE { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio10Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio10Generator::cmGlobalVisualStudio10Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio8Generator(cm, name, platformName)
{
  std::string vc10Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\10.0\\Setup\\VC;"
    "ProductDir",
    vc10Express, cmSystemTools::KeyWOW64_32);
  this->CudaEnabled = false;
  this->SystemIsWindowsCE = false;
  this->SystemIsWindowsPhone = false;
  this->SystemIsWindowsStore = false;
  this->MSBuildCommandInitialized = false;
  {
    std::string envPlatformToolset;
    if (cmSystemTools::GetEnv("PlatformToolset", envPlatformToolset) &&
        envPlatformToolset == "Windows7.1SDK") {
      // We are running from a Windows7.1SDK command prompt.
      this->DefaultPlatformToolset = "Windows7.1SDK";
    } else {
      this->DefaultPlatformToolset = "v100";
    }
  }
  this->DefaultClFlagTable = cmVS10CLFlagTable;
  this->DefaultCSharpFlagTable = cmVS10CSharpFlagTable;
  this->DefaultLibFlagTable = cmVS10LibFlagTable;
  this->DefaultLinkFlagTable = cmVS10LinkFlagTable;
  this->DefaultCudaFlagTable = cmVS10CudaFlagTable;
  this->DefaultCudaHostFlagTable = cmVS10CudaHostFlagTable;
  this->DefaultMasmFlagTable = cmVS10MASMFlagTable;
  this->DefaultNasmFlagTable = cmVS10NASMFlagTable;
  this->DefaultRcFlagTable = cmVS10RCFlagTable;
  this->Version = VS10;
  this->PlatformToolsetNeedsDebugEnum = false;
}

bool cmGlobalVisualStudio10Generator::MatchesGeneratorName(
  const std::string& name) const
{
  std::string genName;
  if (cmVS10GenName(name, genName)) {
    return genName == this->GetName();
  }
  return false;
}

bool cmGlobalVisualStudio10Generator::SetSystemName(std::string const& s,
                                                    cmMakefile* mf)
{
  this->SystemName = s;
  this->SystemVersion = mf->GetSafeDefinition("CMAKE_SYSTEM_VERSION");
  if (!this->InitializeSystem(mf)) {
    return false;
  }
  return this->cmGlobalVisualStudio8Generator::SetSystemName(s, mf);
}

bool cmGlobalVisualStudio10Generator::SetGeneratorPlatform(
  std::string const& p, cmMakefile* mf)
{
  if (!this->cmGlobalVisualStudio8Generator::SetGeneratorPlatform(p, mf)) {
    return false;
  }
  if (this->GetPlatformName() == "Itanium" ||
      this->GetPlatformName() == "x64") {
    if (this->IsExpressEdition() && !this->Find64BitTools(mf)) {
      return false;
    }
  }
  return true;
}

static void cmCudaToolVersion(std::string& s)
{
  // "CUDA x.y.props" => "x.y"
  s = s.substr(5);
  s = s.substr(0, s.size() - 6);
}

bool cmGlobalVisualStudio10Generator::SetGeneratorToolset(
  std::string const& ts, cmMakefile* mf)
{
  if (this->SystemIsWindowsCE && ts.empty() &&
      this->DefaultPlatformToolset.empty()) {
    std::ostringstream e;
    e << this->GetName() << " Windows CE version '" << this->SystemVersion
      << "' requires CMAKE_GENERATOR_TOOLSET to be set.";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  if (!this->ParseGeneratorToolset(ts, mf)) {
    return false;
  }

  if (!this->FindVCTargetsPath(mf)) {
    return false;
  }

  if (cmHasLiteralPrefix(this->GetPlatformToolsetString(), "v140")) {
    // The GenerateDebugInformation link setting for the v140 toolset
    // in VS 2015 was originally an enum with "No" and "Debug" values,
    // differing from the "false" and "true" values used in older toolsets.
    // A VS 2015 update changed it back.  Parse the "link.xml" file to
    // discover which one we need.
    std::string const link_xml = this->VCTargetsPath + "/1033/link.xml";
    cmsys::ifstream fin(link_xml.c_str());
    std::string line;
    while (fin && cmSystemTools::GetLineFromStream(fin, line)) {
      if (line.find(" Switch=\"DEBUG\" ") != std::string::npos) {
        this->PlatformToolsetNeedsDebugEnum =
          line.find(" Name=\"Debug\" ") != std::string::npos;
        break;
      }
    }
  }

  if (this->GeneratorToolsetCuda.empty()) {
    // Find the highest available version of the CUDA tools.
    std::vector<std::string> cudaTools;
    std::string const bcDir = this->VCTargetsPath + "/BuildCustomizations";
    cmsys::Glob gl;
    gl.SetRelative(bcDir.c_str());
    if (gl.FindFiles(bcDir + "/CUDA *.props")) {
      cudaTools = gl.GetFiles();
    }
    if (!cudaTools.empty()) {
      std::for_each(cudaTools.begin(), cudaTools.end(), cmCudaToolVersion);
      std::sort(cudaTools.begin(), cudaTools.end(),
                cmSystemTools::VersionCompareGreater);
      this->GeneratorToolsetCuda = cudaTools.at(0);
    }
  }

  if (const char* toolset = this->GetPlatformToolset()) {
    mf->AddDefinition("CMAKE_VS_PLATFORM_TOOLSET", toolset);
  }
  if (const char* hostArch = this->GetPlatformToolsetHostArchitecture()) {
    mf->AddDefinition("CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE", hostArch);
  }
  if (const char* cuda = this->GetPlatformToolsetCuda()) {
    mf->AddDefinition("CMAKE_VS_PLATFORM_TOOLSET_CUDA", cuda);
  }
  return true;
}

bool cmGlobalVisualStudio10Generator::ParseGeneratorToolset(
  std::string const& ts, cmMakefile* mf)
{
  std::vector<std::string> const fields = cmSystemTools::tokenize(ts, ",");
  std::vector<std::string>::const_iterator fi = fields.begin();
  if (fi == fields.end()) {
    return true;
  }

  // The first field may be the VS platform toolset.
  if (fi->find('=') == fi->npos) {
    this->GeneratorToolset = *fi;
    ++fi;
  }

  std::set<std::string> handled;

  // The rest of the fields must be key=value pairs.
  for (; fi != fields.end(); ++fi) {
    std::string::size_type pos = fi->find('=');
    if (pos == fi->npos) {
      std::ostringstream e;
      /* clang-format off */
      e <<
        "Generator\n"
        "  " << this->GetName() << "\n"
        "given toolset specification\n"
        "  " << ts << "\n"
        "that contains a field after the first ',' with no '='."
        ;
      /* clang-format on */
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
    std::string const key = fi->substr(0, pos);
    std::string const value = fi->substr(pos + 1);
    if (!handled.insert(key).second) {
      std::ostringstream e;
      /* clang-format off */
      e <<
        "Generator\n"
        "  " << this->GetName() << "\n"
        "given toolset specification\n"
        "  " << ts << "\n"
        "that contains duplicate field key '" << key << "'."
        ;
      /* clang-format on */
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
    if (!this->ProcessGeneratorToolsetField(key, value)) {
      std::ostringstream e;
      /* clang-format off */
      e <<
        "Generator\n"
        "  " << this->GetName() << "\n"
        "given toolset specification\n"
        "  " << ts << "\n"
        "that contains invalid field '" << *fi << "'."
        ;
      /* clang-format on */
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
  }

  return true;
}

bool cmGlobalVisualStudio10Generator::ProcessGeneratorToolsetField(
  std::string const& key, std::string const& value)
{
  if (key == "cuda") {
    this->GeneratorToolsetCuda = value;
    return true;
  }
  return false;
}

bool cmGlobalVisualStudio10Generator::InitializeSystem(cmMakefile* mf)
{
  if (this->SystemName == "Windows") {
    if (!this->InitializeWindows(mf)) {
      return false;
    }
  } else if (this->SystemName == "WindowsCE") {
    this->SystemIsWindowsCE = true;
    if (!this->InitializeWindowsCE(mf)) {
      return false;
    }
  } else if (this->SystemName == "WindowsPhone") {
    this->SystemIsWindowsPhone = true;
    if (!this->InitializeWindowsPhone(mf)) {
      return false;
    }
  } else if (this->SystemName == "WindowsStore") {
    this->SystemIsWindowsStore = true;
    if (!this->InitializeWindowsStore(mf)) {
      return false;
    }
  } else if (this->SystemName == "Android") {
    if (this->DefaultPlatformName != "Win32") {
      std::ostringstream e;
      e << "CMAKE_SYSTEM_NAME is 'Android' but CMAKE_GENERATOR "
        << "specifies a platform too: '" << this->GetName() << "'";
      mf->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
    std::string v = this->GetInstalledNsightTegraVersion();
    if (v.empty()) {
      mf->IssueMessage(cmake::FATAL_ERROR,
                       "CMAKE_SYSTEM_NAME is 'Android' but "
                       "'NVIDIA Nsight Tegra Visual Studio Edition' "
                       "is not installed.");
      return false;
    }
    this->DefaultPlatformName = "Tegra-Android";
    this->DefaultPlatformToolset = "Default";
    this->NsightTegraVersion = v;
    mf->AddDefinition("CMAKE_VS_NsightTegra_VERSION", v.c_str());
  }

  return true;
}

bool cmGlobalVisualStudio10Generator::InitializeWindows(cmMakefile*)
{
  return true;
}

bool cmGlobalVisualStudio10Generator::InitializeWindowsCE(cmMakefile* mf)
{
  if (this->DefaultPlatformName != "Win32") {
    std::ostringstream e;
    e << "CMAKE_SYSTEM_NAME is 'WindowsCE' but CMAKE_GENERATOR "
      << "specifies a platform too: '" << this->GetName() << "'";
    mf->IssueMessage(cmake::FATAL_ERROR, e.str());
    return false;
  }

  this->DefaultPlatformToolset = this->SelectWindowsCEToolset();

  return true;
}

bool cmGlobalVisualStudio10Generator::InitializeWindowsPhone(cmMakefile* mf)
{
  std::ostringstream e;
  e << this->GetName() << " does not support Windows Phone.";
  mf->IssueMessage(cmake::FATAL_ERROR, e.str());
  return false;
}

bool cmGlobalVisualStudio10Generator::InitializeWindowsStore(cmMakefile* mf)
{
  std::ostringstream e;
  e << this->GetName() << " does not support Windows Store.";
  mf->IssueMessage(cmake::FATAL_ERROR, e.str());
  return false;
}

bool cmGlobalVisualStudio10Generator::SelectWindowsPhoneToolset(
  std::string& toolset) const
{
  toolset = "";
  return false;
}

bool cmGlobalVisualStudio10Generator::SelectWindowsStoreToolset(
  std::string& toolset) const
{
  toolset = "";
  return false;
}

std::string cmGlobalVisualStudio10Generator::SelectWindowsCEToolset() const
{
  if (this->SystemVersion == "8.0") {
    return "CE800";
  }
  return "";
}

void cmGlobalVisualStudio10Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 11.00\n";
  if (this->ExpressEdition) {
    fout << "# Visual C++ Express 2010\n";
  } else {
    fout << "# Visual Studio 2010\n";
  }
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator* cmGlobalVisualStudio10Generator::CreateLocalGenerator(
  cmMakefile* mf)
{
  return new cmLocalVisualStudio10Generator(this, mf);
}

void cmGlobalVisualStudio10Generator::Generate()
{
  this->LongestSource = LongestSourcePath();
  this->cmGlobalVisualStudio8Generator::Generate();
  if (this->LongestSource.Length > 0) {
    cmLocalGenerator* lg = this->LongestSource.Target->GetLocalGenerator();
    std::ostringstream e;
    /* clang-format off */
    e <<
      "The binary and/or source directory paths may be too long to generate "
      "Visual Studio 10 files for this project.  "
      "Consider choosing shorter directory names to build this project with "
      "Visual Studio 10.  "
      "A more detailed explanation follows."
      "\n"
      "There is a bug in the VS 10 IDE that renders property dialog fields "
      "blank for files referenced by full path in the project file.  "
      "However, CMake must reference at least one file by full path:\n"
      "  " << this->LongestSource.SourceFile->GetFullPath() << "\n"
      "This is because some Visual Studio tools would append the relative "
      "path to the end of the referencing directory path, as in:\n"
      "  " << lg->GetCurrentBinaryDirectory() << "/"
      << this->LongestSource.SourceRel << "\n"
      "and then incorrectly complain that the file does not exist because "
      "the path length is too long for some internal buffer or API.  "
      "To avoid this problem CMake must use a full path for this file "
      "which then triggers the VS 10 property dialog bug.";
    /* clang-format on */
    lg->IssueMessage(cmake::WARNING, e.str().c_str());
  }
}

void cmGlobalVisualStudio10Generator::EnableLanguage(
  std::vector<std::string> const& lang, cmMakefile* mf, bool optional)
{
  for (std::vector<std::string>::const_iterator it = lang.begin();
       it != lang.end(); ++it) {
    if (*it == "ASM_NASM") {
      this->NasmEnabled = true;
    }
    if (*it == "CUDA") {
      this->CudaEnabled = true;
    }
  }
  this->AddPlatformDefinitions(mf);
  cmGlobalVisualStudio8Generator::EnableLanguage(lang, mf, optional);
}

const char* cmGlobalVisualStudio10Generator::GetPlatformToolset() const
{
  std::string const& toolset = this->GetPlatformToolsetString();
  if (toolset.empty()) {
    return CM_NULLPTR;
  }
  return toolset.c_str();
}

std::string const& cmGlobalVisualStudio10Generator::GetPlatformToolsetString()
  const
{
  if (!this->GeneratorToolset.empty()) {
    return this->GeneratorToolset;
  }
  if (!this->DefaultPlatformToolset.empty()) {
    return this->DefaultPlatformToolset;
  }
  static std::string const empty;
  return empty;
}

const char*
cmGlobalVisualStudio10Generator::GetPlatformToolsetHostArchitecture() const
{
  if (!this->GeneratorToolsetHostArchitecture.empty()) {
    return this->GeneratorToolsetHostArchitecture.c_str();
  }
  return CM_NULLPTR;
}

const char* cmGlobalVisualStudio10Generator::GetPlatformToolsetCuda() const
{
  if (!this->GeneratorToolsetCuda.empty()) {
    return this->GeneratorToolsetCuda.c_str();
  }
  return CM_NULLPTR;
}

std::string const&
cmGlobalVisualStudio10Generator::GetPlatformToolsetCudaString() const
{
  return this->GeneratorToolsetCuda;
}

bool cmGlobalVisualStudio10Generator::FindMakeProgram(cmMakefile* mf)
{
  if (!this->cmGlobalVisualStudio8Generator::FindMakeProgram(mf)) {
    return false;
  }
  mf->AddDefinition("CMAKE_VS_MSBUILD_COMMAND",
                    this->GetMSBuildCommand().c_str());
  return true;
}

std::string const& cmGlobalVisualStudio10Generator::GetMSBuildCommand()
{
  if (!this->MSBuildCommandInitialized) {
    this->MSBuildCommandInitialized = true;
    this->MSBuildCommand = this->FindMSBuildCommand();
  }
  return this->MSBuildCommand;
}

std::string cmGlobalVisualStudio10Generator::FindMSBuildCommand()
{
  std::string msbuild;
  std::string mskey;

  // Search in standard location.
  mskey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\";
  mskey += this->GetToolsVersion();
  mskey += ";MSBuildToolsPath";
  if (cmSystemTools::ReadRegistryValue(mskey.c_str(), msbuild,
                                       cmSystemTools::KeyWOW64_32)) {
    cmSystemTools::ConvertToUnixSlashes(msbuild);
    msbuild += "/MSBuild.exe";
    if (cmSystemTools::FileExists(msbuild, true)) {
      return msbuild;
    }
  }

  msbuild = "MSBuild.exe";
  return msbuild;
}

std::string cmGlobalVisualStudio10Generator::FindDevEnvCommand()
{
  if (this->ExpressEdition) {
    // Visual Studio Express >= 10 do not have "devenv.com" or
    // "VCExpress.exe" that we can use to build reliably.
    // Tell the caller it needs to use MSBuild instead.
    return "";
  }
  // Skip over the cmGlobalVisualStudio8Generator implementation because
  // we expect a real devenv and do not want to look for VCExpress.
  return this->cmGlobalVisualStudio71Generator::FindDevEnvCommand();
}

bool cmGlobalVisualStudio10Generator::FindVCTargetsPath(cmMakefile* mf)
{
  // Skip this in special cases within our own test suite.
  if (this->GetPlatformName() == "Test Platform" ||
      this->GetPlatformToolsetString() == "Test Toolset") {
    return true;
  }

  std::string wd;
  if (!this->ConfiguredFilesPath.empty()) {
    // In a try-compile we are given the outer CMakeFiles directory.
    wd = this->ConfiguredFilesPath;
  } else {
    wd = this->GetCMakeInstance()->GetHomeOutputDirectory();
    wd += cmake::GetCMakeFilesDirectory();
  }
  wd += "/";
  wd += cmVersion::GetCMakeVersion();

  // We record the result persistently in a file.
  std::string const txt = wd + "/VCTargetsPath.txt";

  // If we have a recorded result, use it.
  {
    cmsys::ifstream fin(txt.c_str());
    if (fin && cmSystemTools::GetLineFromStream(fin, this->VCTargetsPath) &&
        cmSystemTools::FileIsDirectory(this->VCTargetsPath)) {
      cmSystemTools::ConvertToUnixSlashes(this->VCTargetsPath);
      return true;
    }
  }

  // Prepare the work directory.
  if (!cmSystemTools::MakeDirectory(wd)) {
    std::string e = "Failed to make directory:\n  " + wd;
    mf->IssueMessage(cmake::FATAL_ERROR, e.c_str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }

  // Generate a project file for MSBuild to tell us the VCTargetsPath value.
  std::string const vcxproj = "VCTargetsPath.vcxproj";
  {
    std::string const vcxprojAbs = wd + "/" + vcxproj;
    cmsys::ofstream fout(vcxprojAbs.c_str());
    cmXMLWriter xw(fout);

    /* clang-format off */
    xw.StartDocument();
    xw.StartElement("Project");
     xw.Attribute("DefaultTargets", "Build");
     xw.Attribute("ToolsVersion", "4.0");
     xw.Attribute("xmlns",
                  "http://schemas.microsoft.com/developer/msbuild/2003");
     if (this->IsNsightTegra()) {
       xw.StartElement("PropertyGroup");
        xw.Attribute("Label", "NsightTegraProject");
        xw.StartElement("NsightTegraProjectRevisionNumber");
         xw.Content("6");
        xw.EndElement(); // NsightTegraProjectRevisionNumber
       xw.EndElement(); // PropertyGroup
     }
     xw.StartElement("ItemGroup");
      xw.Attribute("Label", "ProjectConfigurations");
      xw.StartElement("ProjectConfiguration");
       xw.Attribute("Include", "Debug|" + this->GetPlatformName());
       xw.StartElement("Configuration");
        xw.Content("Debug");
       xw.EndElement(); // Configuration
       xw.StartElement("Platform");
        xw.Content(this->GetPlatformName());
       xw.EndElement(); // Platform
      xw.EndElement(); // ProjectConfiguration
     xw.EndElement(); // ItemGroup
    xw.StartElement("PropertyGroup");
     xw.Attribute("Label", "Globals");
     xw.StartElement("ProjectGuid");
      xw.Content("{F3FC6D86-508D-3FB1-96D2-995F08B142EC}");
     xw.EndElement(); // ProjectGuid
     xw.StartElement("Keyword");
      xw.Content("Win32Proj");
     xw.EndElement(); // Keyword
     xw.StartElement("Platform");
      xw.Content(this->GetPlatformName());
     xw.EndElement(); // Platform
     if (this->GetSystemName() == "WindowsPhone") {
       xw.StartElement("ApplicationType");
        xw.Content("Windows Phone");
       xw.EndElement(); // ApplicationType
       xw.StartElement("ApplicationTypeRevision");
        xw.Content(this->GetSystemVersion());
       xw.EndElement(); // ApplicationTypeRevision
     } else if (this->GetSystemName() == "WindowsStore") {
       xw.StartElement("ApplicationType");
        xw.Content("Windows Store");
       xw.EndElement(); // ApplicationType
       xw.StartElement("ApplicationTypeRevision");
        xw.Content(this->GetSystemVersion());
       xw.EndElement(); // ApplicationTypeRevision
     }
     if (!this->WindowsTargetPlatformVersion.empty()) {
       xw.StartElement("WindowsTargetPlatformVersion");
        xw.Content(this->WindowsTargetPlatformVersion);
       xw.EndElement(); // WindowsTargetPlatformVersion
     }
     if (this->GetPlatformName() == "ARM") {
       xw.StartElement("WindowsSDKDesktopARMSupport");
        xw.Content("true");
       xw.EndElement(); // WindowsSDKDesktopARMSupport
     }
    xw.EndElement(); // PropertyGroup
    xw.StartElement("Import");
     xw.Attribute("Project",
                  "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
    xw.EndElement(); // Import
    if (!this->GeneratorToolsetHostArchitecture.empty()) {
      xw.StartElement("PropertyGroup");
       xw.StartElement("PreferredToolArchitecture");
        xw.Content(this->GeneratorToolsetHostArchitecture);
       xw.EndElement(); // PreferredToolArchitecture
      xw.EndElement(); // PropertyGroup
    }
    xw.StartElement("PropertyGroup");
     xw.Attribute("Label", "Configuration");
     xw.StartElement("ConfigurationType");
      if (this->IsNsightTegra()) {
        // Tegra-Android platform does not understand "Utility".
        xw.Content("StaticLibrary");
      } else {
        xw.Content("Utility");
      }
     xw.EndElement(); // ConfigurationType
     xw.StartElement("CharacterSet");
      xw.Content("MultiByte");
     xw.EndElement(); // CharacterSet
     if (this->IsNsightTegra()) {
       xw.StartElement("NdkToolchainVersion");
       xw.Content(this->GetPlatformToolsetString());
       xw.EndElement(); // NdkToolchainVersion
     } else {
       xw.StartElement("PlatformToolset");
       xw.Content(this->GetPlatformToolsetString());
       xw.EndElement(); // PlatformToolset
     }
    xw.EndElement(); // PropertyGroup
    xw.StartElement("Import");
     xw.Attribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
    xw.EndElement(); // Import
    xw.StartElement("ItemDefinitionGroup");
     xw.StartElement("PostBuildEvent");
      xw.StartElement("Command");
       xw.Content("echo VCTargetsPath=$(VCTargetsPath)");
      xw.EndElement(); // Command
     xw.EndElement(); // PostBuildEvent
    xw.EndElement(); // ItemDefinitionGroup
    xw.StartElement("Import");
     xw.Attribute("Project",
                  "$(VCTargetsPath)\\Microsoft.Cpp.targets");
    xw.EndElement(); // Import
    xw.EndElement(); // Project
    xw.EndDocument();
    /* clang-format on */
  }

  std::vector<std::string> cmd;
  cmd.push_back(this->GetMSBuildCommand());
  cmd.push_back(vcxproj);
  cmd.push_back("/p:Configuration=Debug");
  cmd.push_back(std::string("/p:VisualStudioVersion=") +
                this->GetIDEVersion());
  std::string out;
  int ret = 0;
  cmsys::RegularExpression regex("\n *VCTargetsPath=([^%\r\n]+)[\r\n]");
  if (!cmSystemTools::RunSingleCommand(cmd, &out, &out, &ret, wd.c_str(),
                                       cmSystemTools::OUTPUT_NONE) ||
      ret != 0 || !regex.find(out)) {
    cmSystemTools::ReplaceString(out, "\n", "\n  ");
    std::ostringstream e;
    /* clang-format off */
    e <<
      "Failed to run MSBuild command:\n"
      "  " << cmd[0] << "\n"
      "to get the value of VCTargetsPath:\n"
      "  " << out << "\n"
      ;
    /* clang-format on */
    if (ret != 0) {
      e << "Exit code: " << ret << "\n";
    }
    mf->IssueMessage(cmake::FATAL_ERROR, e.str().c_str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
  this->VCTargetsPath = regex.match(1);
  cmSystemTools::ConvertToUnixSlashes(this->VCTargetsPath);

  {
    cmsys::ofstream fout(txt.c_str());
    fout << this->VCTargetsPath << "\n";
  }
  return true;
}

void cmGlobalVisualStudio10Generator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& projectDir,
  const std::string& targetName, const std::string& config, bool fast,
  bool verbose, std::vector<std::string> const& makeOptions)
{
  // Select the caller- or user-preferred make program, else MSBuild.
  std::string makeProgramSelected =
    this->SelectMakeProgram(makeProgram, this->GetMSBuildCommand());

  // Check if the caller explicitly requested a devenv tool.
  std::string makeProgramLower = makeProgramSelected;
  cmSystemTools::LowerCase(makeProgramLower);
  bool useDevEnv = (makeProgramLower.find("devenv") != std::string::npos ||
                    makeProgramLower.find("vcexpress") != std::string::npos);

  // MSBuild is preferred (and required for VS Express), but if the .sln has
  // an Intel Fortran .vfproj then we have to use devenv. Parse it to find out.
  cmSlnData slnData;
  {
    std::string slnFile;
    if (!projectDir.empty()) {
      slnFile = projectDir;
      slnFile += "/";
    }
    slnFile += projectName;
    slnFile += ".sln";
    cmVisualStudioSlnParser parser;
    if (parser.ParseFile(slnFile, slnData,
                         cmVisualStudioSlnParser::DataGroupProjects)) {
      std::vector<cmSlnProjectEntry> slnProjects = slnData.GetProjects();
      for (std::vector<cmSlnProjectEntry>::iterator i = slnProjects.begin();
           !useDevEnv && i != slnProjects.end(); ++i) {
        std::string proj = i->GetRelativePath();
        if (proj.size() > 7 && proj.substr(proj.size() - 7) == ".vfproj") {
          useDevEnv = true;
        }
      }
    }
  }
  if (useDevEnv) {
    // Use devenv to build solutions containing Intel Fortran projects.
    cmGlobalVisualStudio7Generator::GenerateBuildCommand(
      makeCommand, makeProgram, projectName, projectDir, targetName, config,
      fast, verbose, makeOptions);
    return;
  }

  makeCommand.push_back(makeProgramSelected);

  std::string realTarget = targetName;
  // msbuild.exe CxxOnly.sln /t:Build /p:Configuration=Debug /target:ALL_BUILD
  if (realTarget.empty()) {
    realTarget = "ALL_BUILD";
  }
  if (realTarget == "clean") {
    makeCommand.push_back(std::string(projectName) + ".sln");
    makeCommand.push_back("/t:Clean");
  } else {
    std::string targetProject(realTarget);
    targetProject += ".vcxproj";
    if (targetProject.find('/') == std::string::npos) {
      // it might be in a subdir
      if (cmSlnProjectEntry const* proj =
            slnData.GetProjectByName(realTarget)) {
        targetProject = proj->GetRelativePath();
        cmSystemTools::ConvertToUnixSlashes(targetProject);
      }
    }
    makeCommand.push_back(targetProject);
  }
  std::string configArg = "/p:Configuration=";
  if (!config.empty()) {
    configArg += config;
  } else {
    configArg += "Debug";
  }
  makeCommand.push_back(configArg);
  makeCommand.push_back(std::string("/p:VisualStudioVersion=") +
                        this->GetIDEVersion());
  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
}

bool cmGlobalVisualStudio10Generator::Find64BitTools(cmMakefile* mf)
{
  if (this->DefaultPlatformToolset == "v100") {
    // The v100 64-bit toolset does not exist in the express edition.
    this->DefaultPlatformToolset.clear();
  }
  if (this->GetPlatformToolset()) {
    return true;
  }
  // This edition does not come with 64-bit tools.  Look for them.
  //
  // TODO: Detect available tools?  x64\v100 exists but does not work?
  // HKLM\\SOFTWARE\\Microsoft\\MSBuild\\ToolsVersions\\4.0;VCTargetsPath
  // c:/Program Files (x86)/MSBuild/Microsoft.Cpp/v4.0/Platforms/
  //   {Itanium,Win32,x64}/PlatformToolsets/{v100,v90,Windows7.1SDK}
  std::string winSDK_7_1;
  if (cmSystemTools::ReadRegistryValue(
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\"
        "Windows\\v7.1;InstallationFolder",
        winSDK_7_1)) {
    std::ostringstream m;
    m << "Found Windows SDK v7.1: " << winSDK_7_1;
    mf->DisplayStatus(m.str().c_str(), -1);
    this->DefaultPlatformToolset = "Windows7.1SDK";
    return true;
  } else {
    std::ostringstream e;
    /* clang-format off */
    e << "Cannot enable 64-bit tools with Visual Studio 2010 Express.\n"
      << "Install the Microsoft Windows SDK v7.1 to get 64-bit tools:\n"
      << "  http://msdn.microsoft.com/en-us/windows/bb980924.aspx";
    /* clang-format on */
    mf->IssueMessage(cmake::FATAL_ERROR, e.str().c_str());
    cmSystemTools::SetFatalErrorOccured();
    return false;
  }
}

std::string cmGlobalVisualStudio10Generator::GenerateRuleFile(
  std::string const& output) const
{
  // The VS 10 generator needs to create the .rule files on disk.
  // Hide them away under the CMakeFiles directory.
  std::string ruleDir = this->GetCMakeInstance()->GetHomeOutputDirectory();
  ruleDir += cmake::GetCMakeFilesDirectory();
  ruleDir += "/";
  ruleDir += cmSystemTools::ComputeStringMD5(
    cmSystemTools::GetFilenamePath(output).c_str());
  std::string ruleFile = ruleDir + "/";
  ruleFile += cmSystemTools::GetFilenameName(output);
  ruleFile += ".rule";
  return ruleFile;
}

void cmGlobalVisualStudio10Generator::PathTooLong(cmGeneratorTarget* target,
                                                  cmSourceFile const* sf,
                                                  std::string const& sfRel)
{
  size_t len =
    (strlen(target->GetLocalGenerator()->GetCurrentBinaryDirectory()) + 1 +
     sfRel.length());
  if (len > this->LongestSource.Length) {
    this->LongestSource.Length = len;
    this->LongestSource.Target = target;
    this->LongestSource.SourceFile = sf;
    this->LongestSource.SourceRel = sfRel;
  }
}

bool cmGlobalVisualStudio10Generator::IsNsightTegra() const
{
  return !this->NsightTegraVersion.empty();
}

std::string cmGlobalVisualStudio10Generator::GetNsightTegraVersion() const
{
  return this->NsightTegraVersion;
}

std::string cmGlobalVisualStudio10Generator::GetInstalledNsightTegraVersion()
{
  std::string version;
  cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\NVIDIA Corporation\\Nsight Tegra;"
    "Version",
    version, cmSystemTools::KeyWOW64_32);
  return version;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetClFlagTable() const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetClFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultClFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetCSharpFlagTable()
  const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetCSharpFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultCSharpFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetRcFlagTable() const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetRcFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultRcFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetLibFlagTable() const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetLibFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultLibFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetLinkFlagTable() const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetLinkFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultLinkFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetCudaFlagTable() const
{
  return this->DefaultCudaFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetCudaHostFlagTable()
  const
{
  return this->DefaultCudaHostFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetMasmFlagTable() const
{
  cmIDEFlagTable const* table = this->ToolsetOptions.GetMasmFlagTable(
    this->GetPlatformName(), this->GetPlatformToolsetString());

  return (table != CM_NULLPTR) ? table : this->DefaultMasmFlagTable;
}

cmIDEFlagTable const* cmGlobalVisualStudio10Generator::GetNasmFlagTable() const
{
  return this->DefaultNasmFlagTable;
}
