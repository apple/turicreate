/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio8Generator.h"

#include "cmDocumentationEntry.h"
#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmLocalVisualStudio7Generator.h"
#include "cmMakefile.h"
#include "cmSourceFile.h"
#include "cmVisualStudioWCEPlatformParser.h"
#include "cmake.h"

static const char vs8generatorName[] = "Visual Studio 8 2005";

class cmGlobalVisualStudio8Generator::Factory : public cmGlobalGeneratorFactory
{
public:
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE
  {
    if (strncmp(name.c_str(), vs8generatorName,
                sizeof(vs8generatorName) - 1) != 0) {
      return 0;
    }

    const char* p = name.c_str() + sizeof(vs8generatorName) - 1;
    if (p[0] == '\0') {
      return new cmGlobalVisualStudio8Generator(cm, name, "");
    }

    if (p[0] != ' ') {
      return 0;
    }

    ++p;

    if (!strcmp(p, "Win64")) {
      return new cmGlobalVisualStudio8Generator(cm, name, "x64");
    }

    cmVisualStudioWCEPlatformParser parser(p);
    parser.ParseVersion("8.0");
    if (!parser.Found()) {
      return 0;
    }

    cmGlobalVisualStudio8Generator* ret =
      new cmGlobalVisualStudio8Generator(cm, name, p);
    ret->WindowsCEVersion = parser.GetOSVersion();
    return ret;
  }

  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    entry.Name = std::string(vs8generatorName) + " [arch]";
    entry.Brief = "Deprecated.  Generates Visual Studio 2005 project files.  "
                  "Optional [arch] can be \"Win64\".";
  }

  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(vs8generatorName);
    names.push_back(vs8generatorName + std::string(" Win64"));
    cmVisualStudioWCEPlatformParser parser;
    parser.ParseVersion("8.0");
    const std::vector<std::string>& availablePlatforms =
      parser.GetAvailablePlatforms();
    for (std::vector<std::string>::const_iterator i =
           availablePlatforms.begin();
         i != availablePlatforms.end(); ++i) {
      names.push_back("Visual Studio 8 2005 " + *i);
    }
  }

  bool SupportsToolset() const CM_OVERRIDE { return false; }
  bool SupportsPlatform() const CM_OVERRIDE { return true; }
};

cmGlobalGeneratorFactory* cmGlobalVisualStudio8Generator::NewFactory()
{
  return new Factory;
}

cmGlobalVisualStudio8Generator::cmGlobalVisualStudio8Generator(
  cmake* cm, const std::string& name, const std::string& platformName)
  : cmGlobalVisualStudio71Generator(cm, platformName)
{
  this->ProjectConfigurationSectionName = "ProjectConfigurationPlatforms";
  this->Name = name;
  this->ExtraFlagTable = this->GetExtraFlagTableVS8();
  this->Version = VS8;
  std::string vc8Express;
  this->ExpressEdition = cmSystemTools::ReadRegistryValue(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\8.0\\Setup\\VC;"
    "ProductDir",
    vc8Express, cmSystemTools::KeyWOW64_32);
}

std::string cmGlobalVisualStudio8Generator::FindDevEnvCommand()
{
  // First look for VCExpress.
  std::string vsxcmd;
  std::string vsxkey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\";
  vsxkey += this->GetIDEVersion();
  vsxkey += ";InstallDir";
  if (cmSystemTools::ReadRegistryValue(vsxkey.c_str(), vsxcmd,
                                       cmSystemTools::KeyWOW64_32)) {
    cmSystemTools::ConvertToUnixSlashes(vsxcmd);
    vsxcmd += "/VCExpress.exe";
    return vsxcmd;
  }
  // Now look for devenv.
  return this->cmGlobalVisualStudio71Generator::FindDevEnvCommand();
}

void cmGlobalVisualStudio8Generator::EnableLanguage(
  std::vector<std::string> const& lang, cmMakefile* mf, bool optional)
{
  for (std::vector<std::string>::const_iterator it = lang.begin();
       it != lang.end(); ++it) {
    if (*it == "ASM_MASM") {
      this->MasmEnabled = true;
    }
  }
  this->AddPlatformDefinitions(mf);
  cmGlobalVisualStudio7Generator::EnableLanguage(lang, mf, optional);
}

void cmGlobalVisualStudio8Generator::AddPlatformDefinitions(cmMakefile* mf)
{
  if (this->TargetsWindowsCE()) {
    mf->AddDefinition("CMAKE_VS_WINCE_VERSION",
                      this->WindowsCEVersion.c_str());
  }
}

