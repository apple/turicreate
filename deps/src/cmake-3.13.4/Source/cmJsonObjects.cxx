/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmJsonObjects.h" // IWYU pragma: keep

#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmInstallGenerator.h"
#include "cmInstallTargetGenerator.h"
#include "cmJsonObjectDictionary.h"
#include "cmJsonObjects.h"
#include "cmLinkLineComputer.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmProperty.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateDirectory.h"
#include "cmStateSnapshot.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTest.h"
#include "cmake.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

std::vector<std::string> getConfigurations(const cmake* cm)
{
  std::vector<std::string> configurations;
  auto makefiles = cm->GetGlobalGenerator()->GetMakefiles();
  if (makefiles.empty()) {
    return configurations;
  }

  makefiles[0]->GetConfigurations(configurations);
  if (configurations.empty()) {
    configurations.push_back("");
  }
  return configurations;
}

bool hasString(const Json::Value& v, const std::string& s)
{
  return !v.isNull() &&
    std::any_of(v.begin(), v.end(),
                [s](const Json::Value& i) { return i.asString() == s; });
}

template <class T>
Json::Value fromStringList(const T& in)
{
  Json::Value result = Json::arrayValue;
  for (std::string const& i : in) {
    result.append(i);
  }
  return result;
}

} // namespace

void cmGetCMakeInputs(const cmGlobalGenerator* gg,
                      const std::string& sourceDir,
                      const std::string& buildDir,
                      std::vector<std::string>* internalFiles,
                      std::vector<std::string>* explicitFiles,
                      std::vector<std::string>* tmpFiles)
{
  const std::string cmakeRootDir = cmSystemTools::GetCMakeRoot() + '/';
  std::vector<cmMakefile*> const& makefiles = gg->GetMakefiles();
  for (cmMakefile const* mf : makefiles) {
    for (std::string const& lf : mf->GetListFiles()) {

      const std::string startOfFile = lf.substr(0, cmakeRootDir.size());
      const bool isInternal = (startOfFile == cmakeRootDir);
      const bool isTemporary = !isInternal && (lf.find(buildDir + '/') == 0);

      std::string toAdd = lf;
      if (!sourceDir.empty()) {
        const std::string& relative =
          cmSystemTools::RelativePath(sourceDir, lf);
        if (toAdd.size() > relative.size()) {
          toAdd = relative;
        }
      }

      if (isInternal) {
        if (internalFiles) {
          internalFiles->push_back(std::move(toAdd));
        }
      } else {
        if (isTemporary) {
          if (tmpFiles) {
            tmpFiles->push_back(std::move(toAdd));
          }
        } else {
          if (explicitFiles) {
            explicitFiles->push_back(std::move(toAdd));
          }
        }
      }
    }
  }
}

Json::Value cmDumpCMakeInputs(const cmake* cm)
{
  const cmGlobalGenerator* gg = cm->GetGlobalGenerator();
  const std::string& buildDir = cm->GetHomeOutputDirectory();
  const std::string& sourceDir = cm->GetHomeDirectory();

  std::vector<std::string> internalFiles;
  std::vector<std::string> explicitFiles;
  std::vector<std::string> tmpFiles;
  cmGetCMakeInputs(gg, sourceDir, buildDir, &internalFiles, &explicitFiles,
                   &tmpFiles);

  Json::Value array = Json::arrayValue;

  Json::Value tmp = Json::objectValue;
  tmp[kIS_CMAKE_KEY] = true;
  tmp[kIS_TEMPORARY_KEY] = false;
  tmp[kSOURCES_KEY] = fromStringList(internalFiles);
  array.append(tmp);

  tmp = Json::objectValue;
  tmp[kIS_CMAKE_KEY] = false;
  tmp[kIS_TEMPORARY_KEY] = false;
  tmp[kSOURCES_KEY] = fromStringList(explicitFiles);
  array.append(tmp);

  tmp = Json::objectValue;
  tmp[kIS_CMAKE_KEY] = false;
  tmp[kIS_TEMPORARY_KEY] = true;
  tmp[kSOURCES_KEY] = fromStringList(tmpFiles);
  array.append(tmp);

  return array;
}

class LanguageData
{
public:
  bool operator==(const LanguageData& other) const;

  void SetDefines(const std::set<std::string>& defines);

  bool IsGenerated = false;
  std::string Language;
  std::string Flags;
  std::vector<std::string> Defines;
  std::vector<std::pair<std::string, bool>> IncludePathList;
};

