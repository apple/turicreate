/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalVisualStudio7Generator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmLocalVisualStudio7Generator.h"
#include "cmMakefile.h"
#include "cmState.h"
#include "cmUuid.h"
#include "cmake.h"
#include "cmsys/Encoding.hxx"

#include <assert.h>
#include <vector>
#include <windows.h>

static cmVS7FlagTable cmVS7ExtraFlagTable[] = {
  // Precompiled header and related options.  Note that the
  // UsePrecompiledHeader entries are marked as "Continue" so that the
  // corresponding PrecompiledHeaderThrough entry can be found.
  { "UsePrecompiledHeader", "YX", "Automatically Generate", "2",
    cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue },
  { "PrecompiledHeaderThrough", "YX", "Precompiled Header Name", "",
    cmVS7FlagTable::UserValueRequired },
  { "UsePrecompiledHeader", "Yu", "Use Precompiled Header", "3",
    cmVS7FlagTable::UserValueIgnored | cmVS7FlagTable::Continue },
  { "PrecompiledHeaderThrough", "Yu", "Precompiled Header Name", "",
    cmVS7FlagTable::UserValueRequired },
  { "WholeProgramOptimization", "LTCG", "WholeProgramOptimization", "true",
    0 },

  // Exception handling mode.  If no entries match, it will be FALSE.
  { "ExceptionHandling", "GX", "enable c++ exceptions", "true", 0 },
  { "ExceptionHandling", "EHsc", "enable c++ exceptions", "true", 0 },
  // The EHa option does not have an IDE setting.  Let it go to false,
  // and have EHa passed on the command line by leaving out the table
  // entry.

  { 0, 0, 0, 0, 0 }
};

cmGlobalVisualStudio7Generator::cmGlobalVisualStudio7Generator(
  cmake* cm, const std::string& platformName)
  : cmGlobalVisualStudioGenerator(cm)
{
  this->IntelProjectVersion = 0;
  this->DevEnvCommandInitialized = false;
  this->MasmEnabled = false;
  this->NasmEnabled = false;

  if (platformName.empty()) {
    this->DefaultPlatformName = "Win32";
  } else {
    this->DefaultPlatformName = platformName;
  }
  this->ExtraFlagTable = cmVS7ExtraFlagTable;
}

cmGlobalVisualStudio7Generator::~cmGlobalVisualStudio7Generator()
{
  free(this->IntelProjectVersion);
}

// Package GUID of Intel Visual Fortran plugin to VS IDE
#define CM_INTEL_PLUGIN_GUID "{B68A201D-CB9B-47AF-A52F-7EEC72E217E4}"

const char* cmGlobalVisualStudio7Generator::GetIntelProjectVersion()
{
  if (!this->IntelProjectVersion) {
    // Compute the version of the Intel plugin to the VS IDE.
    // If the key does not exist then use a default guess.
    std::string intelVersion;
    std::string vskey = this->GetRegistryBase();
    vskey += "\\Packages\\" CM_INTEL_PLUGIN_GUID ";ProductVersion";
    cmSystemTools::ReadRegistryValue(vskey.c_str(), intelVersion,
                                     cmSystemTools::KeyWOW64_32);
    unsigned int intelVersionNumber = ~0u;
    sscanf(intelVersion.c_str(), "%u", &intelVersionNumber);
    if (intelVersionNumber >= 11) {
      // Default to latest known project file version.
      intelVersion = "11.0";
    } else if (intelVersionNumber == 10) {
      // Version 10.x actually uses 9.10 in project files!
      intelVersion = "9.10";
    } else {
      // Version <= 9: use ProductVersion from registry.
    }
    this->IntelProjectVersion = strdup(intelVersion.c_str());
  }
  return this->IntelProjectVersion;
}