bool cmGlobalVisualStudio8Generator::SetGeneratorPlatform(std::string const& p,
                                                          cmMakefile* mf)
{
  if (this->DefaultPlatformName == "Win32") {
    this->GeneratorPlatform = p;
    return this->cmGlobalVisualStudio7Generator::SetGeneratorPlatform("", mf);
  } else {
    return this->cmGlobalVisualStudio7Generator::SetGeneratorPlatform(p, mf);
  }
}

// ouput standard header for dsw file
void cmGlobalVisualStudio8Generator::WriteSLNHeader(std::ostream& fout)
{
  fout << "Microsoft Visual Studio Solution File, Format Version 9.00\n";
  fout << "# Visual Studio 2005\n";
}

std::string cmGlobalVisualStudio8Generator::GetGenerateStampList()
{
  return "generate.stamp.list";
}

void cmGlobalVisualStudio8Generator::Configure()
{
  this->cmGlobalVisualStudio7Generator::Configure();
}

bool cmGlobalVisualStudio8Generator::UseFolderProperty()
{
  return IsExpressEdition() ? false : cmGlobalGenerator::UseFolderProperty();
}

std::string cmGlobalVisualStudio8Generator::GetUserMacrosDirectory()
{
  // Some VS8 sp0 versions cannot run macros.
  // See http://support.microsoft.com/kb/928209
  const char* vc8sp1Registry =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\"
    "InstalledProducts\\KB926601;";
  const char* vc8exSP1Registry =
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\8.0\\"
    "InstalledProducts\\KB926748;";
  std::string vc8sp1;
  if (!cmSystemTools::ReadRegistryValue(vc8sp1Registry, vc8sp1) &&
      !cmSystemTools::ReadRegistryValue(vc8exSP1Registry, vc8sp1)) {
    return "";
  }

  std::string base;
  std::string path;

  // base begins with the VisualStudioProjectsLocation reg value...
  if (cmSystemTools::ReadRegistryValue(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\VisualStudio\\8.0;"
        "VisualStudioProjectsLocation",
        base)) {
    cmSystemTools::ConvertToUnixSlashes(base);

    // 8.0 macros folder:
    path = base + "/VSMacros80";
  }

  // path is (correctly) still empty if we did not read the base value from
  // the Registry value
  return path;
}

std::string cmGlobalVisualStudio8Generator::GetUserMacrosRegKeyBase()
{
  return "Software\\Microsoft\\VisualStudio\\8.0\\vsmacros";
}

bool cmGlobalVisualStudio8Generator::AddCheckTarget()
{
  // Add a special target on which all other targets depend that
  // checks the build system and optionally re-runs CMake.
  const char* no_working_directory = 0;
  std::vector<std::string> no_depends;
  std::vector<cmLocalGenerator*> const& generators = this->LocalGenerators;
  cmLocalVisualStudio7Generator* lg =
    static_cast<cmLocalVisualStudio7Generator*>(generators[0]);
  cmMakefile* mf = lg->GetMakefile();

  // Skip the target if no regeneration is to be done.
  if (mf->IsOn("CMAKE_SUPPRESS_REGENERATION")) {
    return false;
  }

  cmCustomCommandLines noCommandLines;
  cmTarget* tgt =
    mf->AddUtilityCommand(CMAKE_CHECK_BUILD_SYSTEM_TARGET, false,
                          no_working_directory, no_depends, noCommandLines);

  cmGeneratorTarget* gt = new cmGeneratorTarget(tgt, lg);
  lg->AddGeneratorTarget(gt);

  // Organize in the "predefined targets" folder:
  //
  if (this->UseFolderProperty()) {
    tgt->SetProperty("FOLDER", this->GetPredefinedTargetsFolder());
  }

  // Create a list of all stamp files for this project.
  std::vector<std::string> stamps;
  std::string stampList = cmake::GetCMakeFilesDirectoryPostSlash();
  stampList += cmGlobalVisualStudio8Generator::GetGenerateStampList();
  {
    std::string stampListFile =
      generators[0]->GetMakefile()->GetCurrentBinaryDirectory();
    stampListFile += "/";
    stampListFile += stampList;
    std::string stampFile;
    cmGeneratedFileStream fout(stampListFile.c_str());
    for (std::vector<cmLocalGenerator*>::const_iterator gi =
           generators.begin();
         gi != generators.end(); ++gi) {
      stampFile = (*gi)->GetMakefile()->GetCurrentBinaryDirectory();
      stampFile += "/";
      stampFile += cmake::GetCMakeFilesDirectoryPostSlash();
      stampFile += "generate.stamp";
      fout << stampFile << "\n";
      stamps.push_back(stampFile);
    }
  }

  // Add a custom rule to re-run CMake if any input files changed.
  {
    // Collect the input files used to generate all targets in this
    // project.
    std::vector<std::string> listFiles;
    for (unsigned int j = 0; j < generators.size(); ++j) {
      cmMakefile* lmf = generators[j]->GetMakefile();
      listFiles.insert(listFiles.end(), lmf->GetListFiles().begin(),
                       lmf->GetListFiles().end());
    }
    // Sort the list of input files and remove duplicates.
    std::sort(listFiles.begin(), listFiles.end(), std::less<std::string>());
    std::vector<std::string>::iterator new_end =
      std::unique(listFiles.begin(), listFiles.end());
    listFiles.erase(new_end, listFiles.end());

    // Create a rule to re-run CMake.
    std::string stampName = cmake::GetCMakeFilesDirectoryPostSlash();
    stampName += "generate.stamp";
    cmCustomCommandLine commandLine;
    commandLine.push_back(cmSystemTools::GetCMakeCommand());
    std::string argH = "-H";
    argH += lg->GetSourceDirectory();
    commandLine.push_back(argH);
    std::string argB = "-B";
    argB += lg->GetBinaryDirectory();
    commandLine.push_back(argB);
    commandLine.push_back("--check-stamp-list");
    commandLine.push_back(stampList.c_str());
    commandLine.push_back("--vs-solution-file");
    std::string const sln = std::string(lg->GetBinaryDirectory()) + "/" +
      lg->GetProjectName() + ".sln";
    commandLine.push_back(sln);
    cmCustomCommandLines commandLines;
    commandLines.push_back(commandLine);

    // Add the rule.  Note that we cannot use the CMakeLists.txt
    // file as the main dependency because it would get
    // overwritten by the CreateVCProjBuildRule.
    // (this could be avoided with per-target source files)
    std::string no_main_dependency = "";
    std::vector<std::string> no_byproducts;
    if (cmSourceFile* file = mf->AddCustomCommandToOutput(
          stamps, no_byproducts, listFiles, no_main_dependency, commandLines,
          "Checking Build System", no_working_directory, true, false)) {
      gt->AddSource(file->GetFullPath());
    } else {
      cmSystemTools::Error("Error adding rule for ", stamps[0].c_str());
    }
  }

  return true;
}