bool LanguageData::operator==(const LanguageData& other) const
{
  return Language == other.Language && Defines == other.Defines &&
    Flags == other.Flags && IncludePathList == other.IncludePathList &&
    IsGenerated == other.IsGenerated;
}

void LanguageData::SetDefines(const std::set<std::string>& defines)
{
  std::vector<std::string> result;
  result.reserve(defines.size());
  for (std::string const& i : defines) {
    result.push_back(i);
  }
  std::sort(result.begin(), result.end());
  Defines = std::move(result);
}

namespace std {

template <>
struct hash<LanguageData>
{
  std::size_t operator()(const LanguageData& in) const
  {
    using std::hash;
    size_t result =
      hash<std::string>()(in.Language) ^ hash<std::string>()(in.Flags);
    for (auto const& i : in.IncludePathList) {
      result = result ^
        (hash<std::string>()(i.first) ^
         (i.second ? std::numeric_limits<size_t>::max() : 0));
    }
    for (auto const& i : in.Defines) {
      result = result ^ hash<std::string>()(i);
    }
    result =
      result ^ (in.IsGenerated ? std::numeric_limits<size_t>::max() : 0);
    return result;
  }
};

} // namespace std

static Json::Value DumpSourceFileGroup(const LanguageData& data,
                                       const std::vector<std::string>& files,
                                       const std::string& baseDir)
{
  Json::Value result = Json::objectValue;

  if (!data.Language.empty()) {
    result[kLANGUAGE_KEY] = data.Language;
    if (!data.Flags.empty()) {
      result[kCOMPILE_FLAGS_KEY] = data.Flags;
    }
    if (!data.IncludePathList.empty()) {
      Json::Value includes = Json::arrayValue;
      for (auto const& i : data.IncludePathList) {
        Json::Value tmp = Json::objectValue;
        tmp[kPATH_KEY] = i.first;
        if (i.second) {
          tmp[kIS_SYSTEM_KEY] = i.second;
        }
        includes.append(tmp);
      }
      result[kINCLUDE_PATH_KEY] = includes;
    }
    if (!data.Defines.empty()) {
      result[kDEFINES_KEY] = fromStringList(data.Defines);
    }
  }

  result[kIS_GENERATED_KEY] = data.IsGenerated;

  Json::Value sourcesValue = Json::arrayValue;
  for (auto const& i : files) {
    const std::string relPath = cmSystemTools::RelativePath(baseDir, i);
    sourcesValue.append(relPath.size() < i.size() ? relPath : i);
  }

  result[kSOURCES_KEY] = sourcesValue;
  return result;
}

static Json::Value DumpSourceFilesList(
  cmGeneratorTarget* target, const std::string& config,
  const std::map<std::string, LanguageData>& languageDataMap)
{
  // Collect sourcefile groups:

  std::vector<cmSourceFile*> files;
  target->GetSourceFiles(files, config);

  std::unordered_map<LanguageData, std::vector<std::string>> fileGroups;
  for (cmSourceFile* file : files) {
    LanguageData fileData;
    fileData.Language = file->GetLanguage();
    if (!fileData.Language.empty()) {
      const LanguageData& ld = languageDataMap.at(fileData.Language);
      cmLocalGenerator* lg = target->GetLocalGenerator();
      cmGeneratorExpressionInterpreter genexInterpreter(lg, config, target,
                                                        fileData.Language);

      std::string compileFlags = ld.Flags;
      const std::string COMPILE_FLAGS("COMPILE_FLAGS");
      if (const char* cflags = file->GetProperty(COMPILE_FLAGS)) {
        lg->AppendFlags(compileFlags,
                        genexInterpreter.Evaluate(cflags, COMPILE_FLAGS));
      }
      const std::string COMPILE_OPTIONS("COMPILE_OPTIONS");
      if (const char* coptions = file->GetProperty(COMPILE_OPTIONS)) {
        lg->AppendCompileOptions(
          compileFlags, genexInterpreter.Evaluate(coptions, COMPILE_OPTIONS));
      }
      fileData.Flags = compileFlags;

      // Add include directories from source file properties.
      std::vector<std::string> includes;

      const std::string INCLUDE_DIRECTORIES("INCLUDE_DIRECTORIES");
      if (const char* cincludes = file->GetProperty(INCLUDE_DIRECTORIES)) {
        const std::string& evaluatedIncludes =
          genexInterpreter.Evaluate(cincludes, INCLUDE_DIRECTORIES);
        lg->AppendIncludeDirectories(includes, evaluatedIncludes, *file);

        for (const auto& include : includes) {
          fileData.IncludePathList.push_back(
            std::make_pair(include,
                           target->IsSystemIncludeDirectory(
                             include, config, fileData.Language)));
        }
      }

      fileData.IncludePathList.insert(fileData.IncludePathList.end(),
                                      ld.IncludePathList.begin(),
                                      ld.IncludePathList.end());

      const std::string COMPILE_DEFINITIONS("COMPILE_DEFINITIONS");
      std::set<std::string> defines;
      if (const char* defs = file->GetProperty(COMPILE_DEFINITIONS)) {
        lg->AppendDefines(
          defines, genexInterpreter.Evaluate(defs, COMPILE_DEFINITIONS));
      }

      const std::string defPropName =
        "COMPILE_DEFINITIONS_" + cmSystemTools::UpperCase(config);
      if (const char* config_defs = file->GetProperty(defPropName)) {
        lg->AppendDefines(
          defines,
          genexInterpreter.Evaluate(config_defs, COMPILE_DEFINITIONS));
      }

      defines.insert(ld.Defines.begin(), ld.Defines.end());

      fileData.SetDefines(defines);
    }

    fileData.IsGenerated = file->GetPropertyAsBool("GENERATED");
    std::vector<std::string>& groupFileList = fileGroups[fileData];
    groupFileList.push_back(file->GetFullPath());
  }

  const std::string& baseDir = target->Makefile->GetCurrentSourceDirectory();
  Json::Value result = Json::arrayValue;
  for (auto const& it : fileGroups) {
    Json::Value group = DumpSourceFileGroup(it.first, it.second, baseDir);
    if (!group.isNull()) {
      result.append(group);
    }
  }

  return result;
}