void cmGlobalVisualStudio7Generator::EnableLanguage(
  std::vector<std::string> const& lang, cmMakefile* mf, bool optional)
{
  mf->AddDefinition("CMAKE_GENERATOR_RC", "rc");
  mf->AddDefinition("CMAKE_GENERATOR_NO_COMPILER_ENV", "1");
  if (!mf->GetDefinition("CMAKE_CONFIGURATION_TYPES")) {
    mf->AddCacheDefinition(
      "CMAKE_CONFIGURATION_TYPES", "Debug;Release;MinSizeRel;RelWithDebInfo",
      "Semicolon separated list of supported configuration types, "
      "only supports Debug, Release, MinSizeRel, and RelWithDebInfo, "
      "anything else will be ignored.",
      cmStateEnums::STRING);
  }

  // Create list of configurations requested by user's cache, if any.
  this->cmGlobalGenerator::EnableLanguage(lang, mf, optional);

  // if this environment variable is set, then copy it to
  // a static cache entry.  It will be used by
  // cmLocalGenerator::ConstructScript, to add an extra PATH
  // to all custom commands.   This is because the VS IDE
  // does not use the environment it is run in, and this allows
  // for running commands and using dll's that the IDE environment
  // does not know about.
  std::string extraPath;
  if (cmSystemTools::GetEnv("CMAKE_MSVCIDE_RUN_PATH", extraPath)) {
    mf->AddCacheDefinition("CMAKE_MSVCIDE_RUN_PATH", extraPath.c_str(),
                           "Saved environment variable CMAKE_MSVCIDE_RUN_PATH",
                           cmStateEnums::STATIC);
  }
}

bool cmGlobalVisualStudio7Generator::FindMakeProgram(cmMakefile* mf)
{
  if (!this->cmGlobalVisualStudioGenerator::FindMakeProgram(mf)) {
    return false;
  }
  mf->AddDefinition("CMAKE_VS_DEVENV_COMMAND",
                    this->GetDevEnvCommand().c_str());
  return true;
}

std::string const& cmGlobalVisualStudio7Generator::GetDevEnvCommand()
{
  if (!this->DevEnvCommandInitialized) {
    this->DevEnvCommandInitialized = true;
    this->DevEnvCommand = this->FindDevEnvCommand();
  }
  return this->DevEnvCommand;
}

std::string cmGlobalVisualStudio7Generator::FindDevEnvCommand()
{
  std::string vscmd;
  std::string vskey;

  // Search in standard location.
  vskey = this->GetRegistryBase() + ";InstallDir";
  if (cmSystemTools::ReadRegistryValue(vskey.c_str(), vscmd,
                                       cmSystemTools::KeyWOW64_32)) {
    cmSystemTools::ConvertToUnixSlashes(vscmd);
    vscmd += "/devenv.com";
    if (cmSystemTools::FileExists(vscmd, true)) {
      return vscmd;
    }
  }

  // Search where VS15Preview places it.
  vskey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7;";
  vskey += this->GetIDEVersion();
  if (cmSystemTools::ReadRegistryValue(vskey.c_str(), vscmd,
                                       cmSystemTools::KeyWOW64_32)) {
    cmSystemTools::ConvertToUnixSlashes(vscmd);
    vscmd += "/Common7/IDE/devenv.com";
    if (cmSystemTools::FileExists(vscmd, true)) {
      return vscmd;
    }
  }

  vscmd = "devenv.com";
  return vscmd;
}

