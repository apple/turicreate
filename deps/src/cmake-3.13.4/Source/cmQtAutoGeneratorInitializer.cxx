/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmQtAutoGeneratorInitializer.h"
#include "cmQtAutoGeneratorCommon.h"

#include "cmAlgorithms.h"
#include "cmCustomCommandLines.h"
#include "cmFilePathChecksum.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmOutputConverter.h"
#include "cmSourceFile.h"
#include "cmSourceGroup.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cm_sys_stat.h"
#include "cmake.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
#include "cmGlobalVisualStudioGenerator.h"
#endif

#include "cmConfigure.h"
#include "cmsys/FStream.hxx"
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

static void utilCopyTargetProperty(cmTarget* destinationTarget,
                                   cmTarget* sourceTarget,
                                   const std::string& propertyName)
{
  const char* propertyValue = sourceTarget->GetProperty(propertyName);
  if (propertyValue) {
    destinationTarget->SetProperty(propertyName, propertyValue);
  }
}

inline static bool PropertyEnabled(cmSourceFile* sourceFile, const char* key)
{
  return cmSystemTools::IsOn(sourceFile->GetPropertyForUser(key));
}

static std::string GetSafeProperty(cmGeneratorTarget const* target,
                                   const char* key)
{
  const char* tmp = target->GetProperty(key);
  return std::string((tmp != CM_NULLPTR) ? tmp : "");
}

static std::string GetAutogenTargetName(cmGeneratorTarget const* target)
{
  std::string autogenTargetName = target->GetName();
  autogenTargetName += "_autogen";
  return autogenTargetName;
}

static std::string GetAutogenTargetFilesDir(cmGeneratorTarget const* target)
{
  cmMakefile* makefile = target->Target->GetMakefile();
  std::string targetDir = makefile->GetCurrentBinaryDirectory();
  targetDir += makefile->GetCMakeInstance()->GetCMakeFilesDirectory();
  targetDir += "/";
  targetDir += GetAutogenTargetName(target);
  targetDir += ".dir";
  return targetDir;
}

static std::string GetAutogenTargetBuildDir(cmGeneratorTarget const* target)
{
  std::string targetDir = GetSafeProperty(target, "AUTOGEN_BUILD_DIR");
  if (targetDir.empty()) {
    cmMakefile* makefile = target->Target->GetMakefile();
    targetDir = makefile->GetCurrentBinaryDirectory();
    targetDir += "/";
    targetDir += GetAutogenTargetName(target);
  }
  return targetDir;
}

static std::string GetQtMajorVersion(cmGeneratorTarget const* target)
{
  cmMakefile* makefile = target->Target->GetMakefile();
  std::string qtMajorVersion = makefile->GetSafeDefinition("QT_VERSION_MAJOR");
  if (qtMajorVersion.empty()) {
    qtMajorVersion = makefile->GetSafeDefinition("Qt5Core_VERSION_MAJOR");
  }
  const char* targetQtVersion =
    target->GetLinkInterfaceDependentStringProperty("QT_MAJOR_VERSION", "");
  if (targetQtVersion != CM_NULLPTR) {
    qtMajorVersion = targetQtVersion;
  }
  return qtMajorVersion;
}

static std::string GetQtMinorVersion(cmGeneratorTarget const* target,
                                     const std::string& qtMajorVersion)
{
  cmMakefile* makefile = target->Target->GetMakefile();
  std::string qtMinorVersion;
  if (qtMajorVersion == "5") {
    qtMinorVersion = makefile->GetSafeDefinition("Qt5Core_VERSION_MINOR");
  }
  if (qtMinorVersion.empty()) {
    qtMinorVersion = makefile->GetSafeDefinition("QT_VERSION_MINOR");
  }

  const char* targetQtVersion =
    target->GetLinkInterfaceDependentStringProperty("QT_MINOR_VERSION", "");
  if (targetQtVersion != CM_NULLPTR) {
    qtMinorVersion = targetQtVersion;
  }
  return qtMinorVersion;
}

static bool QtVersionGreaterOrEqual(const std::string& major,
                                    const std::string& minor,
                                    unsigned long requestMajor,
                                    unsigned long requestMinor)
{
  unsigned long majorUL(0);
  unsigned long minorUL(0);
  if (cmSystemTools::StringToULong(major.c_str(), &majorUL) &&
      cmSystemTools::StringToULong(minor.c_str(), &minorUL)) {
    return (majorUL > requestMajor) ||
      (majorUL == requestMajor && minorUL >= requestMinor);
  }
  return false;
}

static void GetCompileDefinitionsAndDirectories(
  cmGeneratorTarget const* target, const std::string& config,
  std::string& incs, std::string& defs)
{
  cmLocalGenerator* localGen = target->GetLocalGenerator();
  {
    std::vector<std::string> includeDirs;
    // Get the include dirs for this target, without stripping the implicit
    // include dirs off, see
    // https://gitlab.kitware.com/cmake/cmake/issues/13667
    localGen->GetIncludeDirectories(includeDirs, target, "CXX", config, false);
    incs = cmJoin(includeDirs, ";");
  }
  {
    std::set<std::string> defines;
    localGen->AddCompileDefinitions(defines, target, config, "CXX");
    defs += cmJoin(defines, ";");
  }
}