static Json::Value DumpCTestInfo(cmLocalGenerator* lg, cmTest* testInfo,
                                 const std::string& config)
{
  Json::Value result = Json::objectValue;
  result[kCTEST_NAME] = testInfo->GetName();

  // Concat command entries together. After the first should be the arguments
  // for the command
  std::string command;
  for (auto const& cmd : testInfo->GetCommand()) {
    command.append(cmd);
    command.append(" ");
  }

  // Remove any config specific variables from the output.
  cmGeneratorExpression ge;
  auto cge = ge.Parse(command);
  const std::string& processed = cge->Evaluate(lg, config);
  result[kCTEST_COMMAND] = processed;

  // Build up the list of properties that may have been specified
  Json::Value properties = Json::arrayValue;
  for (auto& prop : testInfo->GetProperties()) {
    Json::Value entry = Json::objectValue;
    entry[kKEY_KEY] = prop.first;

    // Remove config variables from the value too.
    auto cge_value = ge.Parse(prop.second.GetValue());
    const std::string& processed_value = cge_value->Evaluate(lg, config);
    entry[kVALUE_KEY] = processed_value;
    properties.append(entry);
  }
  result[kPROPERTIES_KEY] = properties;

  return result;
}

static void DumpMakefileTests(cmLocalGenerator* lg, const std::string& config,
                              Json::Value* result)
{
  auto mf = lg->GetMakefile();
  std::vector<cmTest*> tests;
  mf->GetTests(config, tests);
  for (auto test : tests) {
    Json::Value tmp = DumpCTestInfo(lg, test, config);
    if (!tmp.isNull()) {
      result->append(tmp);
    }
  }
}

static Json::Value DumpCTestProjectList(const cmake* cm,
                                        std::string const& config)
{
  Json::Value result = Json::arrayValue;

  auto globalGen = cm->GetGlobalGenerator();

  for (const auto& projectIt : globalGen->GetProjectMap()) {
    Json::Value pObj = Json::objectValue;
    pObj[kNAME_KEY] = projectIt.first;

    Json::Value tests = Json::arrayValue;

    // Gather tests for every generator
    for (const auto& lg : projectIt.second) {
      // Make sure they're generated.
      lg->GenerateTestFiles();
      DumpMakefileTests(lg, config, &tests);
    }

    pObj[kCTEST_INFO] = tests;

    result.append(pObj);
  }

  return result;
}

static Json::Value DumpCTestConfiguration(const cmake* cm,
                                          const std::string& config)
{
  Json::Value result = Json::objectValue;
  result[kNAME_KEY] = config;

  result[kPROJECTS_KEY] = DumpCTestProjectList(cm, config);

  return result;
}

static Json::Value DumpCTestConfigurationsList(const cmake* cm)
{
  Json::Value result = Json::arrayValue;

  for (const std::string& c : getConfigurations(cm)) {
    result.append(DumpCTestConfiguration(cm, c));
  }

  return result;
}