const char* cmGlobalVisualStudio7Generator::ExternalProjectType(
  const char* location)
{
  std::string extension = cmSystemTools::GetFilenameLastExtension(location);
  if (extension == ".vbproj") {
    return "F184B08F-C81C-45F6-A57F-5ABD9991F28F";
  } else if (extension == ".csproj") {
    return "FAE04EC0-301F-11D3-BF4B-00C04F79EFBC";
  } else if (extension == ".fsproj") {
    return "F2A71F9B-5D33-465A-A702-920D77279786";
  } else if (extension == ".vdproj") {
    return "54435603-DBB4-11D2-8724-00A0C9A8B90C";
  } else if (extension == ".dbproj") {
    return "C8D11400-126E-41CD-887F-60BD40844F9E";
  } else if (extension == ".wixproj") {
    return "930C7802-8A8C-48F9-8165-68863BCCD9DD";
  } else if (extension == ".pyproj") {
    return "888888A0-9F3D-457C-B088-3A5042F75D52";
  }
  return "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942";
}
void cmGlobalVisualStudio7Generator::GenerateBuildCommand(
  std::vector<std::string>& makeCommand, const std::string& makeProgram,
  const std::string& projectName, const std::string& /*projectDir*/,
  const std::string& targetName, const std::string& config, bool /*fast*/,
  bool /*verbose*/, std::vector<std::string> const& makeOptions)
{
  // Select the caller- or user-preferred make program, else devenv.
  std::string makeProgramSelected =
    this->SelectMakeProgram(makeProgram, this->GetDevEnvCommand());

  // Ignore the above preference if it is msbuild.
  // Assume any other value is either a devenv or
  // command-line compatible with devenv.
  std::string makeProgramLower = makeProgramSelected;
  cmSystemTools::LowerCase(makeProgramLower);
  if (makeProgramLower.find("msbuild") != std::string::npos) {
    makeProgramSelected = this->GetDevEnvCommand();
  }

  makeCommand.push_back(makeProgramSelected);

  makeCommand.push_back(std::string(projectName) + ".sln");
  std::string realTarget = targetName;
  bool clean = false;
  if (realTarget == "clean") {
    clean = true;
    realTarget = "ALL_BUILD";
  }
  if (clean) {
    makeCommand.push_back("/clean");
  } else {
    makeCommand.push_back("/build");
  }

  if (!config.empty()) {
    makeCommand.push_back(config);
  } else {
    makeCommand.push_back("Debug");
  }
  makeCommand.push_back("/project");

  if (!realTarget.empty()) {
    makeCommand.push_back(realTarget);
  } else {
    makeCommand.push_back("ALL_BUILD");
  }
  makeCommand.insert(makeCommand.end(), makeOptions.begin(),
                     makeOptions.end());
}

///! Create a local generator appropriate to this Global Generator
cmLocalGenerator* cmGlobalVisualStudio7Generator::CreateLocalGenerator(
  cmMakefile* mf)
{
  cmLocalVisualStudio7Generator* lg =
    new cmLocalVisualStudio7Generator(this, mf);
  return lg;
}

std::string const& cmGlobalVisualStudio7Generator::GetPlatformName() const
{
  if (!this->GeneratorPlatform.empty()) {
    return this->GeneratorPlatform;
  }
  return this->DefaultPlatformName;
}

bool cmGlobalVisualStudio7Generator::SetSystemName(std::string const& s,
                                                   cmMakefile* mf)
{
  mf->AddDefinition("CMAKE_VS_INTEL_Fortran_PROJECT_VERSION",
                    this->GetIntelProjectVersion());
  return this->cmGlobalVisualStudioGenerator::SetSystemName(s, mf);
}

bool cmGlobalVisualStudio7Generator::SetGeneratorPlatform(std::string const& p,
                                                          cmMakefile* mf)
{
  if (this->GetPlatformName() == "x64") {
    mf->AddDefinition("CMAKE_FORCE_WIN64", "TRUE");
  } else if (this->GetPlatformName() == "Itanium") {
    mf->AddDefinition("CMAKE_FORCE_IA64", "TRUE");
  }
  mf->AddDefinition("CMAKE_VS_PLATFORM_NAME", this->GetPlatformName().c_str());
  return this->cmGlobalVisualStudioGenerator::SetGeneratorPlatform(p, mf);
}

void cmGlobalVisualStudio7Generator::Generate()
{
  // first do the superclass method
  this->cmGlobalVisualStudioGenerator::Generate();

  // Now write out the DSW
  this->OutputSLNFile();
  // If any solution or project files changed during the generation,
  // tell Visual Studio to reload them...
  if (!cmSystemTools::GetErrorOccuredFlag()) {
    this->CallVisualStudioMacro(MacroReload);
  }

  if (this->Version == VS8 && !this->CMakeInstance->GetIsInTryCompile()) {
    const char* cmakeWarnVS8 =
      this->CMakeInstance->GetState()->GetCacheEntryValue("CMAKE_WARN_VS8");
    if (!cmakeWarnVS8 || !cmSystemTools::IsOff(cmakeWarnVS8)) {
      this->CMakeInstance->IssueMessage(
        cmake::WARNING,
        "The \"Visual Studio 8 2005\" generator is deprecated "
        "and will be removed in a future version of CMake."
        "\n"
        "Add CMAKE_WARN_VS8=OFF to the cache to disable this warning.");
    }
  }
}