static bool IsMultiConfig(cmGlobalGenerator* globalGen)
{
  // FIXME: Xcode does not support per-config sources, yet.
  //        (EXCLUDED_SOURCE_FILE_NAMES)
  //        Treat it as a single configuration generator meanwhile.
  if (globalGen->GetName().find("Xcode") != std::string::npos) {
    return false;
  }
  // FIXME: Visual Studio does not fully support per-config sources yet.
  if (globalGen->GetName().find("Visual Studio") != std::string::npos) {
    return false;
  }
  return globalGen->IsMultiConfig();
}

static std::vector<std::string> GetConfigurations(
  cmMakefile* makefile, std::string* config = CM_NULLPTR)
{
  std::vector<std::string> configs;
  {
    std::string cfg = makefile->GetConfigurations(configs);
    if (config != CM_NULLPTR) {
      *config = cfg;
    }
  }
  // Add empty configuration on demand
  if (configs.empty()) {
    configs.push_back("");
  }
  return configs;
}

static std::vector<std::string> GetConfigurationSuffixes(cmMakefile* makefile)
{
  std::vector<std::string> suffixes;
  if (IsMultiConfig(makefile->GetGlobalGenerator())) {
    makefile->GetConfigurations(suffixes);
    for (std::vector<std::string>::iterator it = suffixes.begin();
         it != suffixes.end(); ++it) {
      it->insert(0, "_");
    }
  }
  if (suffixes.empty()) {
    suffixes.push_back("");
  }
  return suffixes;
}

static void AddDefinitionEscaped(cmMakefile* makefile, const char* key,
                                 const std::string& value)
{
  makefile->AddDefinition(key,
                          cmOutputConverter::EscapeForCMake(value).c_str());
}

static void AddDefinitionEscaped(cmMakefile* makefile, const char* key,
                                 const std::vector<std::string>& values)
{
  makefile->AddDefinition(
    key, cmOutputConverter::EscapeForCMake(cmJoin(values, ";")).c_str());
}

static bool AddToSourceGroup(cmMakefile* makefile, const std::string& fileName,
                             cmQtAutoGeneratorCommon::GeneratorType genType)
{
  cmSourceGroup* sourceGroup = CM_NULLPTR;
  // Acquire source group
  {
    const char* groupName = CM_NULLPTR;
    // Use generator specific group name
    switch (genType) {
      case cmQtAutoGeneratorCommon::MOC:
        groupName =
          makefile->GetState()->GetGlobalProperty("AUTOMOC_SOURCE_GROUP");
        break;
      case cmQtAutoGeneratorCommon::RCC:
        groupName =
          makefile->GetState()->GetGlobalProperty("AUTORCC_SOURCE_GROUP");
        break;
      default:
        break;
    }
    // Use default group name on demand
    if ((groupName == CM_NULLPTR) || (*groupName == 0)) {
      groupName =
        makefile->GetState()->GetGlobalProperty("AUTOGEN_SOURCE_GROUP");
    }
    // Generate a source group on demand
    if ((groupName != CM_NULLPTR) && (*groupName != 0)) {
      {
        const char* delimiter =
          makefile->GetDefinition("SOURCE_GROUP_DELIMITER");
        if (delimiter == CM_NULLPTR) {
          delimiter = "\\";
        }
        std::vector<std::string> folders =
          cmSystemTools::tokenize(groupName, delimiter);
        sourceGroup = makefile->GetSourceGroup(folders);
        if (sourceGroup == CM_NULLPTR) {
          makefile->AddSourceGroup(folders);
          sourceGroup = makefile->GetSourceGroup(folders);
        }
      }
      if (sourceGroup == CM_NULLPTR) {
        cmSystemTools::Error(
          "Autogen: Could not create or find source group: ",
          cmQtAutoGeneratorCommon::Quoted(groupName).c_str());
        return false;
      }
    }
  }
  if (sourceGroup != CM_NULLPTR) {
    sourceGroup->AddGroupFile(fileName);
  }
  return true;
}

static void AddGeneratedSource(cmMakefile* makefile,
                               const std::string& filename,
                               cmQtAutoGeneratorCommon::GeneratorType genType)
{
  cmSourceFile* gFile = makefile->GetOrCreateSource(filename, true);
  gFile->SetProperty("GENERATED", "1");
  gFile->SetProperty("SKIP_AUTOGEN", "On");

  AddToSourceGroup(makefile, filename, genType);
}

static void AcquireScanFiles(cmGeneratorTarget const* target,
                             std::vector<std::string>& mocUicSources,
                             std::vector<std::string>& mocUicHeaders,
                             std::vector<std::string>& mocSkipList,
                             std::vector<std::string>& uicSkipList)
{
  const bool mocTarget = target->GetPropertyAsBool("AUTOMOC");
  const bool uicTarget = target->GetPropertyAsBool("AUTOUIC");

  std::vector<cmSourceFile*> srcFiles;
  target->GetConfigCommonSourceFiles(srcFiles);
  for (std::vector<cmSourceFile*>::const_iterator fileIt = srcFiles.begin();
       fileIt != srcFiles.end(); ++fileIt) {
    cmSourceFile* sf = *fileIt;
    const cmSystemTools::FileFormat fileType =
      cmSystemTools::GetFileFormat(sf->GetExtension().c_str());

    if (!(fileType == cmSystemTools::CXX_FILE_FORMAT) &&
        !(fileType == cmSystemTools::HEADER_FILE_FORMAT)) {
      continue;
    }
    if (PropertyEnabled(sf, "GENERATED") &&
        !target->GetPropertyAsBool("__UNDOCUMENTED_AUTOGEN_GENERATED_FILES")) {
      // FIXME: Add a policy whose NEW behavior allows generated files.
      // The implementation already works.  We disable it here to avoid
      // changing behavior for existing projects that do not expect it.
      continue;
    }
    const std::string absFile =
      cmsys::SystemTools::GetRealPath(sf->GetFullPath());
    // Skip flags
    const bool skipAll = PropertyEnabled(sf, "SKIP_AUTOGEN");
    const bool mocSkip = skipAll || PropertyEnabled(sf, "SKIP_AUTOMOC");
    const bool uicSkip = skipAll || PropertyEnabled(sf, "SKIP_AUTOUIC");
    // Add file name to skip lists.
    // Do this even when the file is not added to the sources/headers lists
    // because the file name may be extracted from an other file when
    // processing
    if (mocSkip) {
      mocSkipList.push_back(absFile);
    }
    if (uicSkip) {
      uicSkipList.push_back(absFile);
    }

    if ((mocTarget && !mocSkip) || (uicTarget && !uicSkip)) {
      // Add file name to sources or headers list
      switch (fileType) {
        case cmSystemTools::CXX_FILE_FORMAT:
          mocUicSources.push_back(absFile);
          break;
        case cmSystemTools::HEADER_FILE_FORMAT:
          mocUicHeaders.push_back(absFile);
          break;
        default:
          break;
      }
    }
  }
}