void cmGlobalVisualStudio8Generator::AddExtraIDETargets()
{
  cmGlobalVisualStudio7Generator::AddExtraIDETargets();
  if (this->AddCheckTarget()) {
    for (unsigned int i = 0; i < this->LocalGenerators.size(); ++i) {
      std::vector<cmGeneratorTarget*> tgts =
        this->LocalGenerators[i]->GetGeneratorTargets();
      // All targets depend on the build-system check target.
      for (std::vector<cmGeneratorTarget*>::iterator ti = tgts.begin();
           ti != tgts.end(); ++ti) {
        if ((*ti)->GetName() != CMAKE_CHECK_BUILD_SYSTEM_TARGET) {
          (*ti)->Target->AddUtility(CMAKE_CHECK_BUILD_SYSTEM_TARGET);
        }
      }
    }
  }
}

void cmGlobalVisualStudio8Generator::WriteSolutionConfigurations(
  std::ostream& fout, std::vector<std::string> const& configs)
{
  fout << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    fout << "\t\t" << *i << "|" << this->GetPlatformName() << " = " << *i
         << "|" << this->GetPlatformName() << "\n";
  }
  fout << "\tEndGlobalSection\n";
}

void cmGlobalVisualStudio8Generator::WriteProjectConfigurations(
  std::ostream& fout, const std::string& name, cmGeneratorTarget const& target,
  std::vector<std::string> const& configs,
  const std::set<std::string>& configsPartOfDefaultBuild,
  std::string const& platformMapping)
{
  std::string guid = this->GetGUID(name);
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    std::vector<std::string> mapConfig;
    const char* dstConfig = i->c_str();
    if (target.GetProperty("EXTERNAL_MSPROJECT")) {
      if (const char* m = target.GetProperty("MAP_IMPORTED_CONFIG_" +
                                             cmSystemTools::UpperCase(*i))) {
        cmSystemTools::ExpandListArgument(m, mapConfig);
        if (!mapConfig.empty()) {
          dstConfig = mapConfig[0].c_str();
        }
      }
    }
    fout << "\t\t{" << guid << "}." << *i << "|" << this->GetPlatformName()
         << ".ActiveCfg = " << dstConfig << "|"
         << (!platformMapping.empty() ? platformMapping
                                      : this->GetPlatformName())
         << "\n";
    std::set<std::string>::const_iterator ci =
      configsPartOfDefaultBuild.find(*i);
    if (!(ci == configsPartOfDefaultBuild.end())) {
      fout << "\t\t{" << guid << "}." << *i << "|" << this->GetPlatformName()
           << ".Build.0 = " << dstConfig << "|"
           << (!platformMapping.empty() ? platformMapping
                                        : this->GetPlatformName())
           << "\n";
    }
    if (this->NeedsDeploy(target.GetType())) {
      fout << "\t\t{" << guid << "}." << *i << "|" << this->GetPlatformName()
           << ".Deploy.0 = " << dstConfig << "|"
           << (!platformMapping.empty() ? platformMapping
                                        : this->GetPlatformName())
           << "\n";
    }
  }
}