Json::Value cmDumpCTestInfo(const cmake* cm)
{
  Json::Value result = Json::objectValue;
  result[kCONFIGURATIONS_KEY] = DumpCTestConfigurationsList(cm);
  return result;
}

static Json::Value DumpTarget(cmGeneratorTarget* target,
                              const std::string& config)
{
  cmLocalGenerator* lg = target->GetLocalGenerator();
  const cmState* state = lg->GetState();

  const cmStateEnums::TargetType type = target->GetType();
  const std::string typeName = state->GetTargetTypeName(type);

  Json::Value ttl = Json::arrayValue;
  ttl.append("EXECUTABLE");
  ttl.append("STATIC_LIBRARY");
  ttl.append("SHARED_LIBRARY");
  ttl.append("MODULE_LIBRARY");
  ttl.append("OBJECT_LIBRARY");
  ttl.append("UTILITY");
  ttl.append("INTERFACE_LIBRARY");

  if (!hasString(ttl, typeName) || target->IsImported()) {
    return Json::Value();
  }

  Json::Value result = Json::objectValue;
  result[kNAME_KEY] = target->GetName();
  result[kIS_GENERATOR_PROVIDED_KEY] =
    target->Target->GetIsGeneratorProvided();
  result[kTYPE_KEY] = typeName;
  result[kSOURCE_DIRECTORY_KEY] = lg->GetCurrentSourceDirectory();
  result[kBUILD_DIRECTORY_KEY] = lg->GetCurrentBinaryDirectory();

  if (type == cmStateEnums::INTERFACE_LIBRARY) {
    return result;
  }

  result[kFULL_NAME_KEY] = target->GetFullName(config);

  if (target->Target->GetHaveInstallRule()) {
    result[kHAS_INSTALL_RULE] = true;

    Json::Value installPaths = Json::arrayValue;
    auto targetGenerators = target->Makefile->GetInstallGenerators();
    for (auto installGenerator : targetGenerators) {
      auto installTargetGenerator =
        dynamic_cast<cmInstallTargetGenerator*>(installGenerator);
      if (installTargetGenerator != nullptr &&
          installTargetGenerator->GetTarget()->Target == target->Target) {
        auto dest = installTargetGenerator->GetDestination(config);

        std::string installPath;
        if (!dest.empty() && cmSystemTools::FileIsFullPath(dest)) {
          installPath = dest;
        } else {
          std::string installPrefix =
            target->Makefile->GetSafeDefinition("CMAKE_INSTALL_PREFIX");
          installPath = installPrefix + '/' + dest;
        }

        installPaths.append(installPath);
      }
    }

    result[kINSTALL_PATHS] = installPaths;
  }

  if (target->HaveWellDefinedOutputFiles()) {
    Json::Value artifacts = Json::arrayValue;
    artifacts.append(
      target->GetFullPath(config, cmStateEnums::RuntimeBinaryArtifact));
    if (target->IsDLLPlatform()) {
      artifacts.append(
        target->GetFullPath(config, cmStateEnums::ImportLibraryArtifact));
      const cmGeneratorTarget::OutputInfo* output =
        target->GetOutputInfo(config);
      if (output && !output->PdbDir.empty()) {
        artifacts.append(output->PdbDir + '/' + target->GetPDBName(config));
      }
    }
    result[kARTIFACTS_KEY] = artifacts;

    result[kLINKER_LANGUAGE_KEY] = target->GetLinkerLanguage(config);

    std::string linkLibs;
    std::string linkFlags;
    std::string linkLanguageFlags;
    std::string frameworkPath;
    std::string linkPath;
    cmLinkLineComputer linkLineComputer(lg,
                                        lg->GetStateSnapshot().GetDirectory());
    lg->GetTargetFlags(&linkLineComputer, config, linkLibs, linkLanguageFlags,
                       linkFlags, frameworkPath, linkPath, target);

    linkLibs = cmSystemTools::TrimWhitespace(linkLibs);
    linkFlags = cmSystemTools::TrimWhitespace(linkFlags);
    linkLanguageFlags = cmSystemTools::TrimWhitespace(linkLanguageFlags);
    frameworkPath = cmSystemTools::TrimWhitespace(frameworkPath);
    linkPath = cmSystemTools::TrimWhitespace(linkPath);

    if (!cmSystemTools::TrimWhitespace(linkLibs).empty()) {
      result[kLINK_LIBRARIES_KEY] = linkLibs;
    }
    if (!cmSystemTools::TrimWhitespace(linkFlags).empty()) {
      result[kLINK_FLAGS_KEY] = linkFlags;
    }
    if (!cmSystemTools::TrimWhitespace(linkLanguageFlags).empty()) {
      result[kLINK_LANGUAGE_FLAGS_KEY] = linkLanguageFlags;
    }
    if (!frameworkPath.empty()) {
      result[kFRAMEWORK_PATH_KEY] = frameworkPath;
    }
    if (!linkPath.empty()) {
      result[kLINK_PATH_KEY] = linkPath;
    }
    const std::string sysroot =
      lg->GetMakefile()->GetSafeDefinition("CMAKE_SYSROOT");
    if (!sysroot.empty()) {
      result[kSYSROOT_KEY] = sysroot;
    }
  }

  std::set<std::string> languages;
  target->GetLanguages(languages, config);
  std::map<std::string, LanguageData> languageDataMap;

  for (std::string const& lang : languages) {
    LanguageData& ld = languageDataMap[lang];
    ld.Language = lang;
    lg->GetTargetCompileFlags(target, config, lang, ld.Flags);
    std::set<std::string> defines;
    lg->GetTargetDefines(target, config, lang, defines);
    ld.SetDefines(defines);
    std::vector<std::string> includePathList;
    lg->GetIncludeDirectories(includePathList, target, lang, config, true);
    for (std::string const& i : includePathList) {
      ld.IncludePathList.push_back(
        std::make_pair(i, target->IsSystemIncludeDirectory(i, config, lang)));
    }
  }

  Json::Value sourceGroupsValue =
    DumpSourceFilesList(target, config, languageDataMap);
  if (!sourceGroupsValue.empty()) {
    result[kFILE_GROUPS_KEY] = sourceGroupsValue;
  }

  return result;
}