static void MocSetupAutoTarget(
  cmGeneratorTarget const* target, const std::string& autogenTargetName,
  std::string const& qtMajorVersion, std::string const& config,
  std::vector<std::string> const& configs,
  std::vector<std::string> const& mocSkipList,
  std::map<std::string, std::string>& configMocIncludes,
  std::map<std::string, std::string>& configMocDefines)
{
  cmLocalGenerator* lg = target->GetLocalGenerator();
  cmMakefile* makefile = target->Target->GetMakefile();

  AddDefinitionEscaped(makefile, "_moc_options",
                       GetSafeProperty(target, "AUTOMOC_MOC_OPTIONS"));
  AddDefinitionEscaped(makefile, "_moc_skip", mocSkipList);
  AddDefinitionEscaped(makefile, "_moc_relaxed_mode",
                       makefile->IsOn("CMAKE_AUTOMOC_RELAXED_MODE") ? "TRUE"
                                                                    : "FALSE");
  AddDefinitionEscaped(makefile, "_moc_depend_filters",
                       GetSafeProperty(target, "AUTOMOC_DEPEND_FILTERS"));

  if (QtVersionGreaterOrEqual(
        qtMajorVersion, GetQtMinorVersion(target, qtMajorVersion), 5, 8)) {
    AddDefinitionEscaped(
      makefile, "_moc_predefs_cmd",
      makefile->GetSafeDefinition("CMAKE_CXX_COMPILER_PREDEFINES_COMMAND"));
  }
  // Moc includes and compile definitions
  {
    // Default settings
    std::string incs;
    std::string compileDefs;
    GetCompileDefinitionsAndDirectories(target, config, incs, compileDefs);
    AddDefinitionEscaped(makefile, "_moc_incs", incs);
    AddDefinitionEscaped(makefile, "_moc_compile_defs", compileDefs);

    // Configuration specific settings
    for (std::vector<std::string>::const_iterator li = configs.begin();
         li != configs.end(); ++li) {
      std::string configIncs;
      std::string configCompileDefs;
      GetCompileDefinitionsAndDirectories(target, *li, configIncs,
                                          configCompileDefs);
      if (configIncs != incs) {
        configMocIncludes[*li] = cmOutputConverter::EscapeForCMake(configIncs);
      }
      if (configCompileDefs != compileDefs) {
        configMocDefines[*li] =
          cmOutputConverter::EscapeForCMake(configCompileDefs);
      }
    }
  }

  // Moc executable
  {
    std::string err;
    const char* mocExec = CM_NULLPTR;
    if (qtMajorVersion == "5") {
      cmGeneratorTarget* qt5Moc = lg->FindGeneratorTargetToUse("Qt5::moc");
      if (qt5Moc != CM_NULLPTR) {
        mocExec = qt5Moc->ImportedGetLocation("");
      } else {
        err = "Qt5::moc target not found " + autogenTargetName;
      }
    } else if (qtMajorVersion == "4") {
      cmGeneratorTarget* qt4Moc = lg->FindGeneratorTargetToUse("Qt4::moc");
      if (qt4Moc != CM_NULLPTR) {
        mocExec = qt4Moc->ImportedGetLocation("");
      } else {
        err = "Qt4::moc target not found " + autogenTargetName;
      }
    } else {
      err = "The CMAKE_AUTOMOC feature supports only Qt 4 and Qt 5 ";
      err += autogenTargetName;
    }
    // Add definition or error
    if (err.empty()) {
      AddDefinitionEscaped(makefile, "_qt_moc_executable",
                           mocExec ? mocExec : "");
    } else {
      cmSystemTools::Error(err.c_str());
    }
  }
}

static void UicGetOpts(cmGeneratorTarget const* target,
                       const std::string& config, std::string& optString)
{
  std::vector<std::string> opts;
  target->GetAutoUicOptions(opts, config);
  optString = cmJoin(opts, ";");
}