bool cmGlobalVisualStudio8Generator::NeedsDeploy(
  cmStateEnums::TargetType type) const
{
  bool needsDeploy =
    (type == cmStateEnums::EXECUTABLE || type == cmStateEnums::SHARED_LIBRARY);
  return this->TargetsWindowsCE() && needsDeploy;
}

bool cmGlobalVisualStudio8Generator::ComputeTargetDepends()
{
  // Skip over the cmGlobalVisualStudioGenerator implementation!
  // We do not need the support that VS <= 7.1 needs.
  return this->cmGlobalGenerator::ComputeTargetDepends();
}

void cmGlobalVisualStudio8Generator::WriteProjectDepends(
  std::ostream& fout, const std::string&, const char*,
  cmGeneratorTarget const* gt)
{
  TargetDependSet const& unordered = this->GetTargetDirectDepends(gt);
  OrderedTargetDependSet depends(unordered, std::string());
  for (OrderedTargetDependSet::const_iterator i = depends.begin();
       i != depends.end(); ++i) {
    if ((*i)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    std::string guid = this->GetGUID((*i)->GetName().c_str());
    fout << "\t\t{" << guid << "} = {" << guid << "}\n";
  }
}

bool cmGlobalVisualStudio8Generator::NeedLinkLibraryDependencies(
  cmGeneratorTarget* target)
{
  // Look for utility dependencies that magically link.
  for (std::set<std::string>::const_iterator ui =
         target->GetUtilities().begin();
       ui != target->GetUtilities().end(); ++ui) {
    if (cmGeneratorTarget* depTarget =
          target->GetLocalGenerator()->FindGeneratorTargetToUse(ui->c_str())) {
      if (depTarget->GetType() != cmStateEnums::INTERFACE_LIBRARY &&
          depTarget->GetProperty("EXTERNAL_MSPROJECT")) {
        // This utility dependency names an external .vcproj target.
        // We use LinkLibraryDependencies="true" to link to it without
        // predicting the .lib file location or name.
        return true;
      }
    }
  }
  return false;
}

static cmVS7FlagTable cmVS8ExtraFlagTable[] = {
  { "CallingConvention", "Gd", "cdecl", "0", 0 },
  { "CallingConvention", "Gr", "fastcall", "1", 0 },
  { "CallingConvention", "Gz", "stdcall", "2", 0 },

  { "Detect64BitPortabilityProblems", "Wp64",
    "Detect 64Bit Portability Problems", "true", 0 },
  { "ErrorReporting", "errorReport:prompt", "Report immediately", "1", 0 },
  { "ErrorReporting", "errorReport:queue", "Queue for next login", "2", 0 },
  // Precompiled header and related options.  Note that the
  // UsePrecompiledHeader entries are marked as "Continue" so that the
  // corresponding PrecompiledHeaderThrough entry can be found.
  { "UsePrecompiledHeader", "Yu", "Use Precompiled Header", "2",
    cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue },
  { "PrecompiledHeaderThrough", "Yu", "Precompiled Header Name", "",
    cmVS7FlagTable::UserValueRequired },
  // There is no YX option in the VS8 IDE.

  // Exception handling mode.  If no entries match, it will be FALSE.
  { "ExceptionHandling", "GX", "enable c++ exceptions", "1", 0 },
  { "ExceptionHandling", "EHsc", "enable c++ exceptions", "1", 0 },
  { "ExceptionHandling", "EHa", "enable SEH exceptions", "2", 0 },

  { "EnablePREfast", "analyze", "", "true", 0 },
  { "EnablePREfast", "analyze-", "", "false", 0 },

  // Language options
  { "TreatWChar_tAsBuiltInType", "Zc:wchar_t", "wchar_t is a built-in type",
    "true", 0 },
  { "TreatWChar_tAsBuiltInType", "Zc:wchar_t-",
    "wchar_t is not a built-in type", "false", 0 },

  { 0, 0, 0, 0, 0 }
};
cmIDEFlagTable const* cmGlobalVisualStudio8Generator::GetExtraFlagTableVS8()
{
  return cmVS8ExtraFlagTable;
}