static Json::Value DumpTargetsList(
  const std::vector<cmLocalGenerator*>& generators, const std::string& config)
{
  Json::Value result = Json::arrayValue;

  std::vector<cmGeneratorTarget*> targetList;
  for (auto const& lgIt : generators) {
    const auto& list = lgIt->GetGeneratorTargets();
    targetList.insert(targetList.end(), list.begin(), list.end());
  }
  std::sort(targetList.begin(), targetList.end());

  for (cmGeneratorTarget* target : targetList) {
    Json::Value tmp = DumpTarget(target, config);
    if (!tmp.isNull()) {
      result.append(tmp);
    }
  }

  return result;
}

static Json::Value DumpProjectList(const cmake* cm, std::string const& config)
{
  Json::Value result = Json::arrayValue;

  auto globalGen = cm->GetGlobalGenerator();

  for (auto const& projectIt : globalGen->GetProjectMap()) {
    Json::Value pObj = Json::objectValue;
    pObj[kNAME_KEY] = projectIt.first;

    // All Projects must have at least one local generator
    assert(!projectIt.second.empty());
    const cmLocalGenerator* lg = projectIt.second.at(0);

    // Project structure information:
    const cmMakefile* mf = lg->GetMakefile();
    auto minVersion = mf->GetDefinition("CMAKE_MINIMUM_REQUIRED_VERSION");
    pObj[kMINIMUM_CMAKE_VERSION] = minVersion ? minVersion : "";
    pObj[kSOURCE_DIRECTORY_KEY] = mf->GetCurrentSourceDirectory();
    pObj[kBUILD_DIRECTORY_KEY] = mf->GetCurrentBinaryDirectory();
    pObj[kTARGETS_KEY] = DumpTargetsList(projectIt.second, config);

    // For a project-level install rule it might be defined in any of its
    // associated generators.
    bool hasInstallRule = false;
    for (const auto generator : projectIt.second) {
      hasInstallRule =
        generator->GetMakefile()->GetInstallGenerators().empty() == false;

      if (hasInstallRule) {
        break;
      }
    }

    pObj[kHAS_INSTALL_RULE] = hasInstallRule;

    result.append(pObj);
  }

  return result;
}

static Json::Value DumpConfiguration(const cmake* cm,
                                     const std::string& config)
{
  Json::Value result = Json::objectValue;
  result[kNAME_KEY] = config;

  result[kPROJECTS_KEY] = DumpProjectList(cm, config);

  return result;
}

static Json::Value DumpConfigurationsList(const cmake* cm)
{
  Json::Value result = Json::arrayValue;

  for (std::string const& c : getConfigurations(cm)) {
    result.append(DumpConfiguration(cm, c));
  }

  return result;
}

Json::Value cmDumpCodeModel(const cmake* cm)
{
  Json::Value result = Json::objectValue;
  result[kCONFIGURATIONS_KEY] = DumpConfigurationsList(cm);
  return result;
}