static void UicSetupAutoTarget(
  cmGeneratorTarget const* target, std::string const& qtMajorVersion,
  std::string const& config, std::vector<std::string> const& configs,
  std::vector<std::string> const& uicSkipList,
  std::map<std::string, std::string>& configUicOptions)
{
  cmLocalGenerator* lg = target->GetLocalGenerator();
  cmMakefile* makefile = target->Target->GetMakefile();

  AddDefinitionEscaped(makefile, "_uic_skip", uicSkipList);

  // Uic search paths
  {
    std::vector<std::string> uicSearchPaths;
    cmSystemTools::ExpandListArgument(
      GetSafeProperty(target, "AUTOUIC_SEARCH_PATHS"), uicSearchPaths);
    const std::string srcDir = makefile->GetCurrentSourceDirectory();
    for (std::vector<std::string>::iterator it = uicSearchPaths.begin();
         it != uicSearchPaths.end(); ++it) {
      *it = cmSystemTools::CollapseFullPath(*it, srcDir);
    }
    AddDefinitionEscaped(makefile, "_uic_search_paths", uicSearchPaths);
  }
  // Uic target options
  {
    // Default settings
    std::string uicOpts;
    UicGetOpts(target, config, uicOpts);
    AddDefinitionEscaped(makefile, "_uic_target_options", uicOpts);

    // Configuration specific settings
    for (std::vector<std::string>::const_iterator li = configs.begin();
         li != configs.end(); ++li) {
      std::string configUicOpts;
      UicGetOpts(target, *li, configUicOpts);
      if (configUicOpts != uicOpts) {
        configUicOptions[*li] =
          cmOutputConverter::EscapeForCMake(configUicOpts);
      }
    }
  }
  // Uic files options
  {
    std::vector<std::string> uiFileFiles;
    std::vector<std::string> uiFileOptions;
    {
      std::set<std::string> skipped;
      skipped.insert(uicSkipList.begin(), uicSkipList.end());

      const std::vector<cmSourceFile*> uiFilesWithOptions =
        makefile->GetQtUiFilesWithOptions();
      for (std::vector<cmSourceFile*>::const_iterator fileIt =
             uiFilesWithOptions.begin();
           fileIt != uiFilesWithOptions.end(); ++fileIt) {
        cmSourceFile* sf = *fileIt;
        const std::string absFile =
          cmsys::SystemTools::GetRealPath(sf->GetFullPath());
        if (skipped.insert(absFile).second) {
          // The file wasn't skipped
          uiFileFiles.push_back(absFile);
          {
            std::string opts = sf->GetProperty("AUTOUIC_OPTIONS");
            cmSystemTools::ReplaceString(opts, ";",
                                         cmQtAutoGeneratorCommon::listSep);
            uiFileOptions.push_back(opts);
          }
        }
      }
    }
    AddDefinitionEscaped(makefile, "_qt_uic_options_files", uiFileFiles);
    AddDefinitionEscaped(makefile, "_qt_uic_options_options", uiFileOptions);
  }

  // Uic executable
  {
    std::string err;
    const char* uicExec = CM_NULLPTR;
    if (qtMajorVersion == "5") {
      cmGeneratorTarget* qt5Uic = lg->FindGeneratorTargetToUse("Qt5::uic");
      if (qt5Uic != CM_NULLPTR) {
        uicExec = qt5Uic->ImportedGetLocation("");
      } else {
        // Project does not use Qt5Widgets, but has AUTOUIC ON anyway
      }
    } else if (qtMajorVersion == "4") {
      cmGeneratorTarget* qt4Uic = lg->FindGeneratorTargetToUse("Qt4::uic");
      if (qt4Uic != CM_NULLPTR) {
        uicExec = qt4Uic->ImportedGetLocation("");
      } else {
        err = "Qt4::uic target not found " + target->GetName();
      }
    } else {
      err = "The CMAKE_AUTOUIC feature supports only Qt 4 and Qt 5 ";
      err += target->GetName();
    }
    // Add definition or error
    if (err.empty()) {
      AddDefinitionEscaped(makefile, "_qt_uic_executable",
                           uicExec ? uicExec : "");
    } else {
      cmSystemTools::Error(err.c_str());
    }
  }
}

static std::string RccGetExecutable(cmGeneratorTarget const* target,
                                    const std::string& qtMajorVersion)
{
  std::string rccExec;
  cmLocalGenerator* lg = target->GetLocalGenerator();
  if (qtMajorVersion == "5") {
    cmGeneratorTarget* qt5Rcc = lg->FindGeneratorTargetToUse("Qt5::rcc");
    if (qt5Rcc != CM_NULLPTR) {
      rccExec = qt5Rcc->ImportedGetLocation("");
    } else {
      cmSystemTools::Error("Qt5::rcc target not found ",
                           target->GetName().c_str());
    }
  } else if (qtMajorVersion == "4") {
    cmGeneratorTarget* qt4Rcc = lg->FindGeneratorTargetToUse("Qt4::rcc");
    if (qt4Rcc != CM_NULLPTR) {
      rccExec = qt4Rcc->ImportedGetLocation("");
    } else {
      cmSystemTools::Error("Qt4::rcc target not found ",
                           target->GetName().c_str());
    }
  } else {
    cmSystemTools::Error(
      "The CMAKE_AUTORCC feature supports only Qt 4 and Qt 5 ",
      target->GetName().c_str());
  }
  return rccExec;
}