void cmGlobalVisualStudio7Generator::OutputSLNFile(
  cmLocalGenerator* root, std::vector<cmLocalGenerator*>& generators)
{
  if (generators.empty()) {
    return;
  }
  this->CurrentProject = root->GetProjectName();
  std::string fname = root->GetCurrentBinaryDirectory();
  fname += "/";
  fname += root->GetProjectName();
  fname += ".sln";
  cmGeneratedFileStream fout(fname.c_str());
  fout.SetCopyIfDifferent(true);
  if (!fout) {
    return;
  }
  this->WriteSLNFile(fout, root, generators);
  if (fout.Close()) {
    this->FileReplacedDuringGenerate(fname);
  }
}

// output the SLN file
void cmGlobalVisualStudio7Generator::OutputSLNFile()
{
  std::map<std::string, std::vector<cmLocalGenerator*> >::iterator it;
  for (it = this->ProjectMap.begin(); it != this->ProjectMap.end(); ++it) {
    this->OutputSLNFile(it->second[0], it->second);
  }
}

void cmGlobalVisualStudio7Generator::WriteTargetConfigurations(
  std::ostream& fout, std::vector<std::string> const& configs,
  OrderedTargetDependSet const& projectTargets)
{
  // loop over again and write out configurations for each target
  // in the solution
  for (OrderedTargetDependSet::const_iterator tt = projectTargets.begin();
       tt != projectTargets.end(); ++tt) {
    cmGeneratorTarget const* target = *tt;
    if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    const char* expath = target->GetProperty("EXTERNAL_MSPROJECT");
    if (expath) {
      std::set<std::string> allConfigurations(configs.begin(), configs.end());
      const char* mapping = target->GetProperty("VS_PLATFORM_MAPPING");
      this->WriteProjectConfigurations(fout, target->GetName().c_str(),
                                       *target, configs, allConfigurations,
                                       mapping ? mapping : "");
    } else {
      const std::set<std::string>& configsPartOfDefaultBuild =
        this->IsPartOfDefaultBuild(configs, projectTargets, target);
      const char* vcprojName = target->GetProperty("GENERATOR_FILE_NAME");
      if (vcprojName) {
        this->WriteProjectConfigurations(fout, vcprojName, *target, configs,
                                         configsPartOfDefaultBuild);
      }
    }
  }
}

void cmGlobalVisualStudio7Generator::WriteTargetsToSolution(
  std::ostream& fout, cmLocalGenerator* root,
  OrderedTargetDependSet const& projectTargets)
{
  VisualStudioFolders.clear();

  std::string rootBinaryDir = root->GetCurrentBinaryDirectory();
  for (OrderedTargetDependSet::const_iterator tt = projectTargets.begin();
       tt != projectTargets.end(); ++tt) {
    cmGeneratorTarget const* target = *tt;
    if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    bool written = false;

    // handle external vc project files
    const char* expath = target->GetProperty("EXTERNAL_MSPROJECT");
    if (expath) {
      std::string project = target->GetName();
      std::string location = expath;

      this->WriteExternalProject(fout, project.c_str(), location.c_str(),
                                 target->GetProperty("VS_PROJECT_TYPE"),
                                 target->GetUtilities());
      written = true;
    } else {
      const char* vcprojName = target->GetProperty("GENERATOR_FILE_NAME");
      if (vcprojName) {
        cmLocalGenerator* lg = target->GetLocalGenerator();
        std::string dir = lg->GetCurrentBinaryDirectory();
        dir = root->ConvertToRelativePath(rootBinaryDir, dir.c_str());
        if (dir == ".") {
          dir = ""; // msbuild cannot handle ".\" prefix
        }
        this->WriteProject(fout, vcprojName, dir.c_str(), target);
        written = true;
      }
    }

    // Create "solution folder" information from FOLDER target property
    //
    if (written && this->UseFolderProperty()) {
      const std::string targetFolder = target->GetEffectiveFolderName();
      if (!targetFolder.empty()) {
        std::vector<cmsys::String> tokens =
          cmSystemTools::SplitString(targetFolder, '/', false);

        std::string cumulativePath = "";

        for (std::vector<cmsys::String>::iterator iter = tokens.begin();
             iter != tokens.end(); ++iter) {
          if (!iter->size()) {
            continue;
          }

          if (cumulativePath.empty()) {
            cumulativePath = "CMAKE_FOLDER_GUID_" + *iter;
          } else {
            VisualStudioFolders[cumulativePath].insert(cumulativePath + "/" +
                                                       *iter);

            cumulativePath = cumulativePath + "/" + *iter;
          }
        }

        if (!cumulativePath.empty()) {
          VisualStudioFolders[cumulativePath].insert(target->GetName());
        }
      }
    }
  }
}

void cmGlobalVisualStudio7Generator::WriteTargetDepends(
  std::ostream& fout, OrderedTargetDependSet const& projectTargets)
{
  for (OrderedTargetDependSet::const_iterator tt = projectTargets.begin();
       tt != projectTargets.end(); ++tt) {
    cmGeneratorTarget const* target = *tt;
    if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    const char* vcprojName = target->GetProperty("GENERATOR_FILE_NAME");
    if (vcprojName) {
      std::string dir =
        target->GetLocalGenerator()->GetCurrentSourceDirectory();
      this->WriteProjectDepends(fout, vcprojName, dir.c_str(), target);
    }
  }
}

void cmGlobalVisualStudio7Generator::WriteFolders(std::ostream& fout)
{
  const char* prefix = "CMAKE_FOLDER_GUID_";
  const std::string::size_type skip_prefix = strlen(prefix);
  std::string guidProjectTypeFolder = "2150E333-8FDC-42A3-9474-1A3956D46DE8";
  for (std::map<std::string, std::set<std::string> >::iterator iter =
         VisualStudioFolders.begin();
       iter != VisualStudioFolders.end(); ++iter) {
    std::string fullName = iter->first;
    std::string guid = this->GetGUID(fullName.c_str());

    std::replace(fullName.begin(), fullName.end(), '/', '\\');
    if (cmSystemTools::StringStartsWith(fullName.c_str(), prefix)) {
      fullName = fullName.substr(skip_prefix);
    }

    std::string nameOnly = cmSystemTools::GetFilenameName(fullName);

    fout << "Project(\"{" << guidProjectTypeFolder << "}\") = \"" << nameOnly
         << "\", \"" << fullName << "\", \"{" << guid << "}\"\nEndProject\n";
  }
}

void cmGlobalVisualStudio7Generator::WriteFoldersContent(std::ostream& fout)
{
  for (std::map<std::string, std::set<std::string> >::iterator iter =
         VisualStudioFolders.begin();
       iter != VisualStudioFolders.end(); ++iter) {
    std::string key(iter->first);
    std::string guidParent(this->GetGUID(key.c_str()));

    for (std::set<std::string>::iterator it = iter->second.begin();
         it != iter->second.end(); ++it) {
      std::string value(*it);
      std::string guid(this->GetGUID(value.c_str()));

      fout << "\t\t{" << guid << "} = {" << guidParent << "}\n";
    }
  }
}

std::string cmGlobalVisualStudio7Generator::ConvertToSolutionPath(
  const char* path)
{
  // Convert to backslashes.  Do not use ConvertToOutputPath because
  // we will add quoting ourselves, and we know these projects always
  // use windows slashes.
  std::string d = path;
  std::string::size_type pos = 0;
  while ((pos = d.find('/', pos)) != d.npos) {
    d[pos++] = '\\';
  }
  return d;
}