static void RccMergeOptions(std::vector<std::string>& opts,
                            const std::vector<std::string>& fileOpts,
                            bool isQt5)
{
  static const char* valueOptions[] = { "name", "root", "compress",
                                        "threshold" };
  std::vector<std::string> extraOpts;
  for (std::vector<std::string>::const_iterator fit = fileOpts.begin();
       fit != fileOpts.end(); ++fit) {
    std::vector<std::string>::iterator existingIt =
      std::find(opts.begin(), opts.end(), *fit);
    if (existingIt != opts.end()) {
      const char* optName = fit->c_str();
      if (*optName == '-') {
        ++optName;
        if (isQt5 && *optName == '-') {
          ++optName;
        }
      }
      // Test if this is a value option and change the existing value
      if ((optName != fit->c_str()) &&
          std::find_if(cmArrayBegin(valueOptions), cmArrayEnd(valueOptions),
                       cmStrCmp(optName)) != cmArrayEnd(valueOptions)) {
        const std::vector<std::string>::iterator existValueIt(existingIt + 1);
        const std::vector<std::string>::const_iterator fileValueIt(fit + 1);
        if ((existValueIt != opts.end()) && (fileValueIt != fileOpts.end())) {
          *existValueIt = *fileValueIt;
          ++fit;
        }
      }
    } else {
      extraOpts.push_back(*fit);
    }
  }
  opts.insert(opts.end(), extraOpts.begin(), extraOpts.end());
}

static void RccSetupAutoTarget(cmGeneratorTarget const* target,
                               const std::string& qtMajorVersion)
{
  cmMakefile* makefile = target->Target->GetMakefile();
  const bool qtMajorVersion5 = (qtMajorVersion == "5");
  const std::string rccCommand = RccGetExecutable(target, qtMajorVersion);
  std::vector<std::string> _rcc_files;
  std::vector<std::string> _rcc_inputs;
  std::vector<std::string> rccFileFiles;
  std::vector<std::string> rccFileOptions;
  std::vector<std::string> rccOptionsTarget;
  if (const char* opts = target->GetProperty("AUTORCC_OPTIONS")) {
    cmSystemTools::ExpandListArgument(opts, rccOptionsTarget);
  }

  std::vector<cmSourceFile*> srcFiles;
  target->GetConfigCommonSourceFiles(srcFiles);
  for (std::vector<cmSourceFile*>::const_iterator fileIt = srcFiles.begin();
       fileIt != srcFiles.end(); ++fileIt) {
    cmSourceFile* sf = *fileIt;
    if ((sf->GetExtension() == "qrc") &&
        !PropertyEnabled(sf, "SKIP_AUTOGEN") &&
        !PropertyEnabled(sf, "SKIP_AUTORCC")) {
      const std::string absFile =
        cmsys::SystemTools::GetRealPath(sf->GetFullPath());
      // qrc file
      _rcc_files.push_back(absFile);
      // qrc file entries
      {
        std::string entriesList = "{";
        // Read input file list only for non generated .qrc files.
        if (!PropertyEnabled(sf, "GENERATED")) {
          std::string error;
          std::vector<std::string> files;
          if (cmQtAutoGeneratorCommon::RccListInputs(
                qtMajorVersion, rccCommand, absFile, files, &error)) {
            entriesList += cmJoin(files, cmQtAutoGeneratorCommon::listSep);
          } else {
            cmSystemTools::Error(error.c_str());
          }
        }
        entriesList += "}";
        _rcc_inputs.push_back(entriesList);
      }
      // rcc options for this qrc file
      {
        // Merged target and file options
        std::vector<std::string> rccOptions(rccOptionsTarget);
        if (const char* prop = sf->GetProperty("AUTORCC_OPTIONS")) {
          std::vector<std::string> optsVec;
          cmSystemTools::ExpandListArgument(prop, optsVec);
          RccMergeOptions(rccOptions, optsVec, qtMajorVersion5);
        }
        // Only store non empty options lists
        if (!rccOptions.empty()) {
          rccFileFiles.push_back(absFile);
          rccFileOptions.push_back(
            cmJoin(rccOptions, cmQtAutoGeneratorCommon::listSep));
        }
      }
    }
  }

  AddDefinitionEscaped(makefile, "_qt_rcc_executable", rccCommand);
  AddDefinitionEscaped(makefile, "_rcc_files", _rcc_files);
  AddDefinitionEscaped(makefile, "_rcc_inputs", _rcc_inputs);
  AddDefinitionEscaped(makefile, "_rcc_options_files", rccFileFiles);
  AddDefinitionEscaped(makefile, "_rcc_options_options", rccFileOptions);
}

void cmQtAutoGeneratorInitializer::InitializeAutogenSources(
  cmGeneratorTarget* target)
{
  if (target->GetPropertyAsBool("AUTOMOC")) {
    cmMakefile* makefile = target->Target->GetMakefile();
    const std::vector<std::string> suffixes =
      GetConfigurationSuffixes(makefile);
    // Get build directory
    const std::string autogenBuildDir = GetAutogenTargetBuildDir(target);
    // Register all compilation files as generated
    for (std::vector<std::string>::const_iterator it = suffixes.begin();
         it != suffixes.end(); ++it) {
      std::string mcFile = autogenBuildDir + "/mocs_compilation";
      mcFile += *it;
      mcFile += ".cpp";
      AddGeneratedSource(makefile, mcFile, cmQtAutoGeneratorCommon::MOC);
    }
    // Mocs compilation file
    if (IsMultiConfig(target->GetGlobalGenerator())) {
      target->AddSource(autogenBuildDir + "/mocs_compilation_$<CONFIG>.cpp");
    } else {
      target->AddSource(autogenBuildDir + "/mocs_compilation.cpp");
    }
  }
}