void cmGlobalVisualStudio7Generator::WriteSLNGlobalSections(
  std::ostream& fout, cmLocalGenerator* root)
{
  std::string const guid = this->GetGUID(root->GetProjectName() + ".sln");
  bool extensibilityGlobalsOverridden = false;
  bool extensibilityAddInsOverridden = false;
  const std::vector<std::string> propKeys =
    root->GetMakefile()->GetPropertyKeys();
  for (std::vector<std::string>::const_iterator it = propKeys.begin();
       it != propKeys.end(); ++it) {
    if (it->find("VS_GLOBAL_SECTION_") == 0) {
      std::string sectionType;
      std::string name = it->substr(18);
      if (name.find("PRE_") == 0) {
        name = name.substr(4);
        sectionType = "preSolution";
      } else if (name.find("POST_") == 0) {
        name = name.substr(5);
        sectionType = "postSolution";
      } else
        continue;
      if (!name.empty()) {
        bool addGuid = false;
        if (name == "ExtensibilityGlobals" && sectionType == "postSolution") {
          addGuid = true;
          extensibilityGlobalsOverridden = true;
        } else if (name == "ExtensibilityAddIns" &&
                   sectionType == "postSolution") {
          extensibilityAddInsOverridden = true;
        }
        fout << "\tGlobalSection(" << name << ") = " << sectionType << "\n";
        std::vector<std::string> keyValuePairs;
        cmSystemTools::ExpandListArgument(
          root->GetMakefile()->GetProperty(it->c_str()), keyValuePairs);
        for (std::vector<std::string>::const_iterator itPair =
               keyValuePairs.begin();
             itPair != keyValuePairs.end(); ++itPair) {
          const std::string::size_type posEqual = itPair->find('=');
          if (posEqual != std::string::npos) {
            const std::string key =
              cmSystemTools::TrimWhitespace(itPair->substr(0, posEqual));
            const std::string value =
              cmSystemTools::TrimWhitespace(itPair->substr(posEqual + 1));
            fout << "\t\t" << key << " = " << value << "\n";
            if (key == "SolutionGuid") {
              addGuid = false;
            }
          }
        }
        if (addGuid) {
          fout << "\t\tSolutionGuid = {" << guid << "}\n";
        }
        fout << "\tEndGlobalSection\n";
      }
    }
  }
  if (!extensibilityGlobalsOverridden) {
    fout << "\tGlobalSection(ExtensibilityGlobals) = postSolution\n"
         << "\t\tSolutionGuid = {" << guid << "}\n"
         << "\tEndGlobalSection\n";
  }
  if (!extensibilityAddInsOverridden)
    fout << "\tGlobalSection(ExtensibilityAddIns) = postSolution\n"
         << "\tEndGlobalSection\n";
}

// Standard end of dsw file
void cmGlobalVisualStudio7Generator::WriteSLNFooter(std::ostream& fout)
{
  fout << "EndGlobal\n";
}

std::string cmGlobalVisualStudio7Generator::WriteUtilityDepend(
  cmGeneratorTarget const* target)
{
  std::vector<std::string> configs;
  target->Target->GetMakefile()->GetConfigurations(configs);
  std::string pname = target->GetName();
  pname += "_UTILITY";
  std::string fname = target->GetLocalGenerator()->GetCurrentBinaryDirectory();
  fname += "/";
  fname += pname;
  fname += ".vcproj";
  cmGeneratedFileStream fout(fname.c_str());
  fout.SetCopyIfDifferent(true);
  std::string guid = this->GetGUID(pname.c_str());

  /* clang-format off */
  fout <<
    "<?xml version=\"1.0\" encoding = \""
    << this->Encoding() << "\"?>\n"
    "<VisualStudioProject\n"
    "\tProjectType=\"Visual C++\"\n"
    "\tVersion=\"" << this->GetIDEVersion() << "0\"\n"
    "\tName=\"" << pname << "\"\n"
    "\tProjectGUID=\"{" << guid << "}\"\n"
    "\tKeyword=\"Win32Proj\">\n"
    "\t<Platforms><Platform Name=\"Win32\"/></Platforms>\n"
    "\t<Configurations>\n"
    ;
  /* clang-format on */
  for (std::vector<std::string>::iterator i = configs.begin();
       i != configs.end(); ++i) {
    /* clang-format off */
    fout <<
      "\t\t<Configuration\n"
      "\t\t\tName=\"" << *i << "|Win32\"\n"
      "\t\t\tOutputDirectory=\"" << *i << "\"\n"
      "\t\t\tIntermediateDirectory=\"" << pname << ".dir\\" << *i << "\"\n"
      "\t\t\tConfigurationType=\"10\"\n"
      "\t\t\tUseOfMFC=\"0\"\n"
      "\t\t\tATLMinimizesCRunTimeLibraryUsage=\"FALSE\"\n"
      "\t\t\tCharacterSet=\"2\">\n"
      "\t\t</Configuration>\n"
      ;
    /* clang-format on */
  }
  /* clang-format off */
  fout <<
    "\t</Configurations>\n"
    "\t<Files></Files>\n"
    "\t<Globals></Globals>\n"
    "</VisualStudioProject>\n"
    ;
  /* clang-format on */

  if (fout.Close()) {
    this->FileReplacedDuringGenerate(fname);
  }
  return pname;
}