void cmQtAutoGeneratorInitializer::InitializeAutogenTarget(
  cmLocalGenerator* lg, cmGeneratorTarget* target)
{
  cmMakefile* makefile = target->Target->GetMakefile();

  // Create a custom target for running generators at buildtime
  const bool mocEnabled = target->GetPropertyAsBool("AUTOMOC");
  const bool uicEnabled = target->GetPropertyAsBool("AUTOUIC");
  const bool rccEnabled = target->GetPropertyAsBool("AUTORCC");
  const bool multiConfig = IsMultiConfig(target->GetGlobalGenerator());
  const std::string autogenTargetName = GetAutogenTargetName(target);
  const std::string autogenBuildDir = GetAutogenTargetBuildDir(target);
  const std::string workingDirectory =
    cmSystemTools::CollapseFullPath("", makefile->GetCurrentBinaryDirectory());
  const std::string qtMajorVersion = GetQtMajorVersion(target);
  const std::string rccCommand = RccGetExecutable(target, qtMajorVersion);
  const std::vector<std::string> suffixes = GetConfigurationSuffixes(makefile);
  std::vector<std::string> autogenDependFiles;
  std::vector<std::string> autogenDependTargets;
  std::vector<std::string> autogenProvides;

  // Remove build directories on cleanup
  makefile->AppendProperty("ADDITIONAL_MAKE_CLEAN_FILES",
                           autogenBuildDir.c_str(), false);

  // Remove old settings on cleanup
  {
    std::string base = GetAutogenTargetFilesDir(target);
    for (std::vector<std::string>::const_iterator it = suffixes.begin();
         it != suffixes.end(); ++it) {
      std::string fname = base + "/AutogenOldSettings" + *it + ".cmake";
      makefile->AppendProperty("ADDITIONAL_MAKE_CLEAN_FILES", fname.c_str(),
                               false);
    }
  }

  // Compose command lines
  cmCustomCommandLines commandLines;
  {
    cmCustomCommandLine currentLine;
    currentLine.push_back(cmSystemTools::GetCMakeCommand());
    currentLine.push_back("-E");
    currentLine.push_back("cmake_autogen");
    currentLine.push_back(GetAutogenTargetFilesDir(target));
    currentLine.push_back("$<CONFIGURATION>");
    commandLines.push_back(currentLine);
  }

  // Compose target comment
  std::string autogenComment;
  {
    std::vector<std::string> toolNames;
    if (mocEnabled) {
      toolNames.push_back("MOC");
    }
    if (uicEnabled) {
      toolNames.push_back("UIC");
    }
    if (rccEnabled) {
      toolNames.push_back("RCC");
    }

    std::string tools = toolNames[0];
    toolNames.erase(toolNames.begin());
    while (toolNames.size() > 1) {
      tools += ", " + toolNames[0];
      toolNames.erase(toolNames.begin());
    }
    if (toolNames.size() == 1) {
      tools += " and " + toolNames[0];
    }
    autogenComment = "Automatic " + tools + " for target " + target->GetName();
  }

  // Add moc compilation to generated files list
  if (mocEnabled) {
    for (std::vector<std::string>::const_iterator it = suffixes.begin();
         it != suffixes.end(); ++it) {
      std::string mcFile = autogenBuildDir + "/mocs_compilation";
      mcFile += *it;
      mcFile += ".cpp";
      autogenProvides.push_back(mcFile);
    }
  }

  // Add autogen includes directory to the origin target INCLUDE_DIRECTORIES
  if (mocEnabled || uicEnabled) {
    if (multiConfig) {
      target->AddIncludeDirectory(autogenBuildDir + "/include_$<CONFIG>",
                                  true);

    } else {
      target->AddIncludeDirectory(autogenBuildDir + "/include", true);
    }
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  bool usePRE_BUILD = false;
  cmGlobalGenerator* gg = lg->GetGlobalGenerator();
  if (gg->GetName().find("Visual Studio") != std::string::npos) {
    // Under VS use a PRE_BUILD event instead of a separate target to
    // reduce the number of targets loaded into the IDE.
    // This also works around a VS 11 bug that may skip updating the target:
    //  https://connect.microsoft.com/VisualStudio/feedback/details/769495
    usePRE_BUILD = true;
  }
#endif

  // Initialize autogen target dependencies
  if (const char* extraDeps = target->GetProperty("AUTOGEN_TARGET_DEPENDS")) {
    std::vector<std::string> deps;
    cmSystemTools::ExpandListArgument(extraDeps, deps);
    for (std::vector<std::string>::const_iterator itC = deps.begin(),
                                                  itE = deps.end();
         itC != itE; ++itC) {
      if (makefile->FindTargetToUse(*itC) != CM_NULLPTR) {
        autogenDependTargets.push_back(*itC);
      } else {
        autogenDependFiles.push_back(*itC);
      }
    }
  }
  {
    cmFilePathChecksum fpathCheckSum(makefile);
    // Iterate over all source files
    std::vector<cmSourceFile*> srcFiles;
    target->GetConfigCommonSourceFiles(srcFiles);
    for (std::vector<cmSourceFile*>::const_iterator fileIt = srcFiles.begin();
         fileIt != srcFiles.end(); ++fileIt) {
      cmSourceFile* sf = *fileIt;
      if (!PropertyEnabled(sf, "SKIP_AUTOGEN")) {
        std::string const& ext = sf->GetExtension();
        // Add generated file that will be scanned by moc or uic to
        // the dependencies
        if (mocEnabled || uicEnabled) {
          const cmSystemTools::FileFormat fileType =
            cmSystemTools::GetFileFormat(ext.c_str());
          if ((fileType == cmSystemTools::CXX_FILE_FORMAT) ||
              (fileType == cmSystemTools::HEADER_FILE_FORMAT)) {
            if (PropertyEnabled(sf, "GENERATED")) {
              if ((mocEnabled && !PropertyEnabled(sf, "SKIP_AUTOMOC")) ||
                  (uicEnabled && !PropertyEnabled(sf, "SKIP_AUTOUIC"))) {
                autogenDependFiles.push_back(
                  cmsys::SystemTools::GetRealPath(sf->GetFullPath()));
#if defined(_WIN32) && !defined(__CYGWIN__)
                // Cannot use PRE_BUILD with generated files
                usePRE_BUILD = false;
#endif
              }
            }
          }
        }
        // Process rcc enabled files
        if (rccEnabled && (ext == "qrc") &&
            !PropertyEnabled(sf, "SKIP_AUTORCC")) {
          const std::string absFile =
            cmsys::SystemTools::GetRealPath(sf->GetFullPath());

          // Compose rcc output file name
          {
            std::string rccOutBase = autogenBuildDir + "/";
            rccOutBase += fpathCheckSum.getPart(absFile);
            rccOutBase += "/qrc_";
            rccOutBase +=
              cmsys::SystemTools::GetFilenameWithoutLastExtension(absFile);

            // Register rcc ouput file as generated
            for (std::vector<std::string>::const_iterator it =
                   suffixes.begin();
                 it != suffixes.end(); ++it) {
              std::string rccOutCfg = rccOutBase;
              rccOutCfg += *it;
              rccOutCfg += ".cpp";
              AddGeneratedSource(makefile, rccOutCfg,
                                 cmQtAutoGeneratorCommon::RCC);
              autogenProvides.push_back(rccOutCfg);
            }
            // Add rcc output file to origin target sources
            if (multiConfig) {
              target->AddSource(rccOutBase + "_$<CONFIG>.cpp");
            } else {
              target->AddSource(rccOutBase + ".cpp");
            }
          }

          if (PropertyEnabled(sf, "GENERATED")) {
            // Add generated qrc file to the dependencies
            autogenDependFiles.push_back(absFile);
          } else {
            // Run cmake again when .qrc file changes
            makefile->AddCMakeDependFile(absFile);

            // Add the qrc input files to the dependencies
            std::string error;
            if (!cmQtAutoGeneratorCommon::RccListInputs(
                  qtMajorVersion, rccCommand, absFile, autogenDependFiles,
                  &error)) {
              cmSystemTools::Error(error.c_str());
            }
          }
#if defined(_WIN32) && !defined(__CYGWIN__)
          // Cannot use PRE_BUILD because the resource files themselves
          // may not be sources within the target so VS may not know the
          // target needs to re-build at all.
          usePRE_BUILD = false;
#endif
        }
      }
    }
  }

#if defined(_WIN32) && !defined(__CYGWIN__)
  if (usePRE_BUILD) {
    // We can't use pre-build if we depend on additional files
    if (!autogenDependFiles.empty()) {
      usePRE_BUILD = false;
    }
  }
  if (usePRE_BUILD) {
    // Add the pre-build command directly to bypass the OBJECT_LIBRARY
    // rejection in cmMakefile::AddCustomCommandToTarget because we know
    // PRE_BUILD will work for an OBJECT_LIBRARY in this specific case.
    std::vector<std::string> no_output;
    std::vector<std::string> no_depends;
    cmCustomCommand cc(makefile, no_output, autogenProvides, no_depends,
                       commandLines, autogenComment.c_str(),
                       workingDirectory.c_str());
    cc.SetEscapeOldStyle(false);
    cc.SetEscapeAllowMakeVars(true);
    target->Target->AddPreBuildCommand(cc);

    // Add additional target dependencies to the origin target
    for (std::vector<std::string>::const_iterator
           itC = autogenDependTargets.begin(),
           itE = autogenDependTargets.end();
         itC != itE; ++itC) {
      target->Target->AddUtility(*itC);
    }
  } else
#endif
  {
    cmTarget* autogenTarget = makefile->AddUtilityCommand(
      autogenTargetName, true, workingDirectory.c_str(),
      /*byproducts=*/autogenProvides, autogenDependFiles, commandLines, false,
      autogenComment.c_str());

    cmGeneratorTarget* gt = new cmGeneratorTarget(autogenTarget, lg);
    lg->AddGeneratorTarget(gt);

    // Add origin link library targets to the autogen target dependencies
    {
      const cmTarget::LinkLibraryVectorType& libVec =
        target->Target->GetOriginalLinkLibraries();
      for (cmTarget::LinkLibraryVectorType::const_iterator
             itC = libVec.begin(),
             itE = libVec.end();
           itC != itE; ++itC) {
        const std::string& libName = itC->first;
        if (makefile->FindTargetToUse(libName) != CM_NULLPTR) {
          autogenDependTargets.push_back(libName);
        }
      }
    }
    // Add origin utility targets to the autogen target dependencies
    {
      const std::set<std::string>& utils = target->Target->GetUtilities();
      for (std::set<std::string>::const_iterator itC = utils.begin(),
                                                 itE = utils.end();
           itC != itE; ++itC) {
        autogenDependTargets.push_back(*itC);
      }
    }
    // Add additional target dependencies to the autogen target
    for (std::vector<std::string>::const_iterator
           itC = autogenDependTargets.begin(),
           itE = autogenDependTargets.end();
         itC != itE; ++itC) {
      autogenTarget->AddUtility(*itC);
    }

    // Set target folder
    const char* autogenFolder =
      makefile->GetState()->GetGlobalProperty("AUTOMOC_TARGETS_FOLDER");
    if (!autogenFolder) {
      autogenFolder =
        makefile->GetState()->GetGlobalProperty("AUTOGEN_TARGETS_FOLDER");
    }
    if (autogenFolder && *autogenFolder) {
      autogenTarget->SetProperty("FOLDER", autogenFolder);
    } else {
      // inherit FOLDER property from target (#13688)
      utilCopyTargetProperty(gt->Target, target->Target, "FOLDER");
    }

    target->Target->AddUtility(autogenTargetName);
  }
}

void cmQtAutoGeneratorInitializer::SetupAutoGenerateTarget(
  cmGeneratorTarget const* target)
{
  cmMakefile* makefile = target->Target->GetMakefile();

  // forget the variables added here afterwards again:
  cmMakefile::ScopePushPop varScope(makefile);
  static_cast<void>(varScope);

  // Get configurations
  std::string config;
  const std::vector<std::string> configs(GetConfigurations(makefile, &config));

  // Configurations settings buffers
  std::map<std::string, std::string> configSuffix;
  std::map<std::string, std::string> configMocIncludes;
  std::map<std::string, std::string> configMocDefines;
  std::map<std::string, std::string> configUicOptions;

  // Configuration suffix
  if (IsMultiConfig(target->GetGlobalGenerator())) {
    for (std::vector<std::string>::const_iterator it = configs.begin();
         it != configs.end(); ++it) {
      configSuffix[*it] = "_" + *it;
    }
  }

  // Basic setup
  {
    const bool mocEnabled = target->GetPropertyAsBool("AUTOMOC");
    const bool uicEnabled = target->GetPropertyAsBool("AUTOUIC");
    const bool rccEnabled = target->GetPropertyAsBool("AUTORCC");
    const std::string autogenTargetName = GetAutogenTargetName(target);
    const std::string qtMajorVersion = GetQtMajorVersion(target);

    std::vector<std::string> sources;
    std::vector<std::string> headers;

    if (mocEnabled || uicEnabled || rccEnabled) {
      std::vector<std::string> mocSkipList;
      std::vector<std::string> uicSkipList;
      AcquireScanFiles(target, sources, headers, mocSkipList, uicSkipList);
      if (mocEnabled) {
        MocSetupAutoTarget(target, autogenTargetName, qtMajorVersion, config,
                           configs, mocSkipList, configMocIncludes,
                           configMocDefines);
      }
      if (uicEnabled) {
        UicSetupAutoTarget(target, qtMajorVersion, config, configs,
                           uicSkipList, configUicOptions);
      }
      if (rccEnabled) {
        RccSetupAutoTarget(target, qtMajorVersion);
      }
    }

    AddDefinitionEscaped(makefile, "_autogen_build_dir",
                         GetAutogenTargetBuildDir(target));
    AddDefinitionEscaped(makefile, "_qt_version_major", qtMajorVersion);
    AddDefinitionEscaped(makefile, "_sources", sources);
    AddDefinitionEscaped(makefile, "_headers", headers);
  }

  // Generate info file
  std::string infoFile = GetAutogenTargetFilesDir(target);
  infoFile += "/AutogenInfo.cmake";
  {
    std::string inf = cmSystemTools::GetCMakeRoot();
    inf += "/Modules/AutogenInfo.cmake.in";
    makefile->ConfigureFile(inf.c_str(), infoFile.c_str(), false, true, false);
  }

  // Append custom definitions to info file on demand
  if (!configSuffix.empty() || !configMocDefines.empty() ||
      !configMocIncludes.empty() || !configUicOptions.empty()) {

    // Ensure we have write permission in case .in was read-only.
    mode_t perm = 0;
#if defined(_WIN32) && !defined(__CYGWIN__)
    mode_t mode_write = S_IWRITE;
#else
    mode_t mode_write = S_IWUSR;
#endif
    cmSystemTools::GetPermissions(infoFile, perm);
    if (!(perm & mode_write)) {
      cmSystemTools::SetPermissions(infoFile, perm | mode_write);
    }

    // Open and write file
    cmsys::ofstream ofs(infoFile.c_str(), std::ios::app);
    if (ofs) {
      ofs << "# Configuration specific options\n";
      for (std::map<std::string, std::string>::iterator
             it = configSuffix.begin(),
             end = configSuffix.end();
           it != end; ++it) {
        ofs << "set(AM_CONFIG_SUFFIX_" << it->first << " " << it->second
            << ")\n";
      }
      for (std::map<std::string, std::string>::iterator
             it = configMocDefines.begin(),
             end = configMocDefines.end();
           it != end; ++it) {
        ofs << "set(AM_MOC_DEFINITIONS_" << it->first << " " << it->second
            << ")\n";
      }
      for (std::map<std::string, std::string>::iterator
             it = configMocIncludes.begin(),
             end = configMocIncludes.end();
           it != end; ++it) {
        ofs << "set(AM_MOC_INCLUDES_" << it->first << " " << it->second
            << ")\n";
      }
      for (std::map<std::string, std::string>::iterator
             it = configUicOptions.begin(),
             end = configUicOptions.end();
           it != end; ++it) {
        ofs << "set(AM_UIC_TARGET_OPTIONS_" << it->first << " " << it->second
            << ")\n";
      }
    } else {
      // File open error
      std::string error = "Internal CMake error when trying to open file: ";
      error += cmQtAutoGeneratorCommon::Quoted(infoFile);
      error += " for writing.";
      cmSystemTools::Error(error.c_str());
    }
  }
}