std::string cmGlobalVisualStudio7Generator::GetGUID(std::string const& name)
{
  std::string const& guidStoreName = name + "_GUID_CMAKE";
  if (const char* storedGUID =
        this->CMakeInstance->GetCacheDefinition(guidStoreName.c_str())) {
    return std::string(storedGUID);
  }
  // Compute a GUID that is deterministic but unique to the build tree.
  std::string input = this->CMakeInstance->GetState()->GetBinaryDirectory();
  input += "|";
  input += name;

  cmUuid uuidGenerator;

  std::vector<unsigned char> uuidNamespace;
  uuidGenerator.StringToBinary("ee30c4be-5192-4fb0-b335-722a2dffe760",
                               uuidNamespace);

  std::string guid = uuidGenerator.FromMd5(uuidNamespace, input);

  return cmSystemTools::UpperCase(guid);
}

void cmGlobalVisualStudio7Generator::AppendDirectoryForConfig(
  const std::string& prefix, const std::string& config,
  const std::string& suffix, std::string& dir)
{
  if (!config.empty()) {
    dir += prefix;
    dir += config;
    dir += suffix;
  }
}

std::set<std::string> cmGlobalVisualStudio7Generator::IsPartOfDefaultBuild(
  std::vector<std::string> const& configs,
  OrderedTargetDependSet const& projectTargets,
  cmGeneratorTarget const* target)
{
  std::set<std::string> activeConfigs;
  // if it is a utilitiy target then only make it part of the
  // default build if another target depends on it
  int type = target->GetType();
  if (type == cmStateEnums::GLOBAL_TARGET) {
    std::vector<std::string> targetNames;
    targetNames.push_back("INSTALL");
    targetNames.push_back("PACKAGE");
    for (std::vector<std::string>::const_iterator t = targetNames.begin();
         t != targetNames.end(); ++t) {
      // check if target <*t> is part of default build
      if (target->GetName() == *t) {
        const std::string propertyName =
          "CMAKE_VS_INCLUDE_" + *t + "_TO_DEFAULT_BUILD";
        // inspect CMAKE_VS_INCLUDE_<*t>_TO_DEFAULT_BUILD properties
        for (std::vector<std::string>::const_iterator i = configs.begin();
             i != configs.end(); ++i) {
          const char* propertyValue =
            target->Target->GetMakefile()->GetDefinition(propertyName);
          cmGeneratorExpression ge;
          CM_AUTO_PTR<cmCompiledGeneratorExpression> cge =
            ge.Parse(propertyValue);
          if (cmSystemTools::IsOn(
                cge->Evaluate(target->GetLocalGenerator(), *i))) {
            activeConfigs.insert(*i);
          }
        }
      }
    }
    return activeConfigs;
  }
  if (type == cmStateEnums::UTILITY &&
      !this->IsDependedOn(projectTargets, target)) {
    return activeConfigs;
  }
  // inspect EXCLUDE_FROM_DEFAULT_BUILD[_<CONFIG>] properties
  for (std::vector<std::string>::const_iterator i = configs.begin();
       i != configs.end(); ++i) {
    const char* propertyValue =
      target->GetFeature("EXCLUDE_FROM_DEFAULT_BUILD", i->c_str());
    if (cmSystemTools::IsOff(propertyValue)) {
      activeConfigs.insert(*i);
    }
  }
  return activeConfigs;
}

bool cmGlobalVisualStudio7Generator::IsDependedOn(
  OrderedTargetDependSet const& projectTargets, cmGeneratorTarget const* gtIn)
{
  for (OrderedTargetDependSet::const_iterator l = projectTargets.begin();
       l != projectTargets.end(); ++l) {
    TargetDependSet const& tgtdeps = this->GetTargetDirectDepends(*l);
    if (tgtdeps.count(gtIn)) {
      return true;
    }
  }
  return false;
}

std::string cmGlobalVisualStudio7Generator::Encoding()
{
  return "UTF-8";
}
