/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGeneratorTarget.h"

#include "cmsys/RegularExpression.hxx"
#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <iterator>
#include <queue>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmAlgorithms.h"
#include "cmComputeLinkInformation.h"
#include "cmCustomCommand.h"
#include "cmCustomCommandGenerator.h"
#include "cmCustomCommandLines.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorExpressionDAGChecker.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmPropertyMap.h"
#include "cmSourceFile.h"
#include "cmSourceFileLocation.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetLinkLibraryType.h"
#include "cmTargetPropertyComputer.h"
#include "cm_auto_ptr.hxx"
#include "cm_unordered_set.hxx"
#include "cmake.h"

class cmMessenger;

template <>
const char* cmTargetPropertyComputer::GetSources<cmGeneratorTarget>(
  cmGeneratorTarget const* tgt, cmMessenger* /* messenger */,
  cmListFileBacktrace const& /* context */)
{
  return tgt->GetSourcesProperty();
}

template <>
const char* cmTargetPropertyComputer::ComputeLocationForBuild<
  cmGeneratorTarget>(cmGeneratorTarget const* tgt)
{
  return tgt->GetLocation("");
}

template <>
const char* cmTargetPropertyComputer::ComputeLocation<cmGeneratorTarget>(
  cmGeneratorTarget const* tgt, const std::string& config)
{
  return tgt->GetLocation(config);
}

class cmGeneratorTarget::TargetPropertyEntry
{
  static cmLinkImplItem NoLinkImplItem;

public:
  TargetPropertyEntry(CM_AUTO_PTR<cmCompiledGeneratorExpression> cge,
                      cmLinkImplItem const& item = NoLinkImplItem)
    : ge(cge)
    , LinkImplItem(item)
  {
  }
  const CM_AUTO_PTR<cmCompiledGeneratorExpression> ge;
  cmLinkImplItem const& LinkImplItem;
};
cmLinkImplItem cmGeneratorTarget::TargetPropertyEntry::NoLinkImplItem;

void CreatePropertyGeneratorExpressions(
  cmStringRange entries, cmBacktraceRange backtraces,
  std::vector<cmGeneratorTarget::TargetPropertyEntry*>& items,
  bool evaluateForBuildsystem = false)
{
  std::vector<cmListFileBacktrace>::const_iterator btIt = backtraces.begin();
  for (std::vector<std::string>::const_iterator it = entries.begin();
       it != entries.end(); ++it, ++btIt) {
    cmGeneratorExpression ge(*btIt);
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(*it);
    cge->SetEvaluateForBuildsystem(evaluateForBuildsystem);
    items.push_back(new cmGeneratorTarget::TargetPropertyEntry(cge));
  }
}

cmGeneratorTarget::cmGeneratorTarget(cmTarget* t, cmLocalGenerator* lg)
  : Target(t)
  , FortranModuleDirectoryCreated(false)
  , SourceFileFlagsConstructed(false)
  , PolicyWarnedCMP0022(false)
  , PolicyReportedCMP0069(false)
  , DebugIncludesDone(false)
  , DebugCompileOptionsDone(false)
  , DebugCompileFeaturesDone(false)
  , DebugCompileDefinitionsDone(false)
  , DebugSourcesDone(false)
  , LinkImplementationLanguageIsContextDependent(true)
  , UtilityItemsDone(false)
{
  this->Makefile = this->Target->GetMakefile();
  this->LocalGenerator = lg;
  this->GlobalGenerator = this->LocalGenerator->GetGlobalGenerator();

  this->GlobalGenerator->ComputeTargetObjectDirectory(this);

  CreatePropertyGeneratorExpressions(t->GetIncludeDirectoriesEntries(),
                                     t->GetIncludeDirectoriesBacktraces(),
                                     this->IncludeDirectoriesEntries);

  CreatePropertyGeneratorExpressions(t->GetCompileOptionsEntries(),
                                     t->GetCompileOptionsBacktraces(),
                                     this->CompileOptionsEntries);

  CreatePropertyGeneratorExpressions(t->GetCompileFeaturesEntries(),
                                     t->GetCompileFeaturesBacktraces(),
                                     this->CompileFeaturesEntries);

  CreatePropertyGeneratorExpressions(t->GetCompileDefinitionsEntries(),
                                     t->GetCompileDefinitionsBacktraces(),
                                     this->CompileDefinitionsEntries);

  CreatePropertyGeneratorExpressions(t->GetSourceEntries(),
                                     t->GetSourceBacktraces(),
                                     this->SourceEntries, true);

  this->DLLPlatform =
    (this->Makefile->IsOn("WIN32") || this->Makefile->IsOn("CYGWIN") ||
     this->Makefile->IsOn("MINGW"));

  this->PolicyMap = t->PolicyMap;
}

cmGeneratorTarget::~cmGeneratorTarget()
{
  cmDeleteAll(this->IncludeDirectoriesEntries);
  cmDeleteAll(this->CompileOptionsEntries);
  cmDeleteAll(this->CompileFeaturesEntries);
  cmDeleteAll(this->CompileDefinitionsEntries);
  cmDeleteAll(this->SourceEntries);
  cmDeleteAll(this->LinkInformation);
}

const char* cmGeneratorTarget::GetSourcesProperty() const
{
  std::vector<std::string> values;
  for (std::vector<cmGeneratorTarget::TargetPropertyEntry *>::const_iterator
         it = this->SourceEntries.begin(),
         end = this->SourceEntries.end();
       it != end; ++it) {
    values.push_back((*it)->ge->GetInput());
  }
  static std::string value;
  value.clear();
  value = cmJoin(values, ";");
  return value.c_str();
}

cmGlobalGenerator* cmGeneratorTarget::GetGlobalGenerator() const
{
  return this->GetLocalGenerator()->GetGlobalGenerator();
}

cmLocalGenerator* cmGeneratorTarget::GetLocalGenerator() const
{
  return this->LocalGenerator;
}

cmStateEnums::TargetType cmGeneratorTarget::GetType() const
{
  return this->Target->GetType();
}

const std::string& cmGeneratorTarget::GetName() const
{
  return this->Target->GetName();
}

std::string cmGeneratorTarget::GetExportName() const
{
  const char* exportName = this->GetProperty("EXPORT_NAME");

  if (exportName && *exportName) {
    if (!cmGeneratorExpression::IsValidTargetName(exportName)) {
      std::ostringstream e;
      e << "EXPORT_NAME property \"" << exportName << "\" for \""
        << this->GetName() << "\": is not valid.";
      cmSystemTools::Error(e.str().c_str());
      return "";
    }
    return exportName;
  }
  return this->GetName();
}

const char* cmGeneratorTarget::GetProperty(const std::string& prop) const
{
  if (!cmTargetPropertyComputer::PassesWhitelist(
        this->GetType(), prop, this->Makefile->GetMessenger(),
        this->GetBacktrace())) {
    return CM_NULLPTR;
  }
  if (const char* result = cmTargetPropertyComputer::GetProperty(
        this, prop, this->Makefile->GetMessenger(), this->GetBacktrace())) {
    return result;
  }
  if (cmSystemTools::GetFatalErrorOccured()) {
    return CM_NULLPTR;
  }
  return this->Target->GetProperty(prop);
}

const char* cmGeneratorTarget::GetOutputTargetType(
  cmStateEnums::ArtifactType artifact) const
{
  switch (this->GetType()) {
    case cmStateEnums::SHARED_LIBRARY:
      if (this->IsDLLPlatform()) {
        switch (artifact) {
          case cmStateEnums::RuntimeBinaryArtifact:
            // A DLL shared library is treated as a runtime target.
            return "RUNTIME";
          case cmStateEnums::ImportLibraryArtifact:
            // A DLL import library is treated as an archive target.
            return "ARCHIVE";
        }
      } else {
        // For non-DLL platforms shared libraries are treated as
        // library targets.
        return "LIBRARY";
      }
      break;
    case cmStateEnums::STATIC_LIBRARY:
      // Static libraries are always treated as archive targets.
      return "ARCHIVE";
    case cmStateEnums::MODULE_LIBRARY:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          // Module import libraries are treated as archive targets.
          return "LIBRARY";
        case cmStateEnums::ImportLibraryArtifact:
          // Module libraries are always treated as library targets.
          return "ARCHIVE";
      }
      break;
    case cmStateEnums::EXECUTABLE:
      switch (artifact) {
        case cmStateEnums::RuntimeBinaryArtifact:
          // Executables are always treated as runtime targets.
          return "RUNTIME";
        case cmStateEnums::ImportLibraryArtifact:
          // Executable import libraries are treated as archive targets.
          return "ARCHIVE";
      }
      break;
    default:
      break;
  }
  return "";
}

std::string cmGeneratorTarget::GetOutputName(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  // Lookup/compute/cache the output name for this configuration.
  OutputNameKey key(config, artifact);
  cmGeneratorTarget::OutputNameMapType::iterator i =
    this->OutputNameMap.find(key);
  if (i == this->OutputNameMap.end()) {
    // Add empty name in map to detect potential recursion.
    OutputNameMapType::value_type entry(key, "");
    i = this->OutputNameMap.insert(entry).first;

    // Compute output name.
    std::vector<std::string> props;
    std::string type = this->GetOutputTargetType(artifact);
    std::string configUpper = cmSystemTools::UpperCase(config);
    if (!type.empty() && !configUpper.empty()) {
      // <ARCHIVE|LIBRARY|RUNTIME>_OUTPUT_NAME_<CONFIG>
      props.push_back(type + "_OUTPUT_NAME_" + configUpper);
    }
    if (!type.empty()) {
      // <ARCHIVE|LIBRARY|RUNTIME>_OUTPUT_NAME
      props.push_back(type + "_OUTPUT_NAME");
    }
    if (!configUpper.empty()) {
      // OUTPUT_NAME_<CONFIG>
      props.push_back("OUTPUT_NAME_" + configUpper);
      // <CONFIG>_OUTPUT_NAME
      props.push_back(configUpper + "_OUTPUT_NAME");
    }
    // OUTPUT_NAME
    props.push_back("OUTPUT_NAME");

    std::string outName;
    for (std::vector<std::string>::const_iterator it = props.begin();
         it != props.end(); ++it) {
      if (const char* outNameProp = this->GetProperty(*it)) {
        outName = outNameProp;
        break;
      }
    }

    if (outName.empty()) {
      outName = this->GetName();
    }

    // Now evaluate genex and update the previously-prepared map entry.
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(outName);
    i->second = cge->Evaluate(this->LocalGenerator, config);
  } else if (i->second.empty()) {
    // An empty map entry indicates we have been called recursively
    // from the above block.
    this->LocalGenerator->GetCMakeInstance()->IssueMessage(
      cmake::FATAL_ERROR,
      "Target '" + this->GetName() + "' OUTPUT_NAME depends on itself.",
      this->GetBacktrace());
  }
  return i->second;
}

void cmGeneratorTarget::AddSourceCommon(const std::string& src)
{
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
  cmGeneratorExpression ge(lfbt);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(src);
  cge->SetEvaluateForBuildsystem(true);
  this->SourceEntries.push_back(new TargetPropertyEntry(cge));
  this->KindedSourcesMap.clear();
  this->LinkImplementationLanguageIsContextDependent = true;
}

void cmGeneratorTarget::AddSource(const std::string& src)
{
  this->Target->AddSource(src);
  this->AddSourceCommon(src);
}

void cmGeneratorTarget::AddTracedSources(std::vector<std::string> const& srcs)
{
  this->Target->AddTracedSources(srcs);
  if (!srcs.empty()) {
    this->AddSourceCommon(cmJoin(srcs, ";"));
  }
}

void cmGeneratorTarget::AddIncludeDirectory(const std::string& src,
                                            bool before)
{
  this->Target->InsertInclude(src, this->Makefile->GetBacktrace(), before);
  cmListFileBacktrace lfbt = this->Makefile->GetBacktrace();
  cmGeneratorExpression ge(lfbt);
  CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(src);
  cge->SetEvaluateForBuildsystem(true);
  // Insert before begin/end
  std::vector<TargetPropertyEntry*>::iterator pos = before
    ? this->IncludeDirectoriesEntries.begin()
    : this->IncludeDirectoriesEntries.end();
  this->IncludeDirectoriesEntries.insert(pos, new TargetPropertyEntry(cge));
}

std::vector<cmSourceFile*> const* cmGeneratorTarget::GetSourceDepends(
  cmSourceFile const* sf) const
{
  SourceEntriesType::const_iterator i = this->SourceDepends.find(sf);
  if (i != this->SourceDepends.end()) {
    return &i->second.Depends;
  }
  return CM_NULLPTR;
}

static void handleSystemIncludesDep(
  cmLocalGenerator* lg, cmGeneratorTarget const* depTgt,
  const std::string& config, cmGeneratorTarget const* headTarget,
  cmGeneratorExpressionDAGChecker* dagChecker,
  std::vector<std::string>& result, bool excludeImported)
{
  if (const char* dirs =
        depTgt->GetProperty("INTERFACE_SYSTEM_INCLUDE_DIRECTORIES")) {
    cmGeneratorExpression ge;
    cmSystemTools::ExpandListArgument(
      ge.Parse(dirs)->Evaluate(lg, config, false, headTarget, depTgt,
                               dagChecker),
      result);
  }
  if (!depTgt->IsImported() || excludeImported) {
    return;
  }

  if (const char* dirs =
        depTgt->GetProperty("INTERFACE_INCLUDE_DIRECTORIES")) {
    cmGeneratorExpression ge;
    cmSystemTools::ExpandListArgument(
      ge.Parse(dirs)->Evaluate(lg, config, false, headTarget, depTgt,
                               dagChecker),
      result);
  }
}

/* clang-format off */
#define IMPLEMENT_VISIT(KIND)                                                 \
  {                                                                           \
    KindedSources const& kinded = this->GetKindedSources(config);             \
    for (std::vector<SourceAndKind>::const_iterator                           \
         si = kinded.Sources.begin(); si != kinded.Sources.end(); ++si) {     \
      if (si->Kind == KIND) {                                                 \
        data.push_back(si->Source);                                           \
      }                                                                       \
    }                                                                         \
  }
/* clang-format on */

void cmGeneratorTarget::GetObjectSources(
  std::vector<cmSourceFile const*>& data, const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindObjectSource);

  if (!this->Objects.empty()) {
    return;
  }

  for (std::vector<cmSourceFile const*>::const_iterator it = data.begin();
       it != data.end(); ++it) {
    this->Objects[*it];
  }

  this->LocalGenerator->ComputeObjectFilenames(this->Objects, this);
}

void cmGeneratorTarget::ComputeObjectMapping()
{
  if (!this->Objects.empty()) {
    return;
  }

  std::vector<std::string> configs;
  this->Makefile->GetConfigurations(configs);
  if (configs.empty()) {
    configs.push_back("");
  }
  for (std::vector<std::string>::const_iterator ci = configs.begin();
       ci != configs.end(); ++ci) {
    std::vector<cmSourceFile const*> sourceFiles;
    this->GetObjectSources(sourceFiles, *ci);
  }
}

const char* cmGeneratorTarget::GetFeature(const std::string& feature,
                                          const std::string& config) const
{
  if (!config.empty()) {
    std::string featureConfig = feature;
    featureConfig += "_";
    featureConfig += cmSystemTools::UpperCase(config);
    if (const char* value = this->GetProperty(featureConfig)) {
      return value;
    }
  }
  if (const char* value = this->GetProperty(feature)) {
    return value;
  }
  return this->LocalGenerator->GetFeature(feature, config);
}

bool cmGeneratorTarget::IsIPOEnabled(std::string const& lang,
                                     std::string const& config) const
{
  const char* feature = "INTERPROCEDURAL_OPTIMIZATION";
  const bool result = cmSystemTools::IsOn(this->GetFeature(feature, config));

  if (!result) {
    // 'INTERPROCEDURAL_OPTIMIZATION' is off, no need to check policies
    return false;
  }

  if (lang != "C" && lang != "CXX" && lang != "Fortran") {
    // We do not define IPO behavior for other languages.
    return false;
  }

  cmPolicies::PolicyStatus cmp0069 = this->GetPolicyStatusCMP0069();

  if (cmp0069 == cmPolicies::OLD || cmp0069 == cmPolicies::WARN) {
    if (this->Makefile->IsOn("_CMAKE_" + lang + "_IPO_LEGACY_BEHAVIOR")) {
      return true;
    }
    if (this->PolicyReportedCMP0069) {
      // problem is already reported, no need to issue a message
      return false;
    }
    const bool in_try_compile =
      this->LocalGenerator->GetCMakeInstance()->GetIsInTryCompile();
    if (cmp0069 == cmPolicies::WARN && !in_try_compile) {
      std::ostringstream w;
      w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0069) << "\n";
      w << "INTERPROCEDURAL_OPTIMIZATION property will be ignored for target "
        << "'" << this->GetName() << "'.";
      this->LocalGenerator->GetCMakeInstance()->IssueMessage(
        cmake::AUTHOR_WARNING, w.str(), this->GetBacktrace());

      this->PolicyReportedCMP0069 = true;
    }
    return false;
  }

  // Note: check consistency with messages from CheckIPOSupported
  const char* message = CM_NULLPTR;
  if (!this->Makefile->IsOn("_CMAKE_" + lang + "_IPO_SUPPORTED_BY_CMAKE")) {
    message = "CMake doesn't support IPO for current compiler";
  } else if (!this->Makefile->IsOn("_CMAKE_" + lang +
                                   "_IPO_MAY_BE_SUPPORTED_BY_COMPILER")) {
    message = "Compiler doesn't support IPO";
  } else if (!this->GlobalGenerator->IsIPOSupported()) {
    message = "CMake doesn't support IPO for current generator";
  }

  if (!message) {
    // No error/warning messages
    return true;
  }

  if (this->PolicyReportedCMP0069) {
    // problem is already reported, no need to issue a message
    return false;
  }

  this->PolicyReportedCMP0069 = true;

  this->LocalGenerator->GetCMakeInstance()->IssueMessage(
    cmake::FATAL_ERROR, message, this->GetBacktrace());
  return false;
}

const std::string& cmGeneratorTarget::GetObjectName(cmSourceFile const* file)
{
  this->ComputeObjectMapping();
  return this->Objects[file];
}

const char* cmGeneratorTarget::GetCustomObjectExtension() const
{
  static std::string extension;
  const bool has_ptx_extension =
    this->GetPropertyAsBool("CUDA_PTX_COMPILATION");
  if (has_ptx_extension) {
    extension = ".ptx";
    return extension.c_str();
  }
  return CM_NULLPTR;
}

void cmGeneratorTarget::AddExplicitObjectName(cmSourceFile const* sf)
{
  this->ExplicitObjectName.insert(sf);
}

bool cmGeneratorTarget::HasExplicitObjectName(cmSourceFile const* file) const
{
  const_cast<cmGeneratorTarget*>(this)->ComputeObjectMapping();
  std::set<cmSourceFile const*>::const_iterator it =
    this->ExplicitObjectName.find(file);
  return it != this->ExplicitObjectName.end();
}

void cmGeneratorTarget::GetModuleDefinitionSources(
  std::vector<cmSourceFile const*>& data, const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindModuleDefinition);
}

void cmGeneratorTarget::GetHeaderSources(
  std::vector<cmSourceFile const*>& data, const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindHeader);
}

void cmGeneratorTarget::GetExtraSources(std::vector<cmSourceFile const*>& data,
                                        const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindExtra);
}

void cmGeneratorTarget::GetCustomCommands(
  std::vector<cmSourceFile const*>& data, const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindCustomCommand);
}

void cmGeneratorTarget::GetExternalObjects(
  std::vector<cmSourceFile const*>& data, const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindExternalObject);
}

void cmGeneratorTarget::GetExpectedResxHeaders(std::set<std::string>& headers,
                                               const std::string& config) const
{
  KindedSources const& kinded = this->GetKindedSources(config);
  headers = kinded.ExpectedResxHeaders;
}

void cmGeneratorTarget::GetResxSources(std::vector<cmSourceFile const*>& data,
                                       const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindResx);
}

void cmGeneratorTarget::GetAppManifest(std::vector<cmSourceFile const*>& data,
                                       const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindAppManifest);
}

void cmGeneratorTarget::GetManifests(std::vector<cmSourceFile const*>& data,
                                     const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindManifest);
}

void cmGeneratorTarget::GetCertificates(std::vector<cmSourceFile const*>& data,
                                        const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindCertificate);
}

void cmGeneratorTarget::GetExpectedXamlHeaders(std::set<std::string>& headers,
                                               const std::string& config) const
{
  KindedSources const& kinded = this->GetKindedSources(config);
  headers = kinded.ExpectedXamlHeaders;
}

void cmGeneratorTarget::GetExpectedXamlSources(std::set<std::string>& srcs,
                                               const std::string& config) const
{
  KindedSources const& kinded = this->GetKindedSources(config);
  srcs = kinded.ExpectedXamlSources;
}

std::set<cmLinkItem> const& cmGeneratorTarget::GetUtilityItems() const
{
  if (!this->UtilityItemsDone) {
    this->UtilityItemsDone = true;
    std::set<std::string> const& utilities = this->GetUtilities();
    for (std::set<std::string>::const_iterator i = utilities.begin();
         i != utilities.end(); ++i) {
      cmGeneratorTarget* gt =
        this->LocalGenerator->FindGeneratorTargetToUse(*i);
      this->UtilityItems.insert(cmLinkItem(*i, gt));
    }
  }
  return this->UtilityItems;
}

void cmGeneratorTarget::GetXamlSources(std::vector<cmSourceFile const*>& data,
                                       const std::string& config) const
{
  IMPLEMENT_VISIT(SourceKindXaml);
}

const char* cmGeneratorTarget::GetLocation(const std::string& config) const
{
  static std::string location;
  if (this->IsImported()) {
    location = this->Target->ImportedGetFullPath(
      config, cmStateEnums::RuntimeBinaryArtifact);
  } else {
    location = this->GetFullPath(config, cmStateEnums::RuntimeBinaryArtifact);
  }
  return location.c_str();
}

std::vector<cmCustomCommand> const& cmGeneratorTarget::GetPreBuildCommands()
  const
{
  return this->Target->GetPreBuildCommands();
}

std::vector<cmCustomCommand> const& cmGeneratorTarget::GetPreLinkCommands()
  const
{
  return this->Target->GetPreLinkCommands();
}

std::vector<cmCustomCommand> const& cmGeneratorTarget::GetPostBuildCommands()
  const
{
  return this->Target->GetPostBuildCommands();
}

bool cmGeneratorTarget::IsImported() const
{
  return this->Target->IsImported();
}

bool cmGeneratorTarget::IsImportedGloballyVisible() const
{
  return this->Target->IsImportedGloballyVisible();
}

const char* cmGeneratorTarget::GetLocationForBuild() const
{
  static std::string location;
  if (this->IsImported()) {
    location = this->Target->ImportedGetFullPath(
      "", cmStateEnums::RuntimeBinaryArtifact);
    return location.c_str();
  }

  // Now handle the deprecated build-time configuration location.
  location = this->GetDirectory();
  const char* cfgid = this->Makefile->GetDefinition("CMAKE_CFG_INTDIR");
  if (cfgid && strcmp(cfgid, ".") != 0) {
    location += "/";
    location += cfgid;
  }

  if (this->IsAppBundleOnApple()) {
    std::string macdir = this->BuildBundleDirectory("", "", FullLevel);
    if (!macdir.empty()) {
      location += "/";
      location += macdir;
    }
  }
  location += "/";
  location += this->GetFullName("", cmStateEnums::RuntimeBinaryArtifact);
  return location.c_str();
}

bool cmGeneratorTarget::IsSystemIncludeDirectory(
  const std::string& dir, const std::string& config) const
{
  assert(this->GetType() != cmStateEnums::INTERFACE_LIBRARY);
  std::string config_upper;
  if (!config.empty()) {
    config_upper = cmSystemTools::UpperCase(config);
  }

  typedef std::map<std::string, std::vector<std::string> > IncludeCacheType;
  IncludeCacheType::const_iterator iter =
    this->SystemIncludesCache.find(config_upper);

  if (iter == this->SystemIncludesCache.end()) {
    cmGeneratorExpressionDAGChecker dagChecker(
      this->GetName(), "SYSTEM_INCLUDE_DIRECTORIES", CM_NULLPTR, CM_NULLPTR);

    bool excludeImported = this->GetPropertyAsBool("NO_SYSTEM_FROM_IMPORTED");

    std::vector<std::string> result;
    for (std::set<std::string>::const_iterator it =
           this->Target->GetSystemIncludeDirectories().begin();
         it != this->Target->GetSystemIncludeDirectories().end(); ++it) {
      cmGeneratorExpression ge;
      cmSystemTools::ExpandListArgument(
        ge.Parse(*it)->Evaluate(this->LocalGenerator, config, false, this,
                                &dagChecker),
        result);
    }

    std::vector<cmGeneratorTarget const*> const& deps =
      this->GetLinkImplementationClosure(config);
    for (std::vector<cmGeneratorTarget const *>::const_iterator
           li = deps.begin(),
           le = deps.end();
         li != le; ++li) {
      handleSystemIncludesDep(this->LocalGenerator, *li, config, this,
                              &dagChecker, result, excludeImported);
    }

    std::for_each(result.begin(), result.end(),
                  cmSystemTools::ConvertToUnixSlashes);
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    IncludeCacheType::value_type entry(config_upper, result);
    iter = this->SystemIncludesCache.insert(entry).first;
  }

  return std::binary_search(iter->second.begin(), iter->second.end(), dir);
}

bool cmGeneratorTarget::GetPropertyAsBool(const std::string& prop) const
{
  return this->Target->GetPropertyAsBool(prop);
}

static void AddInterfaceEntries(
  cmGeneratorTarget const* thisTarget, std::string const& config,
  std::string const& prop,
  std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries)
{
  if (cmLinkImplementationLibraries const* impl =
        thisTarget->GetLinkImplementationLibraries(config)) {
    for (std::vector<cmLinkImplItem>::const_iterator
           it = impl->Libraries.begin(),
           end = impl->Libraries.end();
         it != end; ++it) {
      if (it->Target) {
        std::string genex = "$<TARGET_PROPERTY:" + *it + "," + prop + ">";
        cmGeneratorExpression ge(it->Backtrace);
        CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(genex);
        cge->SetEvaluateForBuildsystem(true);
        entries.push_back(
          new cmGeneratorTarget::TargetPropertyEntry(cge, *it));
      }
    }
  }
}

static bool processSources(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& srcs, CM_UNORDERED_SET<std::string>& uniqueSrcs,
  cmGeneratorExpressionDAGChecker* dagChecker, std::string const& config,
  bool debugSources)
{
  cmMakefile* mf = tgt->Target->GetMakefile();

  bool contextDependent = false;

  for (std::vector<cmGeneratorTarget::TargetPropertyEntry *>::const_iterator
         it = entries.begin(),
         end = entries.end();
       it != end; ++it) {
    cmLinkImplItem const& item = (*it)->LinkImplItem;
    std::string const& targetName = item;
    std::vector<std::string> entrySources;
    cmSystemTools::ExpandListArgument(
      (*it)->ge->Evaluate(tgt->GetLocalGenerator(), config, false, tgt, tgt,
                          dagChecker),
      entrySources);

    if ((*it)->ge->GetHadContextSensitiveCondition()) {
      contextDependent = true;
    }

    for (std::vector<std::string>::iterator i = entrySources.begin();
         i != entrySources.end(); ++i) {
      std::string& src = *i;
      cmSourceFile* sf = mf->GetOrCreateSource(src);
      std::string e;
      std::string fullPath = sf->GetFullPath(&e);
      if (fullPath.empty()) {
        if (!e.empty()) {
          cmake* cm = tgt->GetLocalGenerator()->GetCMakeInstance();
          cm->IssueMessage(cmake::FATAL_ERROR, e, tgt->GetBacktrace());
        }
        return contextDependent;
      }

      if (!targetName.empty() && !cmSystemTools::FileIsFullPath(src.c_str())) {
        std::ostringstream err;
        if (!targetName.empty()) {
          err << "Target \"" << targetName
              << "\" contains relative "
                 "path in its INTERFACE_SOURCES:\n"
                 "  \""
              << src << "\"";
        } else {
          err << "Found relative path while evaluating sources of "
                 "\""
              << tgt->GetName() << "\":\n  \"" << src << "\"\n";
        }
        tgt->GetLocalGenerator()->IssueMessage(cmake::FATAL_ERROR, err.str());
        return contextDependent;
      }
      src = fullPath;
    }
    std::string usedSources;
    for (std::vector<std::string>::iterator li = entrySources.begin();
         li != entrySources.end(); ++li) {
      std::string src = *li;

      if (uniqueSrcs.insert(src).second) {
        srcs.push_back(src);
        if (debugSources) {
          usedSources += " * " + src + "\n";
        }
      }
    }
    if (!usedSources.empty()) {
      tgt->GetLocalGenerator()->GetCMakeInstance()->IssueMessage(
        cmake::LOG, std::string("Used sources for target ") + tgt->GetName() +
          ":\n" + usedSources,
        (*it)->ge->GetBacktrace());
    }
  }
  return contextDependent;
}

void cmGeneratorTarget::GetSourceFiles(std::vector<std::string>& files,
                                       const std::string& config) const
{
  assert(this->GetType() != cmStateEnums::INTERFACE_LIBRARY);

  if (!this->LocalGenerator->GetGlobalGenerator()->GetConfigureDoneCMP0026()) {
    // At configure-time, this method can be called as part of getting the
    // LOCATION property or to export() a file to be include()d.  However
    // there is no cmGeneratorTarget at configure-time, so search the SOURCES
    // for TARGET_OBJECTS instead for backwards compatibility with OLD
    // behavior of CMP0024 and CMP0026 only.

    cmStringRange sourceEntries = this->Target->GetSourceEntries();
    for (cmStringRange::const_iterator i = sourceEntries.begin();
         i != sourceEntries.end(); ++i) {
      std::string const& entry = *i;

      std::vector<std::string> items;
      cmSystemTools::ExpandListArgument(entry, items);
      for (std::vector<std::string>::const_iterator li = items.begin();
           li != items.end(); ++li) {
        if (cmHasLiteralPrefix(*li, "$<TARGET_OBJECTS:") &&
            (*li)[li->size() - 1] == '>') {
          continue;
        }
        files.push_back(*li);
      }
    }
    return;
  }

  std::vector<std::string> debugProperties;
  const char* debugProp =
    this->Makefile->GetDefinition("CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugSources = !this->DebugSourcesDone &&
    std::find(debugProperties.begin(), debugProperties.end(), "SOURCES") !=
      debugProperties.end();

  if (this->LocalGenerator->GetGlobalGenerator()->GetConfigureDoneCMP0026()) {
    this->DebugSourcesDone = true;
  }

  cmGeneratorExpressionDAGChecker dagChecker(this->GetName(), "SOURCES",
                                             CM_NULLPTR, CM_NULLPTR);

  CM_UNORDERED_SET<std::string> uniqueSrcs;
  bool contextDependentDirectSources =
    processSources(this, this->SourceEntries, files, uniqueSrcs, &dagChecker,
                   config, debugSources);

  std::vector<cmGeneratorTarget::TargetPropertyEntry*>
    linkInterfaceSourcesEntries;

  AddInterfaceEntries(this, config, "INTERFACE_SOURCES",
                      linkInterfaceSourcesEntries);

  std::vector<std::string>::size_type numFilesBefore = files.size();
  bool contextDependentInterfaceSources =
    processSources(this, linkInterfaceSourcesEntries, files, uniqueSrcs,
                   &dagChecker, config, debugSources);

  if (!contextDependentDirectSources &&
      !(contextDependentInterfaceSources && numFilesBefore < files.size())) {
    this->LinkImplementationLanguageIsContextDependent = false;
  }

  cmDeleteAll(linkInterfaceSourcesEntries);
}

void cmGeneratorTarget::GetSourceFiles(std::vector<cmSourceFile*>& files,
                                       const std::string& config) const
{
  if (!this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    // Since we are still configuring not all sources may exist yet,
    // so we need to avoid full source classification because that
    // requires the absolute paths to all sources to be determined.
    // Since this is only for compatibility with old policies that
    // projects should not depend on anymore, just compute the files
    // without memoizing them.
    std::vector<std::string> srcs;
    this->GetSourceFiles(srcs, config);
    std::set<cmSourceFile*> emitted;
    for (std::vector<std::string>::const_iterator i = srcs.begin();
         i != srcs.end(); ++i) {
      cmSourceFile* sf = this->Makefile->GetOrCreateSource(*i);
      if (emitted.insert(sf).second) {
        files.push_back(sf);
      }
    }
    return;
  }

  KindedSources const& kinded = this->GetKindedSources(config);
  files.reserve(kinded.Sources.size());
  for (std::vector<SourceAndKind>::const_iterator si = kinded.Sources.begin();
       si != kinded.Sources.end(); ++si) {
    files.push_back(si->Source);
  }
}

void cmGeneratorTarget::GetSourceFilesWithoutObjectLibraries(
  std::vector<cmSourceFile*>& files, const std::string& config) const
{
  KindedSources const& kinded = this->GetKindedSources(config);
  files.reserve(kinded.Sources.size());
  for (std::vector<SourceAndKind>::const_iterator si = kinded.Sources.begin();
       si != kinded.Sources.end(); ++si) {
    if (si->Source->GetObjectLibrary().empty()) {
      files.push_back(si->Source);
    }
  }
}

cmGeneratorTarget::KindedSources const& cmGeneratorTarget::GetKindedSources(
  std::string const& config) const
{
  // If we already processed one configuration and found no dependenc
  // on configuration then always use the one result.
  if (!this->LinkImplementationLanguageIsContextDependent) {
    return this->KindedSourcesMap.begin()->second;
  }

  // Lookup any existing link implementation for this configuration.
  std::string const key = cmSystemTools::UpperCase(config);
  KindedSourcesMapType::iterator it = this->KindedSourcesMap.find(key);
  if (it != this->KindedSourcesMap.end()) {
    if (!it->second.Initialized) {
      std::ostringstream e;
      e << "The SOURCES of \"" << this->GetName()
        << "\" use a generator expression that depends on the "
           "SOURCES themselves.";
      this->GlobalGenerator->GetCMakeInstance()->IssueMessage(
        cmake::FATAL_ERROR, e.str(), this->GetBacktrace());
      static KindedSources empty;
      return empty;
    }
    return it->second;
  }

  // Add an entry to the map for this configuration.
  KindedSources& files = this->KindedSourcesMap[key];
  this->ComputeKindedSources(files, config);
  files.Initialized = true;
  return files;
}

void cmGeneratorTarget::ComputeKindedSources(KindedSources& files,
                                             std::string const& config) const
{
  // Get the source file paths by string.
  std::vector<std::string> srcs;
  this->GetSourceFiles(srcs, config);

  cmsys::RegularExpression header_regex(CM_HEADER_REGEX);
  std::vector<cmSourceFile*> badObjLib;

  std::set<cmSourceFile*> emitted;
  for (std::vector<std::string>::const_iterator i = srcs.begin();
       i != srcs.end(); ++i) {
    // Create each source at most once.
    cmSourceFile* sf = this->Makefile->GetOrCreateSource(*i);
    if (!emitted.insert(sf).second) {
      continue;
    }

    // Compute the kind (classification) of this source file.
    SourceKind kind;
    std::string ext = cmSystemTools::LowerCase(sf->GetExtension());
    if (sf->GetCustomCommand()) {
      kind = SourceKindCustomCommand;
    } else if (this->Target->GetType() == cmStateEnums::UTILITY) {
      kind = SourceKindExtra;
    } else if (sf->GetPropertyAsBool("HEADER_FILE_ONLY")) {
      kind = SourceKindHeader;
    } else if (sf->GetPropertyAsBool("EXTERNAL_OBJECT")) {
      kind = SourceKindExternalObject;
      if (this->GetType() == cmStateEnums::OBJECT_LIBRARY) {
        badObjLib.push_back(sf);
      }
    } else if (!sf->GetLanguage().empty()) {
      kind = SourceKindObjectSource;
    } else if (ext == "def") {
      kind = SourceKindModuleDefinition;
      if (this->GetType() == cmStateEnums::OBJECT_LIBRARY) {
        badObjLib.push_back(sf);
      }
    } else if (ext == "idl") {
      kind = SourceKindIDL;
      if (this->GetType() == cmStateEnums::OBJECT_LIBRARY) {
        badObjLib.push_back(sf);
      }
    } else if (ext == "resx") {
      kind = SourceKindResx;
      // Build and save the name of the corresponding .h file
      // This relationship will be used later when building the project files.
      // Both names would have been auto generated from Visual Studio
      // where the user supplied the file name and Visual Studio
      // appended the suffix.
      std::string resx = sf->GetFullPath();
      std::string hFileName = resx.substr(0, resx.find_last_of('.')) + ".h";
      files.ExpectedResxHeaders.insert(hFileName);
    } else if (ext == "appxmanifest") {
      kind = SourceKindAppManifest;
    } else if (ext == "manifest") {
      kind = SourceKindManifest;
    } else if (ext == "pfx") {
      kind = SourceKindCertificate;
    } else if (ext == "xaml") {
      kind = SourceKindXaml;
      // Build and save the name of the corresponding .h and .cpp file
      // This relationship will be used later when building the project files.
      // Both names would have been auto generated from Visual Studio
      // where the user supplied the file name and Visual Studio
      // appended the suffix.
      std::string xaml = sf->GetFullPath();
      std::string hFileName = xaml + ".h";
      std::string cppFileName = xaml + ".cpp";
      files.ExpectedXamlHeaders.insert(hFileName);
      files.ExpectedXamlSources.insert(cppFileName);
    } else if (header_regex.find(sf->GetFullPath().c_str())) {
      kind = SourceKindHeader;
    } else {
      kind = SourceKindExtra;
    }

    // Save this classified source file in the result vector.
    SourceAndKind entry = { sf, kind };
    files.Sources.push_back(entry);
  }

  if (!badObjLib.empty()) {
    std::ostringstream e;
    e << "OBJECT library \"" << this->GetName() << "\" contains:\n";
    for (std::vector<cmSourceFile*>::const_iterator i = badObjLib.begin();
         i != badObjLib.end(); ++i) {
      e << "  " << (*i)->GetLocation().GetName() << "\n";
    }
    e << "but may contain only sources that compile, header files, and "
         "other files that would not affect linking of a normal library.";
    this->GlobalGenerator->GetCMakeInstance()->IssueMessage(
      cmake::FATAL_ERROR, e.str(), this->GetBacktrace());
  }
}

std::vector<cmGeneratorTarget::AllConfigSource> const&
cmGeneratorTarget::GetAllConfigSources() const
{
  if (this->AllConfigSources.empty()) {
    this->ComputeAllConfigSources();
  }
  return this->AllConfigSources;
}

void cmGeneratorTarget::ComputeAllConfigSources() const
{
  std::vector<std::string> configs;
  this->Makefile->GetConfigurations(configs);

  std::map<cmSourceFile const*, size_t> index;

  for (size_t ci = 0; ci < configs.size(); ++ci) {
    KindedSources const& sources = this->GetKindedSources(configs[ci]);
    for (std::vector<cmGeneratorTarget::SourceAndKind>::const_iterator si =
           sources.Sources.begin();
         si != sources.Sources.end(); ++si) {
      std::map<cmSourceFile const*, size_t>::iterator mi =
        index.find(si->Source);
      if (mi == index.end()) {
        AllConfigSource acs;
        acs.Source = si->Source;
        acs.Kind = si->Kind;
        this->AllConfigSources.push_back(acs);
        std::map<cmSourceFile const*, size_t>::value_type entry(
          si->Source, this->AllConfigSources.size() - 1);
        mi = index.insert(entry).first;
      }
      this->AllConfigSources[mi->second].Configs.push_back(ci);
    }
  }
}

std::string cmGeneratorTarget::GetCompilePDBName(
  const std::string& config) const
{
  std::string prefix;
  std::string base;
  std::string suffix;
  this->GetFullNameInternal(config, cmStateEnums::RuntimeBinaryArtifact,
                            prefix, base, suffix);

  // Check for a per-configuration output directory target property.
  std::string configUpper = cmSystemTools::UpperCase(config);
  std::string configProp = "COMPILE_PDB_NAME_";
  configProp += configUpper;
  const char* config_name = this->GetProperty(configProp);
  if (config_name && *config_name) {
    return prefix + config_name + ".pdb";
  }

  const char* name = this->GetProperty("COMPILE_PDB_NAME");
  if (name && *name) {
    return prefix + name + ".pdb";
  }

  return "";
}

std::string cmGeneratorTarget::GetCompilePDBPath(
  const std::string& config) const
{
  std::string dir = this->GetCompilePDBDirectory(config);
  std::string name = this->GetCompilePDBName(config);
  if (dir.empty() && !name.empty() && this->HaveWellDefinedOutputFiles()) {
    dir = this->GetPDBDirectory(config);
  }
  if (!dir.empty()) {
    dir += "/";
  }
  return dir + name;
}

bool cmGeneratorTarget::HasSOName(const std::string& config) const
{
  // soname is supported only for shared libraries and modules,
  // and then only when the platform supports an soname flag.
  return ((this->GetType() == cmStateEnums::SHARED_LIBRARY) &&
          !this->GetPropertyAsBool("NO_SONAME") &&
          this->Makefile->GetSONameFlag(this->GetLinkerLanguage(config)));
}

bool cmGeneratorTarget::NeedRelinkBeforeInstall(
  const std::string& config) const
{
  // Only executables and shared libraries can have an rpath and may
  // need relinking.
  if (this->GetType() != cmStateEnums::EXECUTABLE &&
      this->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GetType() != cmStateEnums::MODULE_LIBRARY) {
    return false;
  }

  // If there is no install location this target will not be installed
  // and therefore does not need relinking.
  if (!this->Target->GetHaveInstallRule()) {
    return false;
  }

  // If skipping all rpaths completely then no relinking is needed.
  if (this->Makefile->IsOn("CMAKE_SKIP_RPATH")) {
    return false;
  }

  // If building with the install-tree rpath no relinking is needed.
  if (this->GetPropertyAsBool("BUILD_WITH_INSTALL_RPATH")) {
    return false;
  }

  // If chrpath is going to be used no relinking is needed.
  if (this->IsChrpathUsed(config)) {
    return false;
  }

  // Check for rpath support on this platform.
  std::string ll = this->GetLinkerLanguage(config);
  if (!ll.empty()) {
    std::string flagVar = "CMAKE_SHARED_LIBRARY_RUNTIME_";
    flagVar += ll;
    flagVar += "_FLAG";
    if (!this->Makefile->IsSet(flagVar)) {
      // There is no rpath support on this platform so nothing needs
      // relinking.
      return false;
    }
  } else {
    // No linker language is known.  This error will be reported by
    // other code.
    return false;
  }

  // If either a build or install tree rpath is set then the rpath
  // will likely change between the build tree and install tree and
  // this target must be relinked.
  bool have_rpath =
    this->HaveBuildTreeRPATH(config) || this->HaveInstallTreeRPATH();
  bool is_ninja =
    this->LocalGenerator->GetGlobalGenerator()->GetName() == "Ninja";

  if (have_rpath && is_ninja) {
    std::ostringstream w;
    /* clang-format off */
    w <<
      "The install of the " << this->GetName() << " target requires "
      "changing an RPATH from the build tree, but this is not supported "
      "with the Ninja generator unless on an ELF-based platform.  The "
      "CMAKE_BUILD_WITH_INSTALL_RPATH variable may be set to avoid this "
      "relinking step."
      ;
    /* clang-format on */

    cmake* cm = this->LocalGenerator->GetCMakeInstance();
    cm->IssueMessage(cmake::FATAL_ERROR, w.str(), this->GetBacktrace());
  }

  return have_rpath;
}

bool cmGeneratorTarget::IsChrpathUsed(const std::string& config) const
{
  // Only certain target types have an rpath.
  if (!(this->GetType() == cmStateEnums::SHARED_LIBRARY ||
        this->GetType() == cmStateEnums::MODULE_LIBRARY ||
        this->GetType() == cmStateEnums::EXECUTABLE)) {
    return false;
  }

  // If the target will not be installed we do not need to change its
  // rpath.
  if (!this->Target->GetHaveInstallRule()) {
    return false;
  }

  // Skip chrpath if skipping rpath altogether.
  if (this->Makefile->IsOn("CMAKE_SKIP_RPATH")) {
    return false;
  }

  // Skip chrpath if it does not need to be changed at install time.
  if (this->GetPropertyAsBool("BUILD_WITH_INSTALL_RPATH")) {
    return false;
  }

  // Allow the user to disable builtin chrpath explicitly.
  if (this->Makefile->IsOn("CMAKE_NO_BUILTIN_CHRPATH")) {
    return false;
  }

  if (this->Makefile->IsOn("CMAKE_PLATFORM_HAS_INSTALLNAME")) {
    return true;
  }

#if defined(CMAKE_USE_ELF_PARSER)
  // Enable if the rpath flag uses a separator and the target uses ELF
  // binaries.
  std::string ll = this->GetLinkerLanguage(config);
  if (!ll.empty()) {
    std::string sepVar = "CMAKE_SHARED_LIBRARY_RUNTIME_";
    sepVar += ll;
    sepVar += "_FLAG_SEP";
    const char* sep = this->Makefile->GetDefinition(sepVar);
    if (sep && *sep) {
      // TODO: Add ELF check to ABI detection and get rid of
      // CMAKE_EXECUTABLE_FORMAT.
      if (const char* fmt =
            this->Makefile->GetDefinition("CMAKE_EXECUTABLE_FORMAT")) {
        return strcmp(fmt, "ELF") == 0;
      }
    }
  }
#endif
  static_cast<void>(config);
  return false;
}

bool cmGeneratorTarget::IsImportedSharedLibWithoutSOName(
  const std::string& config) const
{
  if (this->IsImported() && this->GetType() == cmStateEnums::SHARED_LIBRARY) {
    if (cmGeneratorTarget::ImportInfo const* info =
          this->GetImportInfo(config)) {
      return info->NoSOName;
    }
  }
  return false;
}

bool cmGeneratorTarget::HasMacOSXRpathInstallNameDir(
  const std::string& config) const
{
  bool install_name_is_rpath = false;
  bool macosx_rpath = false;

  if (!this->IsImported()) {
    if (this->GetType() != cmStateEnums::SHARED_LIBRARY) {
      return false;
    }
    const char* install_name = this->GetProperty("INSTALL_NAME_DIR");
    bool use_install_name = this->MacOSXUseInstallNameDir();
    if (install_name && use_install_name &&
        std::string(install_name) == "@rpath") {
      install_name_is_rpath = true;
    } else if (install_name && use_install_name) {
      return false;
    }
    if (!install_name_is_rpath) {
      macosx_rpath = this->MacOSXRpathInstallNameDirDefault();
    }
  } else {
    // Lookup the imported soname.
    if (cmGeneratorTarget::ImportInfo const* info =
          this->GetImportInfo(config)) {
      if (!info->NoSOName && !info->SOName.empty()) {
        if (info->SOName.find("@rpath/") == 0) {
          install_name_is_rpath = true;
        }
      } else {
        std::string install_name;
        cmSystemTools::GuessLibraryInstallName(info->Location, install_name);
        if (install_name.find("@rpath") != std::string::npos) {
          install_name_is_rpath = true;
        }
      }
    }
  }

  if (!install_name_is_rpath && !macosx_rpath) {
    return false;
  }

  if (!this->Makefile->IsSet("CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG")) {
    std::ostringstream w;
    w << "Attempting to use ";
    if (macosx_rpath) {
      w << "MACOSX_RPATH";
    } else {
      w << "@rpath";
    }
    w << " without CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG being set.";
    w << "  This could be because you are using a Mac OS X version";
    w << " less than 10.5 or because CMake's platform configuration is";
    w << " corrupt.";
    cmake* cm = this->LocalGenerator->GetCMakeInstance();
    cm->IssueMessage(cmake::FATAL_ERROR, w.str(), this->GetBacktrace());
  }

  return true;
}

bool cmGeneratorTarget::MacOSXRpathInstallNameDirDefault() const
{
  // we can't do rpaths when unsupported
  if (!this->Makefile->IsSet("CMAKE_SHARED_LIBRARY_RUNTIME_C_FLAG")) {
    return false;
  }

  const char* macosx_rpath_str = this->GetProperty("MACOSX_RPATH");
  if (macosx_rpath_str) {
    return this->GetPropertyAsBool("MACOSX_RPATH");
  }

  cmPolicies::PolicyStatus cmp0042 = this->GetPolicyStatusCMP0042();

  if (cmp0042 == cmPolicies::WARN) {
    this->LocalGenerator->GetGlobalGenerator()->AddCMP0042WarnTarget(
      this->GetName());
  }

  return cmp0042 == cmPolicies::NEW;
}

bool cmGeneratorTarget::MacOSXUseInstallNameDir() const
{
  const char* build_with_install_name =
    this->GetProperty("BUILD_WITH_INSTALL_NAME_DIR");
  if (build_with_install_name) {
    return cmSystemTools::IsOn(build_with_install_name);
  }

  cmPolicies::PolicyStatus cmp0068 = this->GetPolicyStatusCMP0068();
  if (cmp0068 == cmPolicies::NEW) {
    return false;
  }

  bool use_install_name = this->GetPropertyAsBool("BUILD_WITH_INSTALL_RPATH");

  if (use_install_name && cmp0068 == cmPolicies::WARN) {
    this->LocalGenerator->GetGlobalGenerator()->AddCMP0068WarnTarget(
      this->GetName());
  }

  return use_install_name;
}

bool cmGeneratorTarget::CanGenerateInstallNameDir(
  InstallNameType name_type) const
{
  cmPolicies::PolicyStatus cmp0068 = this->GetPolicyStatusCMP0068();

  if (cmp0068 == cmPolicies::NEW) {
    return true;
  }

  bool skip = this->Makefile->IsOn("CMAKE_SKIP_RPATH");
  if (name_type == INSTALL_NAME_FOR_INSTALL) {
    skip |= this->Makefile->IsOn("CMAKE_SKIP_INSTALL_RPATH");
  } else {
    skip |= this->GetPropertyAsBool("SKIP_BUILD_RPATH");
  }

  if (skip && cmp0068 == cmPolicies::WARN) {
    this->LocalGenerator->GetGlobalGenerator()->AddCMP0068WarnTarget(
      this->GetName());
  }

  return !skip;
}

std::string cmGeneratorTarget::GetSOName(const std::string& config) const
{
  if (this->IsImported()) {
    // Lookup the imported soname.
    if (cmGeneratorTarget::ImportInfo const* info =
          this->GetImportInfo(config)) {
      if (info->NoSOName) {
        // The imported library has no builtin soname so the name
        // searched at runtime will be just the filename.
        return cmSystemTools::GetFilenameName(info->Location);
      }
      // Use the soname given if any.
      if (info->SOName.find("@rpath/") == 0) {
        return info->SOName.substr(6);
      }
      return info->SOName;
    }
    return "";
  }
  // Compute the soname that will be built.
  std::string name;
  std::string soName;
  std::string realName;
  std::string impName;
  std::string pdbName;
  this->GetLibraryNames(name, soName, realName, impName, pdbName, config);
  return soName;
}

static bool shouldAddFullLevel(cmGeneratorTarget::BundleDirectoryLevel level)
{
  return level == cmGeneratorTarget::FullLevel;
}

static bool shouldAddContentLevel(
  cmGeneratorTarget::BundleDirectoryLevel level)
{
  return level == cmGeneratorTarget::ContentLevel || shouldAddFullLevel(level);
}

std::string cmGeneratorTarget::GetAppBundleDirectory(
  const std::string& config, BundleDirectoryLevel level) const
{
  std::string fpath =
    this->GetFullName(config, cmStateEnums::RuntimeBinaryArtifact);
  fpath += ".";
  const char* ext = this->GetProperty("BUNDLE_EXTENSION");
  if (!ext) {
    ext = "app";
  }
  fpath += ext;
  if (shouldAddContentLevel(level) && !this->Makefile->PlatformIsAppleIos()) {
    fpath += "/Contents";
    if (shouldAddFullLevel(level)) {
      fpath += "/MacOS";
    }
  }
  return fpath;
}

bool cmGeneratorTarget::IsBundleOnApple() const
{
  return this->IsFrameworkOnApple() || this->IsAppBundleOnApple() ||
    this->IsCFBundleOnApple();
}

std::string cmGeneratorTarget::GetCFBundleDirectory(
  const std::string& config, BundleDirectoryLevel level) const
{
  std::string fpath;
  fpath += this->GetOutputName(config, cmStateEnums::RuntimeBinaryArtifact);
  fpath += ".";
  const char* ext = this->GetProperty("BUNDLE_EXTENSION");
  if (!ext) {
    if (this->IsXCTestOnApple()) {
      ext = "xctest";
    } else {
      ext = "bundle";
    }
  }
  fpath += ext;
  if (shouldAddContentLevel(level) && !this->Makefile->PlatformIsAppleIos()) {
    fpath += "/Contents";
    if (shouldAddFullLevel(level)) {
      fpath += "/MacOS";
    }
  }
  return fpath;
}

std::string cmGeneratorTarget::GetFrameworkDirectory(
  const std::string& config, BundleDirectoryLevel level) const
{
  std::string fpath;
  fpath += this->GetOutputName(config, cmStateEnums::RuntimeBinaryArtifact);
  fpath += ".";
  const char* ext = this->GetProperty("BUNDLE_EXTENSION");
  if (!ext) {
    ext = "framework";
  }
  fpath += ext;
  if (shouldAddFullLevel(level) && !this->Makefile->PlatformIsAppleIos()) {
    fpath += "/Versions/";
    fpath += this->GetFrameworkVersion();
  }
  return fpath;
}

std::string cmGeneratorTarget::GetFullName(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  if (this->IsImported()) {
    return this->GetFullNameImported(config, artifact);
  }
  return this->GetFullNameInternal(config, artifact);
}

std::string cmGeneratorTarget::GetInstallNameDirForBuildTree(
  const std::string& config) const
{
  if (this->Makefile->IsOn("CMAKE_PLATFORM_HAS_INSTALLNAME")) {

    // If building directly for installation then the build tree install_name
    // is the same as the install tree.
    if (this->MacOSXUseInstallNameDir()) {
      return this->GetInstallNameDirForInstallTree();
    }

    // Use the build tree directory for the target.
    if (this->CanGenerateInstallNameDir(INSTALL_NAME_FOR_BUILD)) {
      std::string dir;
      if (this->MacOSXRpathInstallNameDirDefault()) {
        dir = "@rpath";
      } else {
        dir = this->GetDirectory(config);
      }
      dir += "/";
      return dir;
    }
  }
  return "";
}

std::string cmGeneratorTarget::GetInstallNameDirForInstallTree() const
{
  if (this->Makefile->IsOn("CMAKE_PLATFORM_HAS_INSTALLNAME")) {
    std::string dir;
    const char* install_name_dir = this->GetProperty("INSTALL_NAME_DIR");

    if (this->CanGenerateInstallNameDir(INSTALL_NAME_FOR_INSTALL)) {
      if (install_name_dir && *install_name_dir) {
        dir = install_name_dir;
        dir += "/";
      }
    }
    if (!install_name_dir) {
      if (this->MacOSXRpathInstallNameDirDefault()) {
        dir = "@rpath/";
      }
    }
    return dir;
  }
  return "";
}

cmListFileBacktrace cmGeneratorTarget::GetBacktrace() const
{
  return this->Target->GetBacktrace();
}

const std::vector<std::string>& cmGeneratorTarget::GetLinkDirectories() const
{
  return this->Target->GetLinkDirectories();
}

const std::set<std::string>& cmGeneratorTarget::GetUtilities() const
{
  return this->Target->GetUtilities();
}

const cmListFileBacktrace* cmGeneratorTarget::GetUtilityBacktrace(
  const std::string& u) const
{
  return this->Target->GetUtilityBacktrace(u);
}

bool cmGeneratorTarget::HaveWellDefinedOutputFiles() const
{
  return this->GetType() == cmStateEnums::STATIC_LIBRARY ||
    this->GetType() == cmStateEnums::SHARED_LIBRARY ||
    this->GetType() == cmStateEnums::MODULE_LIBRARY ||
    this->GetType() == cmStateEnums::EXECUTABLE;
}

const char* cmGeneratorTarget::GetExportMacro() const
{
  // Define the symbol for targets that export symbols.
  if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
      this->GetType() == cmStateEnums::MODULE_LIBRARY ||
      this->IsExecutableWithExports()) {
    if (const char* custom_export_name = this->GetProperty("DEFINE_SYMBOL")) {
      this->ExportMacro = custom_export_name;
    } else {
      std::string in = this->GetName();
      in += "_EXPORTS";
      this->ExportMacro = cmSystemTools::MakeCidentifier(in);
    }
    return this->ExportMacro.c_str();
  }
  return CM_NULLPTR;
}

class cmTargetCollectLinkLanguages
{
public:
  cmTargetCollectLinkLanguages(cmGeneratorTarget const* target,
                               const std::string& config,
                               CM_UNORDERED_SET<std::string>& languages,
                               cmGeneratorTarget const* head)
    : Config(config)
    , Languages(languages)
    , HeadTarget(head)
    , Target(target)
  {
    this->Visited.insert(target);
  }

  void Visit(cmLinkItem const& item)
  {
    if (!item.Target) {
      if (item.find("::") != std::string::npos) {
        bool noMessage = false;
        cmake::MessageType messageType = cmake::FATAL_ERROR;
        std::ostringstream e;
        switch (this->Target->GetLocalGenerator()->GetPolicyStatus(
          cmPolicies::CMP0028)) {
          case cmPolicies::WARN: {
            e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0028) << "\n";
            messageType = cmake::AUTHOR_WARNING;
          } break;
          case cmPolicies::OLD:
            noMessage = true;
          case cmPolicies::REQUIRED_IF_USED:
          case cmPolicies::REQUIRED_ALWAYS:
          case cmPolicies::NEW:
            // Issue the fatal message.
            break;
        }

        if (!noMessage) {
          e << "Target \"" << this->Target->GetName()
            << "\" links to target \"" << item
            << "\" but the target was not found.  Perhaps a find_package() "
               "call is missing for an IMPORTED target, or an ALIAS target is "
               "missing?";
          this->Target->GetLocalGenerator()->GetCMakeInstance()->IssueMessage(
            messageType, e.str(), this->Target->GetBacktrace());
        }
      }
      return;
    }
    if (!this->Visited.insert(item.Target).second) {
      return;
    }
    cmLinkInterface const* iface =
      item.Target->GetLinkInterface(this->Config, this->HeadTarget);
    if (!iface) {
      return;
    }

    for (std::vector<std::string>::const_iterator li =
           iface->Languages.begin();
         li != iface->Languages.end(); ++li) {
      this->Languages.insert(*li);
    }

    for (std::vector<cmLinkItem>::const_iterator li = iface->Libraries.begin();
         li != iface->Libraries.end(); ++li) {
      this->Visit(*li);
    }
  }

private:
  std::string Config;
  CM_UNORDERED_SET<std::string>& Languages;
  cmGeneratorTarget const* HeadTarget;
  const cmGeneratorTarget* Target;
  std::set<cmGeneratorTarget const*> Visited;
};

cmGeneratorTarget::LinkClosure const* cmGeneratorTarget::GetLinkClosure(
  const std::string& config) const
{
  std::string key(cmSystemTools::UpperCase(config));
  LinkClosureMapType::iterator i = this->LinkClosureMap.find(key);
  if (i == this->LinkClosureMap.end()) {
    LinkClosure lc;
    this->ComputeLinkClosure(config, lc);
    LinkClosureMapType::value_type entry(key, lc);
    i = this->LinkClosureMap.insert(entry).first;
  }
  return &i->second;
}

class cmTargetSelectLinker
{
  int Preference;
  cmGeneratorTarget const* Target;
  cmGlobalGenerator* GG;
  std::set<std::string> Preferred;

public:
  cmTargetSelectLinker(cmGeneratorTarget const* target)
    : Preference(0)
    , Target(target)
  {
    this->GG = this->Target->GetLocalGenerator()->GetGlobalGenerator();
  }
  void Consider(const char* lang)
  {
    int preference = this->GG->GetLinkerPreference(lang);
    if (preference > this->Preference) {
      this->Preference = preference;
      this->Preferred.clear();
    }
    if (preference == this->Preference) {
      this->Preferred.insert(lang);
    }
  }
  std::string Choose()
  {
    if (this->Preferred.empty()) {
      return "";
    }
    if (this->Preferred.size() > 1) {
      std::ostringstream e;
      e << "Target " << this->Target->GetName()
        << " contains multiple languages with the highest linker preference"
        << " (" << this->Preference << "):\n";
      for (std::set<std::string>::const_iterator li = this->Preferred.begin();
           li != this->Preferred.end(); ++li) {
        e << "  " << *li << "\n";
      }
      e << "Set the LINKER_LANGUAGE property for this target.";
      cmake* cm = this->Target->GetLocalGenerator()->GetCMakeInstance();
      cm->IssueMessage(cmake::FATAL_ERROR, e.str(),
                       this->Target->GetBacktrace());
    }
    return *this->Preferred.begin();
  }
};

void cmGeneratorTarget::ComputeLinkClosure(const std::string& config,
                                           LinkClosure& lc) const
{
  // Get languages built in this target.
  CM_UNORDERED_SET<std::string> languages;
  cmLinkImplementation const* impl = this->GetLinkImplementation(config);
  assert(impl);
  for (std::vector<std::string>::const_iterator li = impl->Languages.begin();
       li != impl->Languages.end(); ++li) {
    languages.insert(*li);
  }

  // Add interface languages from linked targets.
  cmTargetCollectLinkLanguages cll(this, config, languages, this);
  for (std::vector<cmLinkImplItem>::const_iterator li =
         impl->Libraries.begin();
       li != impl->Libraries.end(); ++li) {
    cll.Visit(*li);
  }

  // Store the transitive closure of languages.
  for (CM_UNORDERED_SET<std::string>::const_iterator li = languages.begin();
       li != languages.end(); ++li) {
    lc.Languages.push_back(*li);
  }

  // Choose the language whose linker should be used.
  if (this->GetProperty("HAS_CXX")) {
    lc.LinkerLanguage = "CXX";
  } else if (const char* linkerLang = this->GetProperty("LINKER_LANGUAGE")) {
    lc.LinkerLanguage = linkerLang;
  } else {
    // Find the language with the highest preference value.
    cmTargetSelectLinker tsl(this);

    // First select from the languages compiled directly in this target.
    for (std::vector<std::string>::const_iterator li = impl->Languages.begin();
         li != impl->Languages.end(); ++li) {
      tsl.Consider(li->c_str());
    }

    // Now consider languages that propagate from linked targets.
    for (CM_UNORDERED_SET<std::string>::const_iterator sit = languages.begin();
         sit != languages.end(); ++sit) {
      std::string propagates =
        "CMAKE_" + *sit + "_LINKER_PREFERENCE_PROPAGATES";
      if (this->Makefile->IsOn(propagates)) {
        tsl.Consider(sit->c_str());
      }
    }

    lc.LinkerLanguage = tsl.Choose();
  }
}

void cmGeneratorTarget::GetFullNameComponents(
  std::string& prefix, std::string& base, std::string& suffix,
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  this->GetFullNameInternal(config, artifact, prefix, base, suffix);
}

std::string cmGeneratorTarget::BuildBundleDirectory(
  const std::string& base, const std::string& config,
  BundleDirectoryLevel level) const
{
  std::string fpath = base;
  if (this->IsAppBundleOnApple()) {
    fpath += this->GetAppBundleDirectory(config, level);
  }
  if (this->IsFrameworkOnApple()) {
    fpath += this->GetFrameworkDirectory(config, level);
  }
  if (this->IsCFBundleOnApple()) {
    fpath += this->GetCFBundleDirectory(config, level);
  }
  return fpath;
}

std::string cmGeneratorTarget::GetMacContentDirectory(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  // Start with the output directory for the target.
  std::string fpath = this->GetDirectory(config, artifact);
  fpath += "/";
  BundleDirectoryLevel level = ContentLevel;
  if (this->IsFrameworkOnApple()) {
    // additional files with a framework go into the version specific
    // directory
    level = FullLevel;
  }
  fpath = this->BuildBundleDirectory(fpath, config, level);
  return fpath;
}

std::string cmGeneratorTarget::GetEffectiveFolderName() const
{
  std::string effectiveFolder;

  if (!this->GlobalGenerator->UseFolderProperty()) {
    return effectiveFolder;
  }

  const char* targetFolder = this->GetProperty("FOLDER");
  if (targetFolder) {
    effectiveFolder += targetFolder;
  }

  return effectiveFolder;
}

cmGeneratorTarget::CompileInfo const* cmGeneratorTarget::GetCompileInfo(
  const std::string& config) const
{
  // There is no compile information for imported targets.
  if (this->IsImported()) {
    return CM_NULLPTR;
  }

  if (this->GetType() > cmStateEnums::OBJECT_LIBRARY) {
    std::string msg = "cmTarget::GetCompileInfo called for ";
    msg += this->GetName();
    msg += " which has type ";
    msg += cmState::GetTargetTypeName(this->GetType());
    this->LocalGenerator->IssueMessage(cmake::INTERNAL_ERROR, msg);
    return CM_NULLPTR;
  }

  // Lookup/compute/cache the compile information for this configuration.
  std::string config_upper;
  if (!config.empty()) {
    config_upper = cmSystemTools::UpperCase(config);
  }
  CompileInfoMapType::const_iterator i =
    this->CompileInfoMap.find(config_upper);
  if (i == this->CompileInfoMap.end()) {
    CompileInfo info;
    this->ComputePDBOutputDir("COMPILE_PDB", config, info.CompilePdbDir);
    CompileInfoMapType::value_type entry(config_upper, info);
    i = this->CompileInfoMap.insert(entry).first;
  }
  return &i->second;
}

cmGeneratorTarget::ModuleDefinitionInfo const*
cmGeneratorTarget::GetModuleDefinitionInfo(std::string const& config) const
{
  // A module definition file only makes sense on certain target types.
  if (this->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GetType() != cmStateEnums::MODULE_LIBRARY &&
      !this->IsExecutableWithExports()) {
    return CM_NULLPTR;
  }

  // Lookup/compute/cache the compile information for this configuration.
  std::string config_upper;
  if (!config.empty()) {
    config_upper = cmSystemTools::UpperCase(config);
  }
  ModuleDefinitionInfoMapType::const_iterator i =
    this->ModuleDefinitionInfoMap.find(config_upper);
  if (i == this->ModuleDefinitionInfoMap.end()) {
    ModuleDefinitionInfo info;
    this->ComputeModuleDefinitionInfo(config, info);
    ModuleDefinitionInfoMapType::value_type entry(config_upper, info);
    i = this->ModuleDefinitionInfoMap.insert(entry).first;
  }
  return &i->second;
}

void cmGeneratorTarget::ComputeModuleDefinitionInfo(
  std::string const& config, ModuleDefinitionInfo& info) const
{
  this->GetModuleDefinitionSources(info.Sources, config);
  info.WindowsExportAllSymbols =
    this->Makefile->IsOn("CMAKE_SUPPORT_WINDOWS_EXPORT_ALL_SYMBOLS") &&
    this->GetPropertyAsBool("WINDOWS_EXPORT_ALL_SYMBOLS");
  info.DefFileGenerated =
    info.WindowsExportAllSymbols || info.Sources.size() > 1;
  if (info.DefFileGenerated) {
    info.DefFile = this->ObjectDirectory /* has slash */ + "exports.def";
  } else if (!info.Sources.empty()) {
    info.DefFile = info.Sources.front()->GetFullPath();
  }
}

bool cmGeneratorTarget::IsDLLPlatform() const
{
  return this->DLLPlatform;
}

void cmGeneratorTarget::GetAutoUicOptions(std::vector<std::string>& result,
                                          const std::string& config) const
{
  const char* prop =
    this->GetLinkInterfaceDependentStringProperty("AUTOUIC_OPTIONS", config);
  if (!prop) {
    return;
  }
  cmGeneratorExpression ge;

  cmGeneratorExpressionDAGChecker dagChecker(
    this->GetName(), "AUTOUIC_OPTIONS", CM_NULLPTR, CM_NULLPTR);
  cmSystemTools::ExpandListArgument(
    ge.Parse(prop)->Evaluate(this->LocalGenerator, config, false, this,
                             &dagChecker),
    result);
}

void processILibs(const std::string& config,
                  cmGeneratorTarget const* headTarget, cmLinkItem const& item,
                  cmGlobalGenerator* gg,
                  std::vector<cmGeneratorTarget const*>& tgts,
                  std::set<cmGeneratorTarget const*>& emitted)
{
  if (item.Target && emitted.insert(item.Target).second) {
    tgts.push_back(item.Target);
    if (cmLinkInterfaceLibraries const* iface =
          item.Target->GetLinkInterfaceLibraries(config, headTarget, true)) {
      for (std::vector<cmLinkItem>::const_iterator it =
             iface->Libraries.begin();
           it != iface->Libraries.end(); ++it) {
        processILibs(config, headTarget, *it, gg, tgts, emitted);
      }
    }
  }
}

const std::vector<const cmGeneratorTarget*>&
cmGeneratorTarget::GetLinkImplementationClosure(
  const std::string& config) const
{
  LinkImplClosure& tgts = this->LinkImplClosureMap[config];
  if (!tgts.Done) {
    tgts.Done = true;
    std::set<cmGeneratorTarget const*> emitted;

    cmLinkImplementationLibraries const* impl =
      this->GetLinkImplementationLibraries(config);

    for (std::vector<cmLinkImplItem>::const_iterator it =
           impl->Libraries.begin();
         it != impl->Libraries.end(); ++it) {
      processILibs(config, this, *it,
                   this->LocalGenerator->GetGlobalGenerator(), tgts, emitted);
    }
  }
  return tgts;
}

class cmTargetTraceDependencies
{
public:
  cmTargetTraceDependencies(cmGeneratorTarget* target);
  void Trace();

private:
  cmGeneratorTarget* GeneratorTarget;
  cmMakefile* Makefile;
  cmLocalGenerator* LocalGenerator;
  cmGlobalGenerator const* GlobalGenerator;
  typedef cmGeneratorTarget::SourceEntry SourceEntry;
  SourceEntry* CurrentEntry;
  std::queue<cmSourceFile*> SourceQueue;
  std::set<cmSourceFile*> SourcesQueued;
  typedef std::map<std::string, cmSourceFile*> NameMapType;
  NameMapType NameMap;
  std::vector<std::string> NewSources;

  void QueueSource(cmSourceFile* sf);
  void FollowName(std::string const& name);
  void FollowNames(std::vector<std::string> const& names);
  bool IsUtility(std::string const& dep);
  void CheckCustomCommand(cmCustomCommand const& cc);
  void CheckCustomCommands(const std::vector<cmCustomCommand>& commands);
  void FollowCommandDepends(cmCustomCommand const& cc,
                            const std::string& config,
                            std::set<std::string>& emitted);
};

cmTargetTraceDependencies::cmTargetTraceDependencies(cmGeneratorTarget* target)
  : GeneratorTarget(target)
{
  // Convenience.
  this->Makefile = target->Target->GetMakefile();
  this->LocalGenerator = target->GetLocalGenerator();
  this->GlobalGenerator = this->LocalGenerator->GetGlobalGenerator();
  this->CurrentEntry = CM_NULLPTR;

  // Queue all the source files already specified for the target.
  if (target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
    std::vector<std::string> configs;
    this->Makefile->GetConfigurations(configs);
    if (configs.empty()) {
      configs.push_back("");
    }
    std::set<cmSourceFile*> emitted;
    for (std::vector<std::string>::const_iterator ci = configs.begin();
         ci != configs.end(); ++ci) {
      std::vector<cmSourceFile*> sources;
      this->GeneratorTarget->GetSourceFiles(sources, *ci);
      for (std::vector<cmSourceFile*>::const_iterator si = sources.begin();
           si != sources.end(); ++si) {
        cmSourceFile* sf = *si;
        const std::set<cmGeneratorTarget const*> tgts =
          this->GlobalGenerator->GetFilenameTargetDepends(sf);
        if (tgts.find(this->GeneratorTarget) != tgts.end()) {
          std::ostringstream e;
          e << "Evaluation output file\n  \"" << sf->GetFullPath()
            << "\"\ndepends on the sources of a target it is used in.  This "
               "is a dependency loop and is not allowed.";
          this->GeneratorTarget->LocalGenerator->IssueMessage(
            cmake::FATAL_ERROR, e.str());
          return;
        }
        if (emitted.insert(sf).second &&
            this->SourcesQueued.insert(sf).second) {
          this->SourceQueue.push(sf);
        }
      }
    }
  }

  // Queue pre-build, pre-link, and post-build rule dependencies.
  this->CheckCustomCommands(this->GeneratorTarget->GetPreBuildCommands());
  this->CheckCustomCommands(this->GeneratorTarget->GetPreLinkCommands());
  this->CheckCustomCommands(this->GeneratorTarget->GetPostBuildCommands());
}

void cmTargetTraceDependencies::Trace()
{
  // Process one dependency at a time until the queue is empty.
  while (!this->SourceQueue.empty()) {
    // Get the next source from the queue.
    cmSourceFile* sf = this->SourceQueue.front();
    this->SourceQueue.pop();
    this->CurrentEntry = &this->GeneratorTarget->SourceDepends[sf];

    // Queue dependencies added explicitly by the user.
    if (const char* additionalDeps = sf->GetProperty("OBJECT_DEPENDS")) {
      std::vector<std::string> objDeps;
      cmSystemTools::ExpandListArgument(additionalDeps, objDeps);
      for (std::vector<std::string>::iterator odi = objDeps.begin();
           odi != objDeps.end(); ++odi) {
        if (cmSystemTools::FileIsFullPath(*odi)) {
          *odi = cmSystemTools::CollapseFullPath(*odi);
        }
      }
      this->FollowNames(objDeps);
    }

    // Queue the source needed to generate this file, if any.
    this->FollowName(sf->GetFullPath());

    // Queue dependencies added programatically by commands.
    this->FollowNames(sf->GetDepends());

    // Queue custom command dependencies.
    if (cmCustomCommand const* cc = sf->GetCustomCommand()) {
      this->CheckCustomCommand(*cc);
    }
  }
  this->CurrentEntry = CM_NULLPTR;

  this->GeneratorTarget->AddTracedSources(this->NewSources);
}

void cmTargetTraceDependencies::QueueSource(cmSourceFile* sf)
{
  if (this->SourcesQueued.insert(sf).second) {
    this->SourceQueue.push(sf);

    // Make sure this file is in the target at the end.
    this->NewSources.push_back(sf->GetFullPath());
  }
}

void cmTargetTraceDependencies::FollowName(std::string const& name)
{
  NameMapType::iterator i = this->NameMap.find(name);
  if (i == this->NameMap.end()) {
    // Check if we know how to generate this file.
    cmSourceFile* sf = this->Makefile->GetSourceFileWithOutput(name);
    NameMapType::value_type entry(name, sf);
    i = this->NameMap.insert(entry).first;
  }
  if (cmSourceFile* sf = i->second) {
    // Record the dependency we just followed.
    if (this->CurrentEntry) {
      this->CurrentEntry->Depends.push_back(sf);
    }
    this->QueueSource(sf);
  }
}

void cmTargetTraceDependencies::FollowNames(
  std::vector<std::string> const& names)
{
  for (std::vector<std::string>::const_iterator i = names.begin();
       i != names.end(); ++i) {
    this->FollowName(*i);
  }
}

bool cmTargetTraceDependencies::IsUtility(std::string const& dep)
{
  // Dependencies on targets (utilities) are supposed to be named by
  // just the target name.  However for compatibility we support
  // naming the output file generated by the target (assuming there is
  // no output-name property which old code would not have set).  In
  // that case the target name will be the file basename of the
  // dependency.
  std::string util = cmSystemTools::GetFilenameName(dep);
  if (cmSystemTools::GetFilenameLastExtension(util) == ".exe") {
    util = cmSystemTools::GetFilenameWithoutLastExtension(util);
  }

  // Check for a target with this name.
  if (cmGeneratorTarget* t =
        this->GeneratorTarget->GetLocalGenerator()->FindGeneratorTargetToUse(
          util)) {
    // If we find the target and the dep was given as a full path,
    // then make sure it was not a full path to something else, and
    // the fact that the name matched a target was just a coincidence.
    if (cmSystemTools::FileIsFullPath(dep.c_str())) {
      if (t->GetType() >= cmStateEnums::EXECUTABLE &&
          t->GetType() <= cmStateEnums::MODULE_LIBRARY) {
        // This is really only for compatibility so we do not need to
        // worry about configuration names and output names.
        std::string tLocation = t->GetLocationForBuild();
        tLocation = cmSystemTools::GetFilenamePath(tLocation);
        std::string depLocation = cmSystemTools::GetFilenamePath(dep);
        depLocation = cmSystemTools::CollapseFullPath(depLocation);
        tLocation = cmSystemTools::CollapseFullPath(tLocation);
        if (depLocation == tLocation) {
          this->GeneratorTarget->Target->AddUtility(util);
          return true;
        }
      }
    } else {
      // The original name of the dependency was not a full path.  It
      // must name a target, so add the target-level dependency.
      this->GeneratorTarget->Target->AddUtility(util);
      return true;
    }
  }

  // The dependency does not name a target built in this project.
  return false;
}

void cmTargetTraceDependencies::CheckCustomCommand(cmCustomCommand const& cc)
{
  // Transform command names that reference targets built in this
  // project to corresponding target-level dependencies.
  cmGeneratorExpression ge(cc.GetBacktrace());

  // Add target-level dependencies referenced by generator expressions.
  std::set<cmGeneratorTarget*> targets;

  for (cmCustomCommandLines::const_iterator cit = cc.GetCommandLines().begin();
       cit != cc.GetCommandLines().end(); ++cit) {
    std::string const& command = *cit->begin();
    // Check for a target with this name.
    if (cmGeneratorTarget* t =
          this->LocalGenerator->FindGeneratorTargetToUse(command)) {
      if (t->GetType() == cmStateEnums::EXECUTABLE) {
        // The command refers to an executable target built in
        // this project.  Add the target-level dependency to make
        // sure the executable is up to date before this custom
        // command possibly runs.
        this->GeneratorTarget->Target->AddUtility(command);
      }
    }

    // Check for target references in generator expressions.
    for (cmCustomCommandLine::const_iterator cli = cit->begin();
         cli != cit->end(); ++cli) {
      const CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(*cli);
      cge->Evaluate(this->GeneratorTarget->GetLocalGenerator(), "", true);
      std::set<cmGeneratorTarget*> geTargets = cge->GetTargets();
      targets.insert(geTargets.begin(), geTargets.end());
    }
  }

  for (std::set<cmGeneratorTarget*>::iterator ti = targets.begin();
       ti != targets.end(); ++ti) {
    this->GeneratorTarget->Target->AddUtility((*ti)->GetName());
  }

  // Queue the custom command dependencies.
  std::vector<std::string> configs;
  std::set<std::string> emitted;
  this->Makefile->GetConfigurations(configs);
  if (configs.empty()) {
    configs.push_back("");
  }
  for (std::vector<std::string>::const_iterator ci = configs.begin();
       ci != configs.end(); ++ci) {
    this->FollowCommandDepends(cc, *ci, emitted);
  }
}

void cmTargetTraceDependencies::FollowCommandDepends(
  cmCustomCommand const& cc, const std::string& config,
  std::set<std::string>& emitted)
{
  cmCustomCommandGenerator ccg(cc, config,
                               this->GeneratorTarget->LocalGenerator);

  const std::vector<std::string>& depends = ccg.GetDepends();

  for (std::vector<std::string>::const_iterator di = depends.begin();
       di != depends.end(); ++di) {
    std::string const& dep = *di;
    if (emitted.insert(dep).second) {
      if (!this->IsUtility(dep)) {
        // The dependency does not name a target and may be a file we
        // know how to generate.  Queue it.
        this->FollowName(dep);
      }
    }
  }
}

void cmTargetTraceDependencies::CheckCustomCommands(
  const std::vector<cmCustomCommand>& commands)
{
  for (std::vector<cmCustomCommand>::const_iterator cli = commands.begin();
       cli != commands.end(); ++cli) {
    this->CheckCustomCommand(*cli);
  }
}

void cmGeneratorTarget::TraceDependencies()
{
  // CMake-generated targets have no dependencies to trace.  Normally tracing
  // would find nothing anyway, but when building CMake itself the "install"
  // target command ends up referencing the "cmake" target but we do not
  // really want the dependency because "install" depend on "all" anyway.
  if (this->GetType() == cmStateEnums::GLOBAL_TARGET) {
    return;
  }

  // Use a helper object to trace the dependencies.
  cmTargetTraceDependencies tracer(this);
  tracer.Trace();
}

std::string cmGeneratorTarget::GetCompilePDBDirectory(
  const std::string& config) const
{
  if (CompileInfo const* info = this->GetCompileInfo(config)) {
    return info->CompilePdbDir;
  }
  return "";
}

void cmGeneratorTarget::GetAppleArchs(const std::string& config,
                                      std::vector<std::string>& archVec) const
{
  const char* archs = CM_NULLPTR;
  if (!config.empty()) {
    std::string defVarName = "OSX_ARCHITECTURES_";
    defVarName += cmSystemTools::UpperCase(config);
    archs = this->GetProperty(defVarName);
  }
  if (!archs) {
    archs = this->GetProperty("OSX_ARCHITECTURES");
  }
  if (archs) {
    cmSystemTools::ExpandListArgument(std::string(archs), archVec);
  }
}

//----------------------------------------------------------------------------
std::string cmGeneratorTarget::GetFeatureSpecificLinkRuleVariable(
  std::string const& var, std::string const& lang,
  std::string const& config) const
{
  if (this->IsIPOEnabled(lang, config)) {
    std::string varIPO = var + "_IPO";
    if (this->Makefile->IsDefinitionSet(varIPO)) {
      return varIPO;
    }
  }

  return var;
}

//----------------------------------------------------------------------------
std::string cmGeneratorTarget::GetCreateRuleVariable(
  std::string const& lang, std::string const& config) const
{
  switch (this->GetType()) {
    case cmStateEnums::STATIC_LIBRARY: {
      std::string var = "CMAKE_" + lang + "_CREATE_STATIC_LIBRARY";
      return this->GetFeatureSpecificLinkRuleVariable(var, lang, config);
    }
    case cmStateEnums::SHARED_LIBRARY:
      return "CMAKE_" + lang + "_CREATE_SHARED_LIBRARY";
    case cmStateEnums::MODULE_LIBRARY:
      return "CMAKE_" + lang + "_CREATE_SHARED_MODULE";
    case cmStateEnums::EXECUTABLE:
      return "CMAKE_" + lang + "_LINK_EXECUTABLE";
    default:
      break;
  }
  return "";
}
static void processIncludeDirectories(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& includes,
  CM_UNORDERED_SET<std::string>& uniqueIncludes,
  cmGeneratorExpressionDAGChecker* dagChecker, const std::string& config,
  bool debugIncludes, const std::string& language)
{
  for (std::vector<cmGeneratorTarget::TargetPropertyEntry *>::const_iterator
         it = entries.begin(),
         end = entries.end();
       it != end; ++it) {
    cmLinkImplItem const& item = (*it)->LinkImplItem;
    std::string const& targetName = item;
    bool const fromImported = item.Target && item.Target->IsImported();
    bool const checkCMP0027 = item.FromGenex;
    std::vector<std::string> entryIncludes;
    cmSystemTools::ExpandListArgument(
      (*it)->ge->Evaluate(tgt->GetLocalGenerator(), config, false, tgt,
                          dagChecker, language),
      entryIncludes);

    std::string usedIncludes;
    for (std::vector<std::string>::iterator li = entryIncludes.begin();
         li != entryIncludes.end(); ++li) {
      if (fromImported && !cmSystemTools::FileExists(li->c_str())) {
        std::ostringstream e;
        cmake::MessageType messageType = cmake::FATAL_ERROR;
        if (checkCMP0027) {
          switch (tgt->GetPolicyStatusCMP0027()) {
            case cmPolicies::WARN:
              e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0027) << "\n";
              CM_FALLTHROUGH;
            case cmPolicies::OLD:
              messageType = cmake::AUTHOR_WARNING;
              break;
            case cmPolicies::REQUIRED_ALWAYS:
            case cmPolicies::REQUIRED_IF_USED:
            case cmPolicies::NEW:
              break;
          }
        }
        /* clang-format off */
        e << "Imported target \"" << targetName << "\" includes "
             "non-existent path\n  \"" << *li << "\"\nin its "
             "INTERFACE_INCLUDE_DIRECTORIES. Possible reasons include:\n"
             "* The path was deleted, renamed, or moved to another "
             "location.\n"
             "* An install or uninstall procedure did not complete "
             "successfully.\n"
             "* The installation package was faulty and references files it "
             "does not provide.\n";
        /* clang-format on */
        tgt->GetLocalGenerator()->IssueMessage(messageType, e.str());
        return;
      }

      if (!cmSystemTools::FileIsFullPath(li->c_str())) {
        std::ostringstream e;
        bool noMessage = false;
        cmake::MessageType messageType = cmake::FATAL_ERROR;
        if (!targetName.empty()) {
          /* clang-format off */
          e << "Target \"" << targetName << "\" contains relative "
            "path in its INTERFACE_INCLUDE_DIRECTORIES:\n"
            "  \"" << *li << "\"";
          /* clang-format on */
        } else {
          switch (tgt->GetPolicyStatusCMP0021()) {
            case cmPolicies::WARN: {
              e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0021) << "\n";
              messageType = cmake::AUTHOR_WARNING;
            } break;
            case cmPolicies::OLD:
              noMessage = true;
            case cmPolicies::REQUIRED_IF_USED:
            case cmPolicies::REQUIRED_ALWAYS:
            case cmPolicies::NEW:
              // Issue the fatal message.
              break;
          }
          e << "Found relative path while evaluating include directories of "
               "\""
            << tgt->GetName() << "\":\n  \"" << *li << "\"\n";
        }
        if (!noMessage) {
          tgt->GetLocalGenerator()->IssueMessage(messageType, e.str());
          if (messageType == cmake::FATAL_ERROR) {
            return;
          }
        }
      }

      if (!cmSystemTools::IsOff(li->c_str())) {
        cmSystemTools::ConvertToUnixSlashes(*li);
      }
      std::string inc = *li;

      if (uniqueIncludes.insert(inc).second) {
        includes.push_back(inc);
        if (debugIncludes) {
          usedIncludes += " * " + inc + "\n";
        }
      }
    }
    if (!usedIncludes.empty()) {
      tgt->GetLocalGenerator()->GetCMakeInstance()->IssueMessage(
        cmake::LOG, std::string("Used includes for target ") + tgt->GetName() +
          ":\n" + usedIncludes,
        (*it)->ge->GetBacktrace());
    }
  }
}

std::vector<std::string> cmGeneratorTarget::GetIncludeDirectories(
  const std::string& config, const std::string& lang) const
{
  std::vector<std::string> includes;
  CM_UNORDERED_SET<std::string> uniqueIncludes;

  cmGeneratorExpressionDAGChecker dagChecker(
    this->GetName(), "INCLUDE_DIRECTORIES", CM_NULLPTR, CM_NULLPTR);

  std::vector<std::string> debugProperties;
  const char* debugProp =
    this->Makefile->GetDefinition("CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugIncludes = !this->DebugIncludesDone &&
    std::find(debugProperties.begin(), debugProperties.end(),
              "INCLUDE_DIRECTORIES") != debugProperties.end();

  if (this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    this->DebugIncludesDone = true;
  }

  processIncludeDirectories(this, this->IncludeDirectoriesEntries, includes,
                            uniqueIncludes, &dagChecker, config, debugIncludes,
                            lang);

  std::vector<cmGeneratorTarget::TargetPropertyEntry*>
    linkInterfaceIncludeDirectoriesEntries;
  AddInterfaceEntries(this, config, "INTERFACE_INCLUDE_DIRECTORIES",
                      linkInterfaceIncludeDirectoriesEntries);

  if (this->Makefile->IsOn("APPLE")) {
    cmLinkImplementationLibraries const* impl =
      this->GetLinkImplementationLibraries(config);
    for (std::vector<cmLinkImplItem>::const_iterator it =
           impl->Libraries.begin();
         it != impl->Libraries.end(); ++it) {
      std::string libDir = cmSystemTools::CollapseFullPath(*it);

      static cmsys::RegularExpression frameworkCheck(
        "(.*\\.framework)(/Versions/[^/]+)?/[^/]+$");
      if (!frameworkCheck.find(libDir)) {
        continue;
      }

      libDir = frameworkCheck.match(1);

      cmGeneratorExpression ge;
      CM_AUTO_PTR<cmCompiledGeneratorExpression> cge =
        ge.Parse(libDir.c_str());
      linkInterfaceIncludeDirectoriesEntries.push_back(
        new cmGeneratorTarget::TargetPropertyEntry(cge));
    }
  }

  processIncludeDirectories(this, linkInterfaceIncludeDirectoriesEntries,
                            includes, uniqueIncludes, &dagChecker, config,
                            debugIncludes, lang);

  cmDeleteAll(linkInterfaceIncludeDirectoriesEntries);

  return includes;
}

static void processCompileOptionsInternal(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& options,
  CM_UNORDERED_SET<std::string>& uniqueOptions,
  cmGeneratorExpressionDAGChecker* dagChecker, const std::string& config,
  bool debugOptions, const char* logName, std::string const& language)
{
  for (std::vector<cmGeneratorTarget::TargetPropertyEntry *>::const_iterator
         it = entries.begin(),
         end = entries.end();
       it != end; ++it) {
    std::vector<std::string> entryOptions;
    cmSystemTools::ExpandListArgument(
      (*it)->ge->Evaluate(tgt->GetLocalGenerator(), config, false, tgt,
                          dagChecker, language),
      entryOptions);
    std::string usedOptions;
    for (std::vector<std::string>::iterator li = entryOptions.begin();
         li != entryOptions.end(); ++li) {
      std::string const& opt = *li;

      if (uniqueOptions.insert(opt).second) {
        options.push_back(opt);
        if (debugOptions) {
          usedOptions += " * " + opt + "\n";
        }
      }
    }
    if (!usedOptions.empty()) {
      tgt->GetLocalGenerator()->GetCMakeInstance()->IssueMessage(
        cmake::LOG, std::string("Used compile ") + logName +
          std::string(" for target ") + tgt->GetName() + ":\n" + usedOptions,
        (*it)->ge->GetBacktrace());
    }
  }
}

static void processCompileOptions(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& options,
  CM_UNORDERED_SET<std::string>& uniqueOptions,
  cmGeneratorExpressionDAGChecker* dagChecker, const std::string& config,
  bool debugOptions, std::string const& language)
{
  processCompileOptionsInternal(tgt, entries, options, uniqueOptions,
                                dagChecker, config, debugOptions, "options",
                                language);
}

void cmGeneratorTarget::GetCompileOptions(std::vector<std::string>& result,
                                          const std::string& config,
                                          const std::string& language) const
{
  CM_UNORDERED_SET<std::string> uniqueOptions;

  cmGeneratorExpressionDAGChecker dagChecker(
    this->GetName(), "COMPILE_OPTIONS", CM_NULLPTR, CM_NULLPTR);

  std::vector<std::string> debugProperties;
  const char* debugProp =
    this->Makefile->GetDefinition("CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugOptions = !this->DebugCompileOptionsDone &&
    std::find(debugProperties.begin(), debugProperties.end(),
              "COMPILE_OPTIONS") != debugProperties.end();

  if (this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    this->DebugCompileOptionsDone = true;
  }

  processCompileOptions(this, this->CompileOptionsEntries, result,
                        uniqueOptions, &dagChecker, config, debugOptions,
                        language);

  std::vector<cmGeneratorTarget::TargetPropertyEntry*>
    linkInterfaceCompileOptionsEntries;

  AddInterfaceEntries(this, config, "INTERFACE_COMPILE_OPTIONS",
                      linkInterfaceCompileOptionsEntries);

  processCompileOptions(this, linkInterfaceCompileOptionsEntries, result,
                        uniqueOptions, &dagChecker, config, debugOptions,
                        language);

  cmDeleteAll(linkInterfaceCompileOptionsEntries);
}

static void processCompileFeatures(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& options,
  CM_UNORDERED_SET<std::string>& uniqueOptions,
  cmGeneratorExpressionDAGChecker* dagChecker, const std::string& config,
  bool debugOptions)
{
  processCompileOptionsInternal(tgt, entries, options, uniqueOptions,
                                dagChecker, config, debugOptions, "features",
                                std::string());
}

void cmGeneratorTarget::GetCompileFeatures(std::vector<std::string>& result,
                                           const std::string& config) const
{
  CM_UNORDERED_SET<std::string> uniqueFeatures;

  cmGeneratorExpressionDAGChecker dagChecker(
    this->GetName(), "COMPILE_FEATURES", CM_NULLPTR, CM_NULLPTR);

  std::vector<std::string> debugProperties;
  const char* debugProp =
    this->Makefile->GetDefinition("CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugFeatures = !this->DebugCompileFeaturesDone &&
    std::find(debugProperties.begin(), debugProperties.end(),
              "COMPILE_FEATURES") != debugProperties.end();

  if (this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    this->DebugCompileFeaturesDone = true;
  }

  processCompileFeatures(this, this->CompileFeaturesEntries, result,
                         uniqueFeatures, &dagChecker, config, debugFeatures);

  std::vector<cmGeneratorTarget::TargetPropertyEntry*>
    linkInterfaceCompileFeaturesEntries;
  AddInterfaceEntries(this, config, "INTERFACE_COMPILE_FEATURES",
                      linkInterfaceCompileFeaturesEntries);

  processCompileFeatures(this, linkInterfaceCompileFeaturesEntries, result,
                         uniqueFeatures, &dagChecker, config, debugFeatures);

  cmDeleteAll(linkInterfaceCompileFeaturesEntries);
}

static void processCompileDefinitions(
  cmGeneratorTarget const* tgt,
  const std::vector<cmGeneratorTarget::TargetPropertyEntry*>& entries,
  std::vector<std::string>& options,
  CM_UNORDERED_SET<std::string>& uniqueOptions,
  cmGeneratorExpressionDAGChecker* dagChecker, const std::string& config,
  bool debugOptions, std::string const& language)
{
  processCompileOptionsInternal(tgt, entries, options, uniqueOptions,
                                dagChecker, config, debugOptions,
                                "definitions", language);
}

void cmGeneratorTarget::GetCompileDefinitions(
  std::vector<std::string>& list, const std::string& config,
  const std::string& language) const
{
  CM_UNORDERED_SET<std::string> uniqueOptions;

  cmGeneratorExpressionDAGChecker dagChecker(
    this->GetName(), "COMPILE_DEFINITIONS", CM_NULLPTR, CM_NULLPTR);

  std::vector<std::string> debugProperties;
  const char* debugProp =
    this->Makefile->GetDefinition("CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugDefines = !this->DebugCompileDefinitionsDone &&
    std::find(debugProperties.begin(), debugProperties.end(),
              "COMPILE_DEFINITIONS") != debugProperties.end();

  if (this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    this->DebugCompileDefinitionsDone = true;
  }

  processCompileDefinitions(this, this->CompileDefinitionsEntries, list,
                            uniqueOptions, &dagChecker, config, debugDefines,
                            language);

  std::vector<cmGeneratorTarget::TargetPropertyEntry*>
    linkInterfaceCompileDefinitionsEntries;
  AddInterfaceEntries(this, config, "INTERFACE_COMPILE_DEFINITIONS",
                      linkInterfaceCompileDefinitionsEntries);
  if (!config.empty()) {
    std::string configPropName =
      "COMPILE_DEFINITIONS_" + cmSystemTools::UpperCase(config);
    const char* configProp = this->GetProperty(configPropName);
    if (configProp) {
      switch (this->Makefile->GetPolicyStatus(cmPolicies::CMP0043)) {
        case cmPolicies::WARN: {
          std::ostringstream e;
          e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0043);
          this->LocalGenerator->IssueMessage(cmake::AUTHOR_WARNING, e.str());
          CM_FALLTHROUGH;
        }
        case cmPolicies::OLD: {
          cmGeneratorExpression ge;
          CM_AUTO_PTR<cmCompiledGeneratorExpression> cge =
            ge.Parse(configProp);
          linkInterfaceCompileDefinitionsEntries.push_back(
            new cmGeneratorTarget::TargetPropertyEntry(cge));
        } break;
        case cmPolicies::NEW:
        case cmPolicies::REQUIRED_ALWAYS:
        case cmPolicies::REQUIRED_IF_USED:
          break;
      }
    }
  }

  processCompileDefinitions(this, linkInterfaceCompileDefinitionsEntries, list,
                            uniqueOptions, &dagChecker, config, debugDefines,
                            language);

  cmDeleteAll(linkInterfaceCompileDefinitionsEntries);
}

void cmGeneratorTarget::ComputeTargetManifest(const std::string& config) const
{
  if (this->IsImported()) {
    return;
  }
  cmGlobalGenerator* gg = this->LocalGenerator->GetGlobalGenerator();

  // Get the names.
  std::string name;
  std::string soName;
  std::string realName;
  std::string impName;
  std::string pdbName;
  if (this->GetType() == cmStateEnums::EXECUTABLE) {
    this->GetExecutableNames(name, realName, impName, pdbName, config);
  } else if (this->GetType() == cmStateEnums::STATIC_LIBRARY ||
             this->GetType() == cmStateEnums::SHARED_LIBRARY ||
             this->GetType() == cmStateEnums::MODULE_LIBRARY) {
    this->GetLibraryNames(name, soName, realName, impName, pdbName, config);
  } else {
    return;
  }

  // Get the directory.
  std::string dir =
    this->GetDirectory(config, cmStateEnums::RuntimeBinaryArtifact);

  // Add each name.
  std::string f;
  if (!name.empty()) {
    f = dir;
    f += "/";
    f += name;
    gg->AddToManifest(f);
  }
  if (!soName.empty()) {
    f = dir;
    f += "/";
    f += soName;
    gg->AddToManifest(f);
  }
  if (!realName.empty()) {
    f = dir;
    f += "/";
    f += realName;
    gg->AddToManifest(f);
  }
  if (!pdbName.empty()) {
    f = dir;
    f += "/";
    f += pdbName;
    gg->AddToManifest(f);
  }
  if (!impName.empty()) {
    f = this->GetDirectory(config, cmStateEnums::ImportLibraryArtifact);
    f += "/";
    f += impName;
    gg->AddToManifest(f);
  }
}

bool cmGeneratorTarget::ComputeCompileFeatures(std::string const& config) const
{
  std::vector<std::string> features;
  this->GetCompileFeatures(features, config);
  for (std::vector<std::string>::const_iterator it = features.begin();
       it != features.end(); ++it) {
    if (!this->Makefile->AddRequiredTargetFeature(this->Target, *it)) {
      return false;
    }
  }
  return true;
}

std::string cmGeneratorTarget::GetImportedLibName(
  std::string const& config) const
{
  if (cmGeneratorTarget::ImportInfo const* info =
        this->GetImportInfo(config)) {
    return info->LibName;
  }
  return std::string();
}

std::string cmGeneratorTarget::GetFullPath(const std::string& config,
                                           cmStateEnums::ArtifactType artifact,
                                           bool realname) const
{
  if (this->IsImported()) {
    return this->Target->ImportedGetFullPath(config, artifact);
  }
  return this->NormalGetFullPath(config, artifact, realname);
}

std::string cmGeneratorTarget::NormalGetFullPath(
  const std::string& config, cmStateEnums::ArtifactType artifact,
  bool realname) const
{
  std::string fpath = this->GetDirectory(config, artifact);
  fpath += "/";
  if (this->IsAppBundleOnApple()) {
    fpath = this->BuildBundleDirectory(fpath, config, FullLevel);
    fpath += "/";
  }

  // Add the full name of the target.
  switch (artifact) {
    case cmStateEnums::RuntimeBinaryArtifact:
      if (realname) {
        fpath += this->NormalGetRealName(config);
      } else {
        fpath +=
          this->GetFullName(config, cmStateEnums::RuntimeBinaryArtifact);
      }
      break;
    case cmStateEnums::ImportLibraryArtifact:
      fpath += this->GetFullName(config, cmStateEnums::ImportLibraryArtifact);
      break;
  }
  return fpath;
}

std::string cmGeneratorTarget::NormalGetRealName(
  const std::string& config) const
{
  // This should not be called for imported targets.
  // TODO: Split cmTarget into a class hierarchy to get compile-time
  // enforcement of the limited imported target API.
  if (this->IsImported()) {
    std::string msg = "NormalGetRealName called on imported target: ";
    msg += this->GetName();
    this->LocalGenerator->IssueMessage(cmake::INTERNAL_ERROR, msg);
  }

  if (this->GetType() == cmStateEnums::EXECUTABLE) {
    // Compute the real name that will be built.
    std::string name;
    std::string realName;
    std::string impName;
    std::string pdbName;
    this->GetExecutableNames(name, realName, impName, pdbName, config);
    return realName;
  }
  // Compute the real name that will be built.
  std::string name;
  std::string soName;
  std::string realName;
  std::string impName;
  std::string pdbName;
  this->GetLibraryNames(name, soName, realName, impName, pdbName, config);
  return realName;
}

void cmGeneratorTarget::GetLibraryNames(std::string& name, std::string& soName,
                                        std::string& realName,
                                        std::string& impName,
                                        std::string& pdbName,
                                        const std::string& config) const
{
  // This should not be called for imported targets.
  // TODO: Split cmTarget into a class hierarchy to get compile-time
  // enforcement of the limited imported target API.
  if (this->IsImported()) {
    std::string msg = "GetLibraryNames called on imported target: ";
    msg += this->GetName();
    this->LocalGenerator->IssueMessage(cmake::INTERNAL_ERROR, msg);
    return;
  }

  // Check for library version properties.
  const char* version = this->GetProperty("VERSION");
  const char* soversion = this->GetProperty("SOVERSION");
  if (!this->HasSOName(config) ||
      this->Makefile->IsOn("CMAKE_PLATFORM_NO_VERSIONED_SONAME") ||
      this->IsFrameworkOnApple()) {
    // Versioning is supported only for shared libraries and modules,
    // and then only when the platform supports an soname flag.
    version = CM_NULLPTR;
    soversion = CM_NULLPTR;
  }
  if (version && !soversion) {
    // The soversion must be set if the library version is set.  Use
    // the library version as the soversion.
    soversion = version;
  }
  if (!version && soversion) {
    // Use the soversion as the library version.
    version = soversion;
  }

  // Get the components of the library name.
  std::string prefix;
  std::string base;
  std::string suffix;
  this->GetFullNameInternal(config, cmStateEnums::RuntimeBinaryArtifact,
                            prefix, base, suffix);

  // The library name.
  name = prefix + base + suffix;

  if (this->IsFrameworkOnApple()) {
    realName = prefix;
    if (!this->Makefile->PlatformIsAppleIos()) {
      realName += "Versions/";
      realName += this->GetFrameworkVersion();
      realName += "/";
    }
    realName += base;
    soName = realName;
  } else {
    // The library's soname.
    this->ComputeVersionedName(soName, prefix, base, suffix, name, soversion);

    // The library's real name on disk.
    this->ComputeVersionedName(realName, prefix, base, suffix, name, version);
  }

  // The import library name.
  if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
      this->GetType() == cmStateEnums::MODULE_LIBRARY) {
    impName =
      this->GetFullNameInternal(config, cmStateEnums::ImportLibraryArtifact);
  } else {
    impName = "";
  }

  // The program database file name.
  pdbName = this->GetPDBName(config);
}

void cmGeneratorTarget::GetExecutableNames(std::string& name,
                                           std::string& realName,
                                           std::string& impName,
                                           std::string& pdbName,
                                           const std::string& config) const
{
  // This should not be called for imported targets.
  // TODO: Split cmTarget into a class hierarchy to get compile-time
  // enforcement of the limited imported target API.
  if (this->IsImported()) {
    std::string msg = "GetExecutableNames called on imported target: ";
    msg += this->GetName();
    this->LocalGenerator->IssueMessage(cmake::INTERNAL_ERROR, msg);
  }

// This versioning is supported only for executables and then only
// when the platform supports symbolic links.
#if defined(_WIN32) && !defined(__CYGWIN__)
  const char* version = 0;
#else
  // Check for executable version properties.
  const char* version = this->GetProperty("VERSION");
  if (this->GetType() != cmStateEnums::EXECUTABLE ||
      this->Makefile->IsOn("XCODE")) {
    version = CM_NULLPTR;
  }
#endif

  // Get the components of the executable name.
  std::string prefix;
  std::string base;
  std::string suffix;
  this->GetFullNameInternal(config, cmStateEnums::RuntimeBinaryArtifact,
                            prefix, base, suffix);

  // The executable name.
  name = prefix + base + suffix;

// The executable's real name on disk.
#if defined(__CYGWIN__)
  realName = prefix + base;
#else
  realName = name;
#endif
  if (version) {
    realName += "-";
    realName += version;
  }
#if defined(__CYGWIN__)
  realName += suffix;
#endif

  // The import library name.
  impName =
    this->GetFullNameInternal(config, cmStateEnums::ImportLibraryArtifact);

  // The program database file name.
  pdbName = this->GetPDBName(config);
}

std::string cmGeneratorTarget::GetFullNameInternal(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  std::string prefix;
  std::string base;
  std::string suffix;
  this->GetFullNameInternal(config, artifact, prefix, base, suffix);
  return prefix + base + suffix;
}

const char* cmGeneratorTarget::ImportedGetLocation(
  const std::string& config) const
{
  static std::string location;
  assert(this->IsImported());
  location = this->Target->ImportedGetFullPath(
    config, cmStateEnums::RuntimeBinaryArtifact);
  return location.c_str();
}

std::string cmGeneratorTarget::GetFullNameImported(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  return cmSystemTools::GetFilenameName(
    this->Target->ImportedGetFullPath(config, artifact));
}

void cmGeneratorTarget::GetFullNameInternal(
  const std::string& config, cmStateEnums::ArtifactType artifact,
  std::string& outPrefix, std::string& outBase, std::string& outSuffix) const
{
  // Use just the target name for non-main target types.
  if (this->GetType() != cmStateEnums::STATIC_LIBRARY &&
      this->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GetType() != cmStateEnums::MODULE_LIBRARY &&
      this->GetType() != cmStateEnums::EXECUTABLE) {
    outPrefix = "";
    outBase = this->GetName();
    outSuffix = "";
    return;
  }

  const bool isImportedLibraryArtifact =
    (artifact == cmStateEnums::ImportLibraryArtifact);

  // Return an empty name for the import library if this platform
  // does not support import libraries.
  if (isImportedLibraryArtifact &&
      !this->Makefile->GetDefinition("CMAKE_IMPORT_LIBRARY_SUFFIX")) {
    outPrefix = "";
    outBase = "";
    outSuffix = "";
    return;
  }

  // The implib option is only allowed for shared libraries, module
  // libraries, and executables.
  if (this->GetType() != cmStateEnums::SHARED_LIBRARY &&
      this->GetType() != cmStateEnums::MODULE_LIBRARY &&
      this->GetType() != cmStateEnums::EXECUTABLE) {
    artifact = cmStateEnums::RuntimeBinaryArtifact;
  }

  // Compute the full name for main target types.
  const char* targetPrefix =
    (isImportedLibraryArtifact ? this->GetProperty("IMPORT_PREFIX")
                               : this->GetProperty("PREFIX"));
  const char* targetSuffix =
    (isImportedLibraryArtifact ? this->GetProperty("IMPORT_SUFFIX")
                               : this->GetProperty("SUFFIX"));
  const char* configPostfix = CM_NULLPTR;
  if (!config.empty()) {
    std::string configProp = cmSystemTools::UpperCase(config);
    configProp += "_POSTFIX";
    configPostfix = this->GetProperty(configProp);
    // Mac application bundles and frameworks have no postfix.
    if (configPostfix &&
        (this->IsAppBundleOnApple() || this->IsFrameworkOnApple())) {
      configPostfix = CM_NULLPTR;
    }
  }
  const char* prefixVar = this->Target->GetPrefixVariableInternal(artifact);
  const char* suffixVar = this->Target->GetSuffixVariableInternal(artifact);

  // Check for language-specific default prefix and suffix.
  std::string ll = this->GetLinkerLanguage(config);
  if (!ll.empty()) {
    if (!targetSuffix && suffixVar && *suffixVar) {
      std::string langSuff = suffixVar + std::string("_") + ll;
      targetSuffix = this->Makefile->GetDefinition(langSuff);
    }
    if (!targetPrefix && prefixVar && *prefixVar) {
      std::string langPrefix = prefixVar + std::string("_") + ll;
      targetPrefix = this->Makefile->GetDefinition(langPrefix);
    }
  }

  // if there is no prefix on the target use the cmake definition
  if (!targetPrefix && prefixVar) {
    targetPrefix = this->Makefile->GetSafeDefinition(prefixVar);
  }
  // if there is no suffix on the target use the cmake definition
  if (!targetSuffix && suffixVar) {
    targetSuffix = this->Makefile->GetSafeDefinition(suffixVar);
  }

  // frameworks have directory prefix but no suffix
  std::string fw_prefix;
  if (this->IsFrameworkOnApple()) {
    fw_prefix = this->GetFrameworkDirectory(config, ContentLevel);
    fw_prefix += "/";
    targetPrefix = fw_prefix.c_str();
    targetSuffix = CM_NULLPTR;
  }

  if (this->IsCFBundleOnApple()) {
    fw_prefix = this->GetCFBundleDirectory(config, FullLevel);
    fw_prefix += "/";
    targetPrefix = fw_prefix.c_str();
    targetSuffix = CM_NULLPTR;
  }

  // Begin the final name with the prefix.
  outPrefix = targetPrefix ? targetPrefix : "";

  // Append the target name or property-specified name.
  outBase += this->GetOutputName(config, artifact);

  // Append the per-configuration postfix.
  outBase += configPostfix ? configPostfix : "";

  // Name shared libraries with their version number on some platforms.
  if (const char* soversion = this->GetProperty("SOVERSION")) {
    if (this->GetType() == cmStateEnums::SHARED_LIBRARY &&
        !isImportedLibraryArtifact &&
        this->Makefile->IsOn("CMAKE_SHARED_LIBRARY_NAME_WITH_VERSION")) {
      outBase += "-";
      outBase += soversion;
    }
  }

  // Append the suffix.
  outSuffix = targetSuffix ? targetSuffix : "";
}

std::string cmGeneratorTarget::GetLinkerLanguage(
  const std::string& config) const
{
  return this->GetLinkClosure(config)->LinkerLanguage;
}

std::string cmGeneratorTarget::GetPDBName(const std::string& config) const
{
  std::string prefix;
  std::string base;
  std::string suffix;
  this->GetFullNameInternal(config, cmStateEnums::RuntimeBinaryArtifact,
                            prefix, base, suffix);

  std::vector<std::string> props;
  std::string configUpper = cmSystemTools::UpperCase(config);
  if (!configUpper.empty()) {
    // PDB_NAME_<CONFIG>
    props.push_back("PDB_NAME_" + configUpper);
  }

  // PDB_NAME
  props.push_back("PDB_NAME");

  for (std::vector<std::string>::const_iterator i = props.begin();
       i != props.end(); ++i) {
    if (const char* outName = this->GetProperty(*i)) {
      base = outName;
      break;
    }
  }
  return prefix + base + ".pdb";
}

std::string cmGeneratorTarget::GetObjectDirectory(
  std::string const& config) const
{
  std::string obj_dir =
    this->GlobalGenerator->ExpandCFGIntDir(this->ObjectDirectory, config);
#if defined(__APPLE__)
  // find and replace $(PROJECT_NAME) xcode placeholder
  const std::string projectName = this->LocalGenerator->GetProjectName();
  cmSystemTools::ReplaceString(obj_dir, "$(PROJECT_NAME)", projectName);
#endif
  return obj_dir;
}

void cmGeneratorTarget::GetTargetObjectNames(
  std::string const& config, std::vector<std::string>& objects) const
{
  std::vector<cmSourceFile const*> objectSources;
  this->GetObjectSources(objectSources, config);
  std::map<cmSourceFile const*, std::string> mapping;

  for (std::vector<cmSourceFile const*>::const_iterator it =
         objectSources.begin();
       it != objectSources.end(); ++it) {
    mapping[*it];
  }

  this->LocalGenerator->ComputeObjectFilenames(mapping, this);

  for (std::vector<cmSourceFile const*>::const_iterator it =
         objectSources.begin();
       it != objectSources.end(); ++it) {
    // Find the object file name corresponding to this source file.
    std::map<cmSourceFile const*, std::string>::const_iterator map_it =
      mapping.find(*it);
    // It must exist because we populated the mapping just above.
    assert(!map_it->second.empty());
    objects.push_back(map_it->second);
  }
}

bool cmGeneratorTarget::StrictTargetComparison::operator()(
  cmGeneratorTarget const* t1, cmGeneratorTarget const* t2) const
{
  int nameResult = strcmp(t1->GetName().c_str(), t2->GetName().c_str());
  if (nameResult == 0) {
    return strcmp(t1->GetLocalGenerator()->GetCurrentBinaryDirectory(),
                  t2->GetLocalGenerator()->GetCurrentBinaryDirectory()) < 0;
  }
  return nameResult < 0;
}

struct cmGeneratorTarget::SourceFileFlags
cmGeneratorTarget::GetTargetSourceFileFlags(const cmSourceFile* sf) const
{
  struct SourceFileFlags flags;
  this->ConstructSourceFileFlags();
  std::map<cmSourceFile const*, SourceFileFlags>::iterator si =
    this->SourceFlagsMap.find(sf);
  if (si != this->SourceFlagsMap.end()) {
    flags = si->second;
  } else {
    // Handle the MACOSX_PACKAGE_LOCATION property on source files that
    // were not listed in one of the other lists.
    if (const char* location = sf->GetProperty("MACOSX_PACKAGE_LOCATION")) {
      flags.MacFolder = location;
      const bool stripResources =
        this->GlobalGenerator->ShouldStripResourcePath(this->Makefile);
      if (strcmp(location, "Resources") == 0) {
        flags.Type = cmGeneratorTarget::SourceFileTypeResource;
        if (stripResources) {
          flags.MacFolder = "";
        }
      } else if (cmSystemTools::StringStartsWith(location, "Resources/")) {
        flags.Type = cmGeneratorTarget::SourceFileTypeDeepResource;
        if (stripResources) {
          flags.MacFolder += strlen("Resources/");
        }
      } else {
        flags.Type = cmGeneratorTarget::SourceFileTypeMacContent;
      }
    }
  }
  return flags;
}

void cmGeneratorTarget::ConstructSourceFileFlags() const
{
  if (this->SourceFileFlagsConstructed) {
    return;
  }
  this->SourceFileFlagsConstructed = true;

  // Process public headers to mark the source files.
  if (const char* files = this->GetProperty("PUBLIC_HEADER")) {
    std::vector<std::string> relFiles;
    cmSystemTools::ExpandListArgument(files, relFiles);
    for (std::vector<std::string>::iterator it = relFiles.begin();
         it != relFiles.end(); ++it) {
      if (cmSourceFile* sf = this->Makefile->GetSource(*it)) {
        SourceFileFlags& flags = this->SourceFlagsMap[sf];
        flags.MacFolder = "Headers";
        flags.Type = cmGeneratorTarget::SourceFileTypePublicHeader;
      }
    }
  }

  // Process private headers after public headers so that they take
  // precedence if a file is listed in both.
  if (const char* files = this->GetProperty("PRIVATE_HEADER")) {
    std::vector<std::string> relFiles;
    cmSystemTools::ExpandListArgument(files, relFiles);
    for (std::vector<std::string>::iterator it = relFiles.begin();
         it != relFiles.end(); ++it) {
      if (cmSourceFile* sf = this->Makefile->GetSource(*it)) {
        SourceFileFlags& flags = this->SourceFlagsMap[sf];
        flags.MacFolder = "PrivateHeaders";
        flags.Type = cmGeneratorTarget::SourceFileTypePrivateHeader;
      }
    }
  }

  // Mark sources listed as resources.
  if (const char* files = this->GetProperty("RESOURCE")) {
    std::vector<std::string> relFiles;
    cmSystemTools::ExpandListArgument(files, relFiles);
    for (std::vector<std::string>::iterator it = relFiles.begin();
         it != relFiles.end(); ++it) {
      if (cmSourceFile* sf = this->Makefile->GetSource(*it)) {
        SourceFileFlags& flags = this->SourceFlagsMap[sf];
        flags.MacFolder = "";
        if (!this->GlobalGenerator->ShouldStripResourcePath(this->Makefile)) {
          flags.MacFolder = "Resources";
        }
        flags.Type = cmGeneratorTarget::SourceFileTypeResource;
      }
    }
  }
}

const cmGeneratorTarget::CompatibleInterfacesBase&
cmGeneratorTarget::GetCompatibleInterfaces(std::string const& config) const
{
  cmGeneratorTarget::CompatibleInterfaces& compat =
    this->CompatibleInterfacesMap[config];
  if (!compat.Done) {
    compat.Done = true;
    compat.PropsBool.insert("POSITION_INDEPENDENT_CODE");
    compat.PropsString.insert("AUTOUIC_OPTIONS");
    std::vector<cmGeneratorTarget const*> const& deps =
      this->GetLinkImplementationClosure(config);
    for (std::vector<cmGeneratorTarget const*>::const_iterator li =
           deps.begin();
         li != deps.end(); ++li) {
#define CM_READ_COMPATIBLE_INTERFACE(X, x)                                    \
  if (const char* prop = (*li)->GetProperty("COMPATIBLE_INTERFACE_" #X)) {    \
    std::vector<std::string> props;                                           \
    cmSystemTools::ExpandListArgument(prop, props);                           \
    compat.Props##x.insert(props.begin(), props.end());                       \
  }
      CM_READ_COMPATIBLE_INTERFACE(BOOL, Bool)
      CM_READ_COMPATIBLE_INTERFACE(STRING, String)
      CM_READ_COMPATIBLE_INTERFACE(NUMBER_MIN, NumberMin)
      CM_READ_COMPATIBLE_INTERFACE(NUMBER_MAX, NumberMax)
#undef CM_READ_COMPATIBLE_INTERFACE
    }
  }
  return compat;
}

bool cmGeneratorTarget::IsLinkInterfaceDependentBoolProperty(
  const std::string& p, const std::string& config) const
{
  if (this->GetType() == cmStateEnums::OBJECT_LIBRARY ||
      this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return false;
  }
  return this->GetCompatibleInterfaces(config).PropsBool.count(p) > 0;
}

bool cmGeneratorTarget::IsLinkInterfaceDependentStringProperty(
  const std::string& p, const std::string& config) const
{
  if (this->GetType() == cmStateEnums::OBJECT_LIBRARY ||
      this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return false;
  }
  return this->GetCompatibleInterfaces(config).PropsString.count(p) > 0;
}

bool cmGeneratorTarget::IsLinkInterfaceDependentNumberMinProperty(
  const std::string& p, const std::string& config) const
{
  if (this->GetType() == cmStateEnums::OBJECT_LIBRARY ||
      this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return false;
  }
  return this->GetCompatibleInterfaces(config).PropsNumberMin.count(p) > 0;
}

bool cmGeneratorTarget::IsLinkInterfaceDependentNumberMaxProperty(
  const std::string& p, const std::string& config) const
{
  if (this->GetType() == cmStateEnums::OBJECT_LIBRARY ||
      this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return false;
  }
  return this->GetCompatibleInterfaces(config).PropsNumberMax.count(p) > 0;
}

enum CompatibleType
{
  BoolType,
  StringType,
  NumberMinType,
  NumberMaxType
};

template <typename PropertyType>
PropertyType getLinkInterfaceDependentProperty(cmGeneratorTarget const* tgt,
                                               const std::string& prop,
                                               const std::string& config,
                                               CompatibleType, PropertyType*);

template <>
bool getLinkInterfaceDependentProperty(cmGeneratorTarget const* tgt,
                                       const std::string& prop,
                                       const std::string& config,
                                       CompatibleType /*unused*/,
                                       bool* /*unused*/)
{
  return tgt->GetLinkInterfaceDependentBoolProperty(prop, config);
}

template <>
const char* getLinkInterfaceDependentProperty(cmGeneratorTarget const* tgt,
                                              const std::string& prop,
                                              const std::string& config,
                                              CompatibleType t,
                                              const char** /*unused*/)
{
  switch (t) {
    case BoolType:
      assert(false &&
             "String compatibility check function called for boolean");
      return CM_NULLPTR;
    case StringType:
      return tgt->GetLinkInterfaceDependentStringProperty(prop, config);
    case NumberMinType:
      return tgt->GetLinkInterfaceDependentNumberMinProperty(prop, config);
    case NumberMaxType:
      return tgt->GetLinkInterfaceDependentNumberMaxProperty(prop, config);
  }
  assert(false && "Unreachable!");
  return CM_NULLPTR;
}

template <typename PropertyType>
void checkPropertyConsistency(cmGeneratorTarget const* depender,
                              cmGeneratorTarget const* dependee,
                              const std::string& propName,
                              std::set<std::string>& emitted,
                              const std::string& config, CompatibleType t,
                              PropertyType* /*unused*/)
{
  const char* prop = dependee->GetProperty(propName);
  if (!prop) {
    return;
  }

  std::vector<std::string> props;
  cmSystemTools::ExpandListArgument(prop, props);
  std::string pdir = cmSystemTools::GetCMakeRoot();
  pdir += "/Help/prop_tgt/";

  for (std::vector<std::string>::iterator pi = props.begin();
       pi != props.end(); ++pi) {
    std::string pname = cmSystemTools::HelpFileName(*pi);
    std::string pfile = pdir + pname + ".rst";
    if (cmSystemTools::FileExists(pfile.c_str(), true)) {
      std::ostringstream e;
      e << "Target \"" << dependee->GetName() << "\" has property \"" << *pi
        << "\" listed in its " << propName
        << " property.  "
           "This is not allowed.  Only user-defined properties may appear "
           "listed in the "
        << propName << " property.";
      depender->GetLocalGenerator()->IssueMessage(cmake::FATAL_ERROR, e.str());
      return;
    }
    if (emitted.insert(*pi).second) {
      getLinkInterfaceDependentProperty<PropertyType>(depender, *pi, config, t,
                                                      CM_NULLPTR);
      if (cmSystemTools::GetErrorOccuredFlag()) {
        return;
      }
    }
  }
}

static std::string intersect(const std::set<std::string>& s1,
                             const std::set<std::string>& s2)
{
  std::set<std::string> intersect;
  std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                        std::inserter(intersect, intersect.begin()));
  if (!intersect.empty()) {
    return *intersect.begin();
  }
  return "";
}

static std::string intersect(const std::set<std::string>& s1,
                             const std::set<std::string>& s2,
                             const std::set<std::string>& s3)
{
  std::string result;
  result = intersect(s1, s2);
  if (!result.empty()) {
    return result;
  }
  result = intersect(s1, s3);
  if (!result.empty()) {
    return result;
  }
  return intersect(s2, s3);
}

static std::string intersect(const std::set<std::string>& s1,
                             const std::set<std::string>& s2,
                             const std::set<std::string>& s3,
                             const std::set<std::string>& s4)
{
  std::string result;
  result = intersect(s1, s2);
  if (!result.empty()) {
    return result;
  }
  result = intersect(s1, s3);
  if (!result.empty()) {
    return result;
  }
  result = intersect(s1, s4);
  if (!result.empty()) {
    return result;
  }
  return intersect(s2, s3, s4);
}

void cmGeneratorTarget::CheckPropertyCompatibility(
  cmComputeLinkInformation* info, const std::string& config) const
{
  const cmComputeLinkInformation::ItemVector& deps = info->GetItems();

  std::set<std::string> emittedBools;
  static std::string strBool = "COMPATIBLE_INTERFACE_BOOL";
  std::set<std::string> emittedStrings;
  static std::string strString = "COMPATIBLE_INTERFACE_STRING";
  std::set<std::string> emittedMinNumbers;
  static std::string strNumMin = "COMPATIBLE_INTERFACE_NUMBER_MIN";
  std::set<std::string> emittedMaxNumbers;
  static std::string strNumMax = "COMPATIBLE_INTERFACE_NUMBER_MAX";

  for (cmComputeLinkInformation::ItemVector::const_iterator li = deps.begin();
       li != deps.end(); ++li) {
    if (!li->Target) {
      continue;
    }

    checkPropertyConsistency<bool>(this, li->Target, strBool, emittedBools,
                                   config, BoolType, CM_NULLPTR);
    if (cmSystemTools::GetErrorOccuredFlag()) {
      return;
    }
    checkPropertyConsistency<const char*>(this, li->Target, strString,
                                          emittedStrings, config, StringType,
                                          CM_NULLPTR);
    if (cmSystemTools::GetErrorOccuredFlag()) {
      return;
    }
    checkPropertyConsistency<const char*>(this, li->Target, strNumMin,
                                          emittedMinNumbers, config,
                                          NumberMinType, CM_NULLPTR);
    if (cmSystemTools::GetErrorOccuredFlag()) {
      return;
    }
    checkPropertyConsistency<const char*>(this, li->Target, strNumMax,
                                          emittedMaxNumbers, config,
                                          NumberMaxType, CM_NULLPTR);
    if (cmSystemTools::GetErrorOccuredFlag()) {
      return;
    }
  }

  std::string prop = intersect(emittedBools, emittedStrings, emittedMinNumbers,
                               emittedMaxNumbers);

  if (!prop.empty()) {
    // Use a sorted std::vector to keep the error message sorted.
    std::vector<std::string> props;
    std::set<std::string>::const_iterator i = emittedBools.find(prop);
    if (i != emittedBools.end()) {
      props.push_back(strBool);
    }
    i = emittedStrings.find(prop);
    if (i != emittedStrings.end()) {
      props.push_back(strString);
    }
    i = emittedMinNumbers.find(prop);
    if (i != emittedMinNumbers.end()) {
      props.push_back(strNumMin);
    }
    i = emittedMaxNumbers.find(prop);
    if (i != emittedMaxNumbers.end()) {
      props.push_back(strNumMax);
    }
    std::sort(props.begin(), props.end());

    std::string propsString = cmJoin(cmMakeRange(props).retreat(1), ", ");
    propsString += " and the " + props.back();

    std::ostringstream e;
    e << "Property \"" << prop << "\" appears in both the " << propsString
      << " property in the dependencies of target \"" << this->GetName()
      << "\".  This is not allowed. A property may only require compatibility "
         "in a boolean interpretation, a numeric minimum, a numeric maximum "
         "or a "
         "string interpretation, but not a mixture.";
    this->LocalGenerator->IssueMessage(cmake::FATAL_ERROR, e.str());
  }
}

std::string compatibilityType(CompatibleType t)
{
  switch (t) {
    case BoolType:
      return "Boolean compatibility";
    case StringType:
      return "String compatibility";
    case NumberMaxType:
      return "Numeric maximum compatibility";
    case NumberMinType:
      return "Numeric minimum compatibility";
  }
  assert(false && "Unreachable!");
  return "";
}

std::string compatibilityAgree(CompatibleType t, bool dominant)
{
  switch (t) {
    case BoolType:
    case StringType:
      return dominant ? "(Disagree)\n" : "(Agree)\n";
    case NumberMaxType:
    case NumberMinType:
      return dominant ? "(Dominant)\n" : "(Ignored)\n";
  }
  assert(false && "Unreachable!");
  return "";
}

template <typename PropertyType>
PropertyType getTypedProperty(cmGeneratorTarget const* tgt,
                              const std::string& prop);

template <>
bool getTypedProperty<bool>(cmGeneratorTarget const* tgt,
                            const std::string& prop)
{
  return tgt->GetPropertyAsBool(prop);
}

template <>
const char* getTypedProperty<const char*>(cmGeneratorTarget const* tgt,
                                          const std::string& prop)
{
  return tgt->GetProperty(prop);
}

template <typename PropertyType>
std::string valueAsString(PropertyType);
template <>
std::string valueAsString<bool>(bool value)
{
  return value ? "TRUE" : "FALSE";
}
template <>
std::string valueAsString<const char*>(const char* value)
{
  return value ? value : "(unset)";
}

template <typename PropertyType>
PropertyType impliedValue(PropertyType);
template <>
bool impliedValue<bool>(bool /*unused*/)
{
  return false;
}
template <>
const char* impliedValue<const char*>(const char* /*unused*/)
{
  return "";
}

template <typename PropertyType>
std::pair<bool, PropertyType> consistentProperty(PropertyType lhs,
                                                 PropertyType rhs,
                                                 CompatibleType t);

template <>
std::pair<bool, bool> consistentProperty(bool lhs, bool rhs,
                                         CompatibleType /*unused*/)
{
  return std::make_pair(lhs == rhs, lhs);
}

std::pair<bool, const char*> consistentStringProperty(const char* lhs,
                                                      const char* rhs)
{
  const bool b = strcmp(lhs, rhs) == 0;
  return std::make_pair(b, b ? lhs : CM_NULLPTR);
}

std::pair<bool, const char*> consistentNumberProperty(const char* lhs,
                                                      const char* rhs,
                                                      CompatibleType t)
{
  char* pEnd;

  const char* const null_ptr = CM_NULLPTR;

  long lnum = strtol(lhs, &pEnd, 0);
  if (pEnd == lhs || *pEnd != '\0' || errno == ERANGE) {
    return std::pair<bool, const char*>(false, null_ptr);
  }

  long rnum = strtol(rhs, &pEnd, 0);
  if (pEnd == rhs || *pEnd != '\0' || errno == ERANGE) {
    return std::pair<bool, const char*>(false, null_ptr);
  }

  if (t == NumberMaxType) {
    return std::make_pair(true, std::max(lnum, rnum) == lnum ? lhs : rhs);
  }
  return std::make_pair(true, std::min(lnum, rnum) == lnum ? lhs : rhs);
}

template <>
std::pair<bool, const char*> consistentProperty(const char* lhs,
                                                const char* rhs,
                                                CompatibleType t)
{
  if (!lhs && !rhs) {
    return std::make_pair(true, lhs);
  }
  if (!lhs) {
    return std::make_pair(true, rhs);
  }
  if (!rhs) {
    return std::make_pair(true, lhs);
  }

  const char* const null_ptr = CM_NULLPTR;

  switch (t) {
    case BoolType:
      assert(false && "consistentProperty for strings called with BoolType");
      return std::pair<bool, const char*>(false, null_ptr);
    case StringType:
      return consistentStringProperty(lhs, rhs);
    case NumberMinType:
    case NumberMaxType:
      return consistentNumberProperty(lhs, rhs, t);
  }
  assert(false && "Unreachable!");
  return std::pair<bool, const char*>(false, null_ptr);
}

template <typename PropertyType>
PropertyType checkInterfacePropertyCompatibility(cmGeneratorTarget const* tgt,
                                                 const std::string& p,
                                                 const std::string& config,
                                                 const char* defaultValue,
                                                 CompatibleType t,
                                                 PropertyType* /*unused*/)
{
  PropertyType propContent = getTypedProperty<PropertyType>(tgt, p);
  std::vector<std::string> headPropKeys = tgt->GetPropertyKeys();
  const bool explicitlySet =
    std::find(headPropKeys.begin(), headPropKeys.end(), p) !=
    headPropKeys.end();

  const bool impliedByUse = tgt->IsNullImpliedByLinkLibraries(p);
  assert((impliedByUse ^ explicitlySet) || (!impliedByUse && !explicitlySet));

  std::vector<cmGeneratorTarget const*> const& deps =
    tgt->GetLinkImplementationClosure(config);

  if (deps.empty()) {
    return propContent;
  }
  bool propInitialized = explicitlySet;

  std::string report = " * Target \"";
  report += tgt->GetName();
  if (explicitlySet) {
    report += "\" has property content \"";
    report += valueAsString<PropertyType>(propContent);
    report += "\"\n";
  } else if (impliedByUse) {
    report += "\" property is implied by use.\n";
  } else {
    report += "\" property not set.\n";
  }

  std::string interfaceProperty = "INTERFACE_" + p;
  for (std::vector<cmGeneratorTarget const*>::const_iterator li = deps.begin();
       li != deps.end(); ++li) {
    // An error should be reported if one dependency
    // has INTERFACE_POSITION_INDEPENDENT_CODE ON and the other
    // has INTERFACE_POSITION_INDEPENDENT_CODE OFF, or if the
    // target itself has a POSITION_INDEPENDENT_CODE which disagrees
    // with a dependency.

    cmGeneratorTarget const* theTarget = *li;

    std::vector<std::string> propKeys = theTarget->GetPropertyKeys();

    const bool ifaceIsSet = std::find(propKeys.begin(), propKeys.end(),
                                      interfaceProperty) != propKeys.end();
    PropertyType ifacePropContent =
      getTypedProperty<PropertyType>(theTarget, interfaceProperty);

    std::string reportEntry;
    if (ifaceIsSet) {
      reportEntry += " * Target \"";
      reportEntry += theTarget->GetName();
      reportEntry += "\" property value \"";
      reportEntry += valueAsString<PropertyType>(ifacePropContent);
      reportEntry += "\" ";
    }

    if (explicitlySet) {
      if (ifaceIsSet) {
        std::pair<bool, PropertyType> consistent =
          consistentProperty(propContent, ifacePropContent, t);
        report += reportEntry;
        report += compatibilityAgree(t, propContent != consistent.second);
        if (!consistent.first) {
          std::ostringstream e;
          e << "Property " << p << " on target \"" << tgt->GetName()
            << "\" does\nnot match the "
               "INTERFACE_"
            << p << " property requirement\nof "
                    "dependency \""
            << theTarget->GetName() << "\".\n";
          cmSystemTools::Error(e.str().c_str());
          break;
        }
        propContent = consistent.second;
        continue;
      }
      // Explicitly set on target and not set in iface. Can't disagree.
      continue;
    }
    if (impliedByUse) {
      propContent = impliedValue<PropertyType>(propContent);

      if (ifaceIsSet) {
        std::pair<bool, PropertyType> consistent =
          consistentProperty(propContent, ifacePropContent, t);
        report += reportEntry;
        report += compatibilityAgree(t, propContent != consistent.second);
        if (!consistent.first) {
          std::ostringstream e;
          e << "Property " << p << " on target \"" << tgt->GetName()
            << "\" is\nimplied to be " << defaultValue
            << " because it was used to determine the link libraries\n"
               "already. The INTERFACE_"
            << p << " property on\ndependency \"" << theTarget->GetName()
            << "\" is in conflict.\n";
          cmSystemTools::Error(e.str().c_str());
          break;
        }
        propContent = consistent.second;
        continue;
      }
      // Implicitly set on target and not set in iface. Can't disagree.
      continue;
    }
    if (ifaceIsSet) {
      if (propInitialized) {
        std::pair<bool, PropertyType> consistent =
          consistentProperty(propContent, ifacePropContent, t);
        report += reportEntry;
        report += compatibilityAgree(t, propContent != consistent.second);
        if (!consistent.first) {
          std::ostringstream e;
          e << "The INTERFACE_" << p << " property of \""
            << theTarget->GetName() << "\" does\nnot agree with the value of "
            << p << " already determined\nfor \"" << tgt->GetName() << "\".\n";
          cmSystemTools::Error(e.str().c_str());
          break;
        }
        propContent = consistent.second;
        continue;
      }
      report += reportEntry + "(Interface set)\n";
      propContent = ifacePropContent;
      propInitialized = true;
    } else {
      // Not set. Nothing to agree on.
      continue;
    }
  }

  tgt->ReportPropertyOrigin(p, valueAsString<PropertyType>(propContent),
                            report, compatibilityType(t));
  return propContent;
}

bool cmGeneratorTarget::GetLinkInterfaceDependentBoolProperty(
  const std::string& p, const std::string& config) const
{
  return checkInterfacePropertyCompatibility<bool>(this, p, config, "FALSE",
                                                   BoolType, CM_NULLPTR);
}

const char* cmGeneratorTarget::GetLinkInterfaceDependentStringProperty(
  const std::string& p, const std::string& config) const
{
  return checkInterfacePropertyCompatibility<const char*>(
    this, p, config, "empty", StringType, CM_NULLPTR);
}

const char* cmGeneratorTarget::GetLinkInterfaceDependentNumberMinProperty(
  const std::string& p, const std::string& config) const
{
  return checkInterfacePropertyCompatibility<const char*>(
    this, p, config, "empty", NumberMinType, CM_NULLPTR);
}

const char* cmGeneratorTarget::GetLinkInterfaceDependentNumberMaxProperty(
  const std::string& p, const std::string& config) const
{
  return checkInterfacePropertyCompatibility<const char*>(
    this, p, config, "empty", NumberMaxType, CM_NULLPTR);
}

cmComputeLinkInformation* cmGeneratorTarget::GetLinkInformation(
  const std::string& config) const
{
  // Lookup any existing information for this configuration.
  std::string key(cmSystemTools::UpperCase(config));
  cmTargetLinkInformationMap::iterator i = this->LinkInformation.find(key);
  if (i == this->LinkInformation.end()) {
    // Compute information for this configuration.
    cmComputeLinkInformation* info =
      new cmComputeLinkInformation(this, config);
    if (!info || !info->Compute()) {
      delete info;
      info = CM_NULLPTR;
    }

    // Store the information for this configuration.
    cmTargetLinkInformationMap::value_type entry(key, info);
    i = this->LinkInformation.insert(entry).first;

    if (info) {
      this->CheckPropertyCompatibility(info, config);
    }
  }
  return i->second;
}

void cmGeneratorTarget::GetTargetVersion(int& major, int& minor) const
{
  int patch;
  this->GetTargetVersion(false, major, minor, patch);
}

void cmGeneratorTarget::GetTargetVersion(bool soversion, int& major,
                                         int& minor, int& patch) const
{
  // Set the default values.
  major = 0;
  minor = 0;
  patch = 0;

  assert(this->GetType() != cmStateEnums::INTERFACE_LIBRARY);

  // Look for a VERSION or SOVERSION property.
  const char* prop = soversion ? "SOVERSION" : "VERSION";
  if (const char* version = this->GetProperty(prop)) {
    // Try to parse the version number and store the results that were
    // successfully parsed.
    int parsed_major;
    int parsed_minor;
    int parsed_patch;
    switch (sscanf(version, "%d.%d.%d", &parsed_major, &parsed_minor,
                   &parsed_patch)) {
      case 3:
        patch = parsed_patch;
        CM_FALLTHROUGH;
      case 2:
        minor = parsed_minor;
        CM_FALLTHROUGH;
      case 1:
        major = parsed_major;
        CM_FALLTHROUGH;
      default:
        break;
    }
  }
}

std::string cmGeneratorTarget::GetFortranModuleDirectory(
  std::string const& working_dir) const
{
  if (!this->FortranModuleDirectoryCreated) {
    this->FortranModuleDirectory =
      this->CreateFortranModuleDirectory(working_dir);
    this->FortranModuleDirectoryCreated = true;
  }

  return this->FortranModuleDirectory;
}

std::string cmGeneratorTarget::CreateFortranModuleDirectory(
  std::string const& working_dir) const
{
  std::string mod_dir;
  std::string target_mod_dir;
  if (const char* prop = this->GetProperty("Fortran_MODULE_DIRECTORY")) {
    target_mod_dir = prop;
  } else {
    std::string const& default_mod_dir =
      this->LocalGenerator->GetCurrentBinaryDirectory();
    if (default_mod_dir != working_dir) {
      target_mod_dir = default_mod_dir;
    }
  }
  const char* moddir_flag =
    this->Makefile->GetDefinition("CMAKE_Fortran_MODDIR_FLAG");
  if (!target_mod_dir.empty() && moddir_flag) {
    // Compute the full path to the module directory.
    if (cmSystemTools::FileIsFullPath(target_mod_dir)) {
      // Already a full path.
      mod_dir = target_mod_dir;
    } else {
      // Interpret relative to the current output directory.
      mod_dir = this->LocalGenerator->GetCurrentBinaryDirectory();
      mod_dir += "/";
      mod_dir += target_mod_dir;
    }

    // Make sure the module output directory exists.
    cmSystemTools::MakeDirectory(mod_dir);
  }
  return mod_dir;
}

std::string cmGeneratorTarget::GetFrameworkVersion() const
{
  assert(this->GetType() != cmStateEnums::INTERFACE_LIBRARY);

  if (const char* fversion = this->GetProperty("FRAMEWORK_VERSION")) {
    return fversion;
  }
  if (const char* tversion = this->GetProperty("VERSION")) {
    return tversion;
  }
  return "A";
}

void cmGeneratorTarget::ComputeVersionedName(std::string& vName,
                                             std::string const& prefix,
                                             std::string const& base,
                                             std::string const& suffix,
                                             std::string const& name,
                                             const char* version) const
{
  vName = this->Makefile->IsOn("APPLE") ? (prefix + base) : name;
  if (version) {
    vName += ".";
    vName += version;
  }
  vName += this->Makefile->IsOn("APPLE") ? suffix : std::string();
}

std::vector<std::string> cmGeneratorTarget::GetPropertyKeys() const
{
  cmPropertyMap const& propsObject = this->Target->GetProperties();
  std::vector<std::string> props;
  props.reserve(propsObject.size());
  for (cmPropertyMap::const_iterator it = propsObject.begin();
       it != propsObject.end(); ++it) {
    props.push_back(it->first);
  }
  return props;
}

void cmGeneratorTarget::ReportPropertyOrigin(
  const std::string& p, const std::string& result, const std::string& report,
  const std::string& compatibilityType) const
{
  std::vector<std::string> debugProperties;
  const char* debugProp = this->Target->GetMakefile()->GetDefinition(
    "CMAKE_DEBUG_TARGET_PROPERTIES");
  if (debugProp) {
    cmSystemTools::ExpandListArgument(debugProp, debugProperties);
  }

  bool debugOrigin = !this->DebugCompatiblePropertiesDone[p] &&
    std::find(debugProperties.begin(), debugProperties.end(), p) !=
      debugProperties.end();

  if (this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    this->DebugCompatiblePropertiesDone[p] = true;
  }
  if (!debugOrigin) {
    return;
  }

  std::string areport = compatibilityType;
  areport += std::string(" of property \"") + p + "\" for target \"";
  areport += std::string(this->GetName());
  areport += "\" (result: \"";
  areport += result;
  areport += "\"):\n" + report;

  this->LocalGenerator->GetCMakeInstance()->IssueMessage(cmake::LOG, areport);
}

void cmGeneratorTarget::LookupLinkItems(std::vector<std::string> const& names,
                                        std::vector<cmLinkItem>& items) const
{
  for (std::vector<std::string>::const_iterator i = names.begin();
       i != names.end(); ++i) {
    std::string name = this->CheckCMP0004(*i);
    if (name == this->GetName() || name.empty()) {
      continue;
    }
    items.push_back(cmLinkItem(name, this->FindTargetToLink(name)));
  }
}

void cmGeneratorTarget::ExpandLinkItems(
  std::string const& prop, std::string const& value, std::string const& config,
  cmGeneratorTarget const* headTarget, bool usage_requirements_only,
  std::vector<cmLinkItem>& items, bool& hadHeadSensitiveCondition) const
{
  cmGeneratorExpression ge;
  cmGeneratorExpressionDAGChecker dagChecker(this->GetName(), prop, CM_NULLPTR,
                                             CM_NULLPTR);
  // The $<LINK_ONLY> expression may be in a link interface to specify private
  // link dependencies that are otherwise excluded from usage requirements.
  if (usage_requirements_only) {
    dagChecker.SetTransitivePropertiesOnly();
  }
  std::vector<std::string> libs;
  CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(value);
  cmSystemTools::ExpandListArgument(cge->Evaluate(this->LocalGenerator, config,
                                                  false, headTarget, this,
                                                  &dagChecker),
                                    libs);
  this->LookupLinkItems(libs, items);
  hadHeadSensitiveCondition = cge->GetHadHeadSensitiveCondition();
}

cmLinkInterface const* cmGeneratorTarget::GetLinkInterface(
  const std::string& config, cmGeneratorTarget const* head) const
{
  // Imported targets have their own link interface.
  if (this->IsImported()) {
    return this->GetImportLinkInterface(config, head, false);
  }

  // Link interfaces are not supported for executables that do not
  // export symbols.
  if (this->GetType() == cmStateEnums::EXECUTABLE &&
      !this->IsExecutableWithExports()) {
    return CM_NULLPTR;
  }

  // Lookup any existing link interface for this configuration.
  cmHeadToLinkInterfaceMap& hm = this->GetHeadToLinkInterfaceMap(config);

  // If the link interface does not depend on the head target
  // then return the one we computed first.
  if (!hm.empty() && !hm.begin()->second.HadHeadSensitiveCondition) {
    return &hm.begin()->second;
  }

  cmOptionalLinkInterface& iface = hm[head];
  if (!iface.LibrariesDone) {
    iface.LibrariesDone = true;
    this->ComputeLinkInterfaceLibraries(config, iface, head, false);
  }
  if (!iface.AllDone) {
    iface.AllDone = true;
    if (iface.Exists) {
      this->ComputeLinkInterface(config, iface, head);
    }
  }

  return iface.Exists ? &iface : CM_NULLPTR;
}

void cmGeneratorTarget::ComputeLinkInterface(
  const std::string& config, cmOptionalLinkInterface& iface,
  cmGeneratorTarget const* headTarget) const
{
  if (iface.ExplicitLibraries) {
    if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
        this->GetType() == cmStateEnums::STATIC_LIBRARY ||
        this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      // Shared libraries may have runtime implementation dependencies
      // on other shared libraries that are not in the interface.
      CM_UNORDERED_SET<std::string> emitted;
      for (std::vector<cmLinkItem>::const_iterator li =
             iface.Libraries.begin();
           li != iface.Libraries.end(); ++li) {
        emitted.insert(*li);
      }
      if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
        cmLinkImplementation const* impl = this->GetLinkImplementation(config);
        for (std::vector<cmLinkImplItem>::const_iterator li =
               impl->Libraries.begin();
             li != impl->Libraries.end(); ++li) {
          if (emitted.insert(*li).second) {
            if (li->Target) {
              // This is a runtime dependency on another shared library.
              if (li->Target->GetType() == cmStateEnums::SHARED_LIBRARY) {
                iface.SharedDeps.push_back(*li);
              }
            } else {
              // TODO: Recognize shared library file names.  Perhaps this
              // should be moved to cmComputeLinkInformation, but that creates
              // a chicken-and-egg problem since this list is needed for its
              // construction.
            }
          }
        }
      }
    }
  } else if (this->GetPolicyStatusCMP0022() == cmPolicies::WARN ||
             this->GetPolicyStatusCMP0022() == cmPolicies::OLD) {
    // The link implementation is the default link interface.
    cmLinkImplementationLibraries const* impl =
      this->GetLinkImplementationLibrariesInternal(config, headTarget);
    iface.ImplementationIsInterface = true;
    iface.WrongConfigLibraries = impl->WrongConfigLibraries;
  }

  if (this->LinkLanguagePropagatesToDependents()) {
    // Targets using this archive need its language runtime libraries.
    if (cmLinkImplementation const* impl =
          this->GetLinkImplementation(config)) {
      iface.Languages = impl->Languages;
    }
  }

  if (this->GetType() == cmStateEnums::STATIC_LIBRARY) {
    // Construct the property name suffix for this configuration.
    std::string suffix = "_";
    if (!config.empty()) {
      suffix += cmSystemTools::UpperCase(config);
    } else {
      suffix += "NOCONFIG";
    }

    // How many repetitions are needed if this library has cyclic
    // dependencies?
    std::string propName = "LINK_INTERFACE_MULTIPLICITY";
    propName += suffix;
    if (const char* config_reps = this->GetProperty(propName)) {
      sscanf(config_reps, "%u", &iface.Multiplicity);
    } else if (const char* reps =
                 this->GetProperty("LINK_INTERFACE_MULTIPLICITY")) {
      sscanf(reps, "%u", &iface.Multiplicity);
    }
  }
}

const cmLinkInterfaceLibraries* cmGeneratorTarget::GetLinkInterfaceLibraries(
  const std::string& config, cmGeneratorTarget const* head,
  bool usage_requirements_only) const
{
  // Imported targets have their own link interface.
  if (this->IsImported()) {
    return this->GetImportLinkInterface(config, head, usage_requirements_only);
  }

  // Link interfaces are not supported for executables that do not
  // export symbols.
  if (this->GetType() == cmStateEnums::EXECUTABLE &&
      !this->IsExecutableWithExports()) {
    return CM_NULLPTR;
  }

  // Lookup any existing link interface for this configuration.
  std::string CONFIG = cmSystemTools::UpperCase(config);
  cmHeadToLinkInterfaceMap& hm =
    (usage_requirements_only
       ? this->GetHeadToLinkInterfaceUsageRequirementsMap(config)
       : this->GetHeadToLinkInterfaceMap(config));

  // If the link interface does not depend on the head target
  // then return the one we computed first.
  if (!hm.empty() && !hm.begin()->second.HadHeadSensitiveCondition) {
    return &hm.begin()->second;
  }

  cmOptionalLinkInterface& iface = hm[head];
  if (!iface.LibrariesDone) {
    iface.LibrariesDone = true;
    this->ComputeLinkInterfaceLibraries(config, iface, head,
                                        usage_requirements_only);
  }

  return iface.Exists ? &iface : CM_NULLPTR;
}

std::string cmGeneratorTarget::GetDirectory(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  if (this->IsImported()) {
    // Return the directory from which the target is imported.
    return cmSystemTools::GetFilenamePath(
      this->Target->ImportedGetFullPath(config, artifact));
  }
  if (OutputInfo const* info = this->GetOutputInfo(config)) {
    // Return the directory in which the target will be built.
    switch (artifact) {
      case cmStateEnums::RuntimeBinaryArtifact:
        return info->OutDir;
      case cmStateEnums::ImportLibraryArtifact:
        return info->ImpDir;
    }
  }
  return "";
}

bool cmGeneratorTarget::UsesDefaultOutputDir(
  const std::string& config, cmStateEnums::ArtifactType artifact) const
{
  std::string dir;
  return this->ComputeOutputDir(config, artifact, dir);
}

cmGeneratorTarget::OutputInfo const* cmGeneratorTarget::GetOutputInfo(
  const std::string& config) const
{
  // There is no output information for imported targets.
  if (this->IsImported()) {
    return CM_NULLPTR;
  }

  // Only libraries and executables have well-defined output files.
  if (!this->HaveWellDefinedOutputFiles()) {
    std::string msg = "cmGeneratorTarget::GetOutputInfo called for ";
    msg += this->GetName();
    msg += " which has type ";
    msg += cmState::GetTargetTypeName(this->GetType());
    this->LocalGenerator->IssueMessage(cmake::INTERNAL_ERROR, msg);
    return CM_NULLPTR;
  }

  // Lookup/compute/cache the output information for this configuration.
  std::string config_upper;
  if (!config.empty()) {
    config_upper = cmSystemTools::UpperCase(config);
  }
  OutputInfoMapType::iterator i = this->OutputInfoMap.find(config_upper);
  if (i == this->OutputInfoMap.end()) {
    // Add empty info in map to detect potential recursion.
    OutputInfo info;
    OutputInfoMapType::value_type entry(config_upper, info);
    i = this->OutputInfoMap.insert(entry).first;

    // Compute output directories.
    this->ComputeOutputDir(config, cmStateEnums::RuntimeBinaryArtifact,
                           info.OutDir);
    this->ComputeOutputDir(config, cmStateEnums::ImportLibraryArtifact,
                           info.ImpDir);
    if (!this->ComputePDBOutputDir("PDB", config, info.PdbDir)) {
      info.PdbDir = info.OutDir;
    }

    // Now update the previously-prepared map entry.
    i->second = info;
  } else if (i->second.empty()) {
    // An empty map entry indicates we have been called recursively
    // from the above block.
    this->LocalGenerator->GetCMakeInstance()->IssueMessage(
      cmake::FATAL_ERROR,
      "Target '" + this->GetName() + "' OUTPUT_DIRECTORY depends on itself.",
      this->GetBacktrace());
    return CM_NULLPTR;
  }
  return &i->second;
}

bool cmGeneratorTarget::ComputeOutputDir(const std::string& config,
                                         cmStateEnums::ArtifactType artifact,
                                         std::string& out) const
{
  bool usesDefaultOutputDir = false;
  std::string conf = config;

  // Look for a target property defining the target output directory
  // based on the target type.
  std::string targetTypeName = this->GetOutputTargetType(artifact);
  const char* propertyName = CM_NULLPTR;
  std::string propertyNameStr = targetTypeName;
  if (!propertyNameStr.empty()) {
    propertyNameStr += "_OUTPUT_DIRECTORY";
    propertyName = propertyNameStr.c_str();
  }

  // Check for a per-configuration output directory target property.
  std::string configUpper = cmSystemTools::UpperCase(conf);
  const char* configProp = CM_NULLPTR;
  std::string configPropStr = targetTypeName;
  if (!configPropStr.empty()) {
    configPropStr += "_OUTPUT_DIRECTORY_";
    configPropStr += configUpper;
    configProp = configPropStr.c_str();
  }

  // Select an output directory.
  if (const char* config_outdir = this->GetProperty(configProp)) {
    // Use the user-specified per-configuration output directory.
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(config_outdir);
    out = cge->Evaluate(this->LocalGenerator, config);

    // Skip per-configuration subdirectory.
    conf = "";
  } else if (const char* outdir = this->GetProperty(propertyName)) {
    // Use the user-specified output directory.
    cmGeneratorExpression ge;
    CM_AUTO_PTR<cmCompiledGeneratorExpression> cge = ge.Parse(outdir);
    out = cge->Evaluate(this->LocalGenerator, config);

    // Skip per-configuration subdirectory if the value contained a
    // generator expression.
    if (out != outdir) {
      conf = "";
    }
  } else if (this->GetType() == cmStateEnums::EXECUTABLE) {
    // Lookup the output path for executables.
    out = this->Makefile->GetSafeDefinition("EXECUTABLE_OUTPUT_PATH");
  } else if (this->GetType() == cmStateEnums::STATIC_LIBRARY ||
             this->GetType() == cmStateEnums::SHARED_LIBRARY ||
             this->GetType() == cmStateEnums::MODULE_LIBRARY) {
    // Lookup the output path for libraries.
    out = this->Makefile->GetSafeDefinition("LIBRARY_OUTPUT_PATH");
  }
  if (out.empty()) {
    // Default to the current output directory.
    usesDefaultOutputDir = true;
    out = ".";
  }

  // Convert the output path to a full path in case it is
  // specified as a relative path.  Treat a relative path as
  // relative to the current output directory for this makefile.
  out = (cmSystemTools::CollapseFullPath(
    out, this->LocalGenerator->GetCurrentBinaryDirectory()));

  // The generator may add the configuration's subdirectory.
  if (!conf.empty()) {
    bool useEPN =
      this->GlobalGenerator->UseEffectivePlatformName(this->Makefile);
    std::string suffix =
      usesDefaultOutputDir && useEPN ? "${EFFECTIVE_PLATFORM_NAME}" : "";
    this->LocalGenerator->GetGlobalGenerator()->AppendDirectoryForConfig(
      "/", conf, suffix, out);
  }

  return usesDefaultOutputDir;
}

bool cmGeneratorTarget::ComputePDBOutputDir(const std::string& kind,
                                            const std::string& config,
                                            std::string& out) const
{
  // Look for a target property defining the target output directory
  // based on the target type.
  const char* propertyName = CM_NULLPTR;
  std::string propertyNameStr = kind;
  if (!propertyNameStr.empty()) {
    propertyNameStr += "_OUTPUT_DIRECTORY";
    propertyName = propertyNameStr.c_str();
  }
  std::string conf = config;

  // Check for a per-configuration output directory target property.
  std::string configUpper = cmSystemTools::UpperCase(conf);
  const char* configProp = CM_NULLPTR;
  std::string configPropStr = kind;
  if (!configPropStr.empty()) {
    configPropStr += "_OUTPUT_DIRECTORY_";
    configPropStr += configUpper;
    configProp = configPropStr.c_str();
  }

  // Select an output directory.
  if (const char* config_outdir = this->GetProperty(configProp)) {
    // Use the user-specified per-configuration output directory.
    out = config_outdir;

    // Skip per-configuration subdirectory.
    conf = "";
  } else if (const char* outdir = this->GetProperty(propertyName)) {
    // Use the user-specified output directory.
    out = outdir;
  }
  if (out.empty()) {
    return false;
  }

  // Convert the output path to a full path in case it is
  // specified as a relative path.  Treat a relative path as
  // relative to the current output directory for this makefile.
  out = (cmSystemTools::CollapseFullPath(
    out, this->LocalGenerator->GetCurrentBinaryDirectory()));

  // The generator may add the configuration's subdirectory.
  if (!conf.empty()) {
    this->LocalGenerator->GetGlobalGenerator()->AppendDirectoryForConfig(
      "/", conf, "", out);
  }
  return true;
}

bool cmGeneratorTarget::HaveInstallTreeRPATH() const
{
  const char* install_rpath = this->GetProperty("INSTALL_RPATH");
  return (install_rpath && *install_rpath) &&
    !this->Makefile->IsOn("CMAKE_SKIP_INSTALL_RPATH");
}

void cmGeneratorTarget::ComputeLinkInterfaceLibraries(
  const std::string& config, cmOptionalLinkInterface& iface,
  cmGeneratorTarget const* headTarget, bool usage_requirements_only) const
{
  // Construct the property name suffix for this configuration.
  std::string suffix = "_";
  if (!config.empty()) {
    suffix += cmSystemTools::UpperCase(config);
  } else {
    suffix += "NOCONFIG";
  }

  // An explicit list of interface libraries may be set for shared
  // libraries and executables that export symbols.
  const char* explicitLibraries = CM_NULLPTR;
  std::string linkIfaceProp;
  if (this->GetPolicyStatusCMP0022() != cmPolicies::OLD &&
      this->GetPolicyStatusCMP0022() != cmPolicies::WARN) {
    // CMP0022 NEW behavior is to use INTERFACE_LINK_LIBRARIES.
    linkIfaceProp = "INTERFACE_LINK_LIBRARIES";
    explicitLibraries = this->GetProperty(linkIfaceProp);
  } else if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
             this->IsExecutableWithExports()) {
    // CMP0022 OLD behavior is to use LINK_INTERFACE_LIBRARIES if set on a
    // shared lib or executable.

    // Lookup the per-configuration property.
    linkIfaceProp = "LINK_INTERFACE_LIBRARIES";
    linkIfaceProp += suffix;
    explicitLibraries = this->GetProperty(linkIfaceProp);

    // If not set, try the generic property.
    if (!explicitLibraries) {
      linkIfaceProp = "LINK_INTERFACE_LIBRARIES";
      explicitLibraries = this->GetProperty(linkIfaceProp);
    }
  }

  if (explicitLibraries &&
      this->GetPolicyStatusCMP0022() == cmPolicies::WARN &&
      !this->PolicyWarnedCMP0022) {
    // Compare the explicitly set old link interface properties to the
    // preferred new link interface property one and warn if different.
    const char* newExplicitLibraries =
      this->GetProperty("INTERFACE_LINK_LIBRARIES");
    if (newExplicitLibraries &&
        strcmp(newExplicitLibraries, explicitLibraries) != 0) {
      std::ostringstream w;
      /* clang-format off */
      w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0022) << "\n"
        "Target \"" << this->GetName() << "\" has an "
        "INTERFACE_LINK_LIBRARIES property which differs from its " <<
        linkIfaceProp << " properties."
        "\n"
        "INTERFACE_LINK_LIBRARIES:\n"
        "  " << newExplicitLibraries << "\n" <<
        linkIfaceProp << ":\n"
        "  " << (explicitLibraries ? explicitLibraries : "(empty)") << "\n";
      /* clang-format on */
      this->LocalGenerator->IssueMessage(cmake::AUTHOR_WARNING, w.str());
      this->PolicyWarnedCMP0022 = true;
    }
  }

  // There is no implicit link interface for executables or modules
  // so if none was explicitly set then there is no link interface.
  if (!explicitLibraries &&
      (this->GetType() == cmStateEnums::EXECUTABLE ||
       (this->GetType() == cmStateEnums::MODULE_LIBRARY))) {
    return;
  }
  iface.Exists = true;
  iface.ExplicitLibraries = explicitLibraries;

  if (explicitLibraries) {
    // The interface libraries have been explicitly set.
    this->ExpandLinkItems(linkIfaceProp, explicitLibraries, config, headTarget,
                          usage_requirements_only, iface.Libraries,
                          iface.HadHeadSensitiveCondition);
  } else if (this->GetPolicyStatusCMP0022() == cmPolicies::WARN ||
             this->GetPolicyStatusCMP0022() == cmPolicies::OLD)
  // If CMP0022 is NEW then the plain tll signature sets the
  // INTERFACE_LINK_LIBRARIES, so if we get here then the project
  // cleared the property explicitly and we should not fall back
  // to the link implementation.
  {
    // The link implementation is the default link interface.
    cmLinkImplementationLibraries const* impl =
      this->GetLinkImplementationLibrariesInternal(config, headTarget);
    iface.Libraries.insert(iface.Libraries.end(), impl->Libraries.begin(),
                           impl->Libraries.end());
    if (this->GetPolicyStatusCMP0022() == cmPolicies::WARN &&
        !this->PolicyWarnedCMP0022 && !usage_requirements_only) {
      // Compare the link implementation fallback link interface to the
      // preferred new link interface property and warn if different.
      std::vector<cmLinkItem> ifaceLibs;
      static const std::string newProp = "INTERFACE_LINK_LIBRARIES";
      if (const char* newExplicitLibraries = this->GetProperty(newProp)) {
        bool hadHeadSensitiveConditionDummy = false;
        this->ExpandLinkItems(newProp, newExplicitLibraries, config,
                              headTarget, usage_requirements_only, ifaceLibs,
                              hadHeadSensitiveConditionDummy);
      }
      if (ifaceLibs != iface.Libraries) {
        std::string oldLibraries = cmJoin(impl->Libraries, ";");
        std::string newLibraries = cmJoin(ifaceLibs, ";");
        if (oldLibraries.empty()) {
          oldLibraries = "(empty)";
        }
        if (newLibraries.empty()) {
          newLibraries = "(empty)";
        }

        std::ostringstream w;
        /* clang-format off */
        w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0022) << "\n"
          "Target \"" << this->GetName() << "\" has an "
          "INTERFACE_LINK_LIBRARIES property.  "
          "This should be preferred as the source of the link interface "
          "for this library but because CMP0022 is not set CMake is "
          "ignoring the property and using the link implementation "
          "as the link interface instead."
          "\n"
          "INTERFACE_LINK_LIBRARIES:\n"
          "  " << newLibraries << "\n"
          "Link implementation:\n"
          "  " << oldLibraries << "\n";
        /* clang-format on */
        this->LocalGenerator->IssueMessage(cmake::AUTHOR_WARNING, w.str());
        this->PolicyWarnedCMP0022 = true;
      }
    }
  }
}

const cmLinkInterface* cmGeneratorTarget::GetImportLinkInterface(
  const std::string& config, cmGeneratorTarget const* headTarget,
  bool usage_requirements_only) const
{
  cmGeneratorTarget::ImportInfo const* info = this->GetImportInfo(config);
  if (!info) {
    return CM_NULLPTR;
  }

  std::string CONFIG = cmSystemTools::UpperCase(config);
  cmHeadToLinkInterfaceMap& hm =
    (usage_requirements_only
       ? this->GetHeadToLinkInterfaceUsageRequirementsMap(config)
       : this->GetHeadToLinkInterfaceMap(config));

  // If the link interface does not depend on the head target
  // then return the one we computed first.
  if (!hm.empty() && !hm.begin()->second.HadHeadSensitiveCondition) {
    return &hm.begin()->second;
  }

  cmOptionalLinkInterface& iface = hm[headTarget];
  if (!iface.AllDone) {
    iface.AllDone = true;
    iface.Multiplicity = info->Multiplicity;
    cmSystemTools::ExpandListArgument(info->Languages, iface.Languages);
    this->ExpandLinkItems(info->LibrariesProp, info->Libraries, config,
                          headTarget, usage_requirements_only, iface.Libraries,
                          iface.HadHeadSensitiveCondition);
    std::vector<std::string> deps;
    cmSystemTools::ExpandListArgument(info->SharedDeps, deps);
    this->LookupLinkItems(deps, iface.SharedDeps);
  }

  return &iface;
}

cmGeneratorTarget::ImportInfo const* cmGeneratorTarget::GetImportInfo(
  const std::string& config) const
{
  // There is no imported information for non-imported targets.
  if (!this->IsImported()) {
    return CM_NULLPTR;
  }

  // Lookup/compute/cache the import information for this
  // configuration.
  std::string config_upper;
  if (!config.empty()) {
    config_upper = cmSystemTools::UpperCase(config);
  } else {
    config_upper = "NOCONFIG";
  }

  ImportInfoMapType::const_iterator i = this->ImportInfoMap.find(config_upper);
  if (i == this->ImportInfoMap.end()) {
    ImportInfo info;
    this->ComputeImportInfo(config_upper, info);
    ImportInfoMapType::value_type entry(config_upper, info);
    i = this->ImportInfoMap.insert(entry).first;
  }

  if (this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return &i->second;
  }
  // If the location is empty then the target is not available for
  // this configuration.
  if (i->second.Location.empty() && i->second.ImportLibrary.empty()) {
    return CM_NULLPTR;
  }

  // Return the import information.
  return &i->second;
}

void cmGeneratorTarget::ComputeImportInfo(std::string const& desired_config,
                                          ImportInfo& info) const
{
  // This method finds information about an imported target from its
  // properties.  The "IMPORTED_" namespace is reserved for properties
  // defined by the project exporting the target.

  // Initialize members.
  info.NoSOName = false;

  const char* loc = CM_NULLPTR;
  const char* imp = CM_NULLPTR;
  std::string suffix;
  if (!this->Target->GetMappedConfig(desired_config, &loc, &imp, suffix)) {
    return;
  }

  // Get the link interface.
  {
    std::string linkProp = "INTERFACE_LINK_LIBRARIES";
    const char* propertyLibs = this->GetProperty(linkProp);

    if (this->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
      if (!propertyLibs) {
        linkProp = "IMPORTED_LINK_INTERFACE_LIBRARIES";
        linkProp += suffix;
        propertyLibs = this->GetProperty(linkProp);
      }

      if (!propertyLibs) {
        linkProp = "IMPORTED_LINK_INTERFACE_LIBRARIES";
        propertyLibs = this->GetProperty(linkProp);
      }
    }
    if (propertyLibs) {
      info.LibrariesProp = linkProp;
      info.Libraries = propertyLibs;
    }
  }
  if (this->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    if (loc) {
      info.LibName = loc;
    }
    return;
  }

  // A provided configuration has been chosen.  Load the
  // configuration's properties.

  // Get the location.
  if (loc) {
    info.Location = loc;
  } else {
    std::string impProp = "IMPORTED_LOCATION";
    impProp += suffix;
    if (const char* config_location = this->GetProperty(impProp)) {
      info.Location = config_location;
    } else if (const char* location = this->GetProperty("IMPORTED_LOCATION")) {
      info.Location = location;
    }
  }

  // Get the soname.
  if (this->GetType() == cmStateEnums::SHARED_LIBRARY) {
    std::string soProp = "IMPORTED_SONAME";
    soProp += suffix;
    if (const char* config_soname = this->GetProperty(soProp)) {
      info.SOName = config_soname;
    } else if (const char* soname = this->GetProperty("IMPORTED_SONAME")) {
      info.SOName = soname;
    }
  }

  // Get the "no-soname" mark.
  if (this->GetType() == cmStateEnums::SHARED_LIBRARY) {
    std::string soProp = "IMPORTED_NO_SONAME";
    soProp += suffix;
    if (const char* config_no_soname = this->GetProperty(soProp)) {
      info.NoSOName = cmSystemTools::IsOn(config_no_soname);
    } else if (const char* no_soname =
                 this->GetProperty("IMPORTED_NO_SONAME")) {
      info.NoSOName = cmSystemTools::IsOn(no_soname);
    }
  }

  // Get the import library.
  if (imp) {
    info.ImportLibrary = imp;
  } else if (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
             this->IsExecutableWithExports()) {
    std::string impProp = "IMPORTED_IMPLIB";
    impProp += suffix;
    if (const char* config_implib = this->GetProperty(impProp)) {
      info.ImportLibrary = config_implib;
    } else if (const char* implib = this->GetProperty("IMPORTED_IMPLIB")) {
      info.ImportLibrary = implib;
    }
  }

  // Get the link dependencies.
  {
    std::string linkProp = "IMPORTED_LINK_DEPENDENT_LIBRARIES";
    linkProp += suffix;
    if (const char* config_libs = this->GetProperty(linkProp)) {
      info.SharedDeps = config_libs;
    } else if (const char* libs =
                 this->GetProperty("IMPORTED_LINK_DEPENDENT_LIBRARIES")) {
      info.SharedDeps = libs;
    }
  }

  // Get the link languages.
  if (this->LinkLanguagePropagatesToDependents()) {
    std::string linkProp = "IMPORTED_LINK_INTERFACE_LANGUAGES";
    linkProp += suffix;
    if (const char* config_libs = this->GetProperty(linkProp)) {
      info.Languages = config_libs;
    } else if (const char* libs =
                 this->GetProperty("IMPORTED_LINK_INTERFACE_LANGUAGES")) {
      info.Languages = libs;
    }
  }

  // Get the cyclic repetition count.
  if (this->GetType() == cmStateEnums::STATIC_LIBRARY) {
    std::string linkProp = "IMPORTED_LINK_INTERFACE_MULTIPLICITY";
    linkProp += suffix;
    if (const char* config_reps = this->GetProperty(linkProp)) {
      sscanf(config_reps, "%u", &info.Multiplicity);
    } else if (const char* reps =
                 this->GetProperty("IMPORTED_LINK_INTERFACE_MULTIPLICITY")) {
      sscanf(reps, "%u", &info.Multiplicity);
    }
  }
}

cmHeadToLinkInterfaceMap& cmGeneratorTarget::GetHeadToLinkInterfaceMap(
  const std::string& config) const
{
  std::string CONFIG = cmSystemTools::UpperCase(config);
  return this->LinkInterfaceMap[CONFIG];
}

cmHeadToLinkInterfaceMap&
cmGeneratorTarget::GetHeadToLinkInterfaceUsageRequirementsMap(
  const std::string& config) const
{
  std::string CONFIG = cmSystemTools::UpperCase(config);
  return this->LinkInterfaceUsageRequirementsOnlyMap[CONFIG];
}

const cmLinkImplementation* cmGeneratorTarget::GetLinkImplementation(
  const std::string& config) const
{
  // There is no link implementation for imported targets.
  if (this->IsImported()) {
    return CM_NULLPTR;
  }

  std::string CONFIG = cmSystemTools::UpperCase(config);
  cmOptionalLinkImplementation& impl = this->LinkImplMap[CONFIG][this];
  if (!impl.LibrariesDone) {
    impl.LibrariesDone = true;
    this->ComputeLinkImplementationLibraries(config, impl, this);
  }
  if (!impl.LanguagesDone) {
    impl.LanguagesDone = true;
    this->ComputeLinkImplementationLanguages(config, impl);
  }
  return &impl;
}

bool cmGeneratorTarget::GetConfigCommonSourceFiles(
  std::vector<cmSourceFile*>& files) const
{
  std::vector<std::string> configs;
  this->Makefile->GetConfigurations(configs);
  if (configs.empty()) {
    configs.push_back("");
  }

  std::vector<std::string>::const_iterator it = configs.begin();
  const std::string& firstConfig = *it;
  this->GetSourceFilesWithoutObjectLibraries(files, firstConfig);

  for (; it != configs.end(); ++it) {
    std::vector<cmSourceFile*> configFiles;
    this->GetSourceFilesWithoutObjectLibraries(configFiles, *it);
    if (configFiles != files) {
      std::string firstConfigFiles;
      const char* sep = "";
      for (std::vector<cmSourceFile*>::const_iterator fi = files.begin();
           fi != files.end(); ++fi) {
        firstConfigFiles += sep;
        firstConfigFiles += (*fi)->GetFullPath();
        sep = "\n  ";
      }

      std::string thisConfigFiles;
      sep = "";
      for (std::vector<cmSourceFile*>::const_iterator fi = configFiles.begin();
           fi != configFiles.end(); ++fi) {
        thisConfigFiles += sep;
        thisConfigFiles += (*fi)->GetFullPath();
        sep = "\n  ";
      }
      std::ostringstream e;
      /* clang-format off */
      e << "Target \"" << this->GetName()
        << "\" has source files which vary by "
        "configuration. This is not supported by the \""
        << this->GlobalGenerator->GetName()
        << "\" generator.\n"
          "Config \"" << firstConfig << "\":\n"
          "  " << firstConfigFiles << "\n"
          "Config \"" << *it << "\":\n"
          "  " << thisConfigFiles << "\n";
      /* clang-format on */
      this->LocalGenerator->IssueMessage(cmake::FATAL_ERROR, e.str());
      return false;
    }
  }
  return true;
}

void cmGeneratorTarget::GetObjectLibrariesCMP0026(
  std::vector<cmGeneratorTarget*>& objlibs) const
{
  // At configure-time, this method can be called as part of getting the
  // LOCATION property or to export() a file to be include()d.  However
  // there is no cmGeneratorTarget at configure-time, so search the SOURCES
  // for TARGET_OBJECTS instead for backwards compatibility with OLD
  // behavior of CMP0024 and CMP0026 only.
  cmStringRange rng = this->Target->GetSourceEntries();
  for (std::vector<std::string>::const_iterator i = rng.begin();
       i != rng.end(); ++i) {
    std::string const& entry = *i;

    std::vector<std::string> files;
    cmSystemTools::ExpandListArgument(entry, files);
    for (std::vector<std::string>::const_iterator li = files.begin();
         li != files.end(); ++li) {
      if (cmHasLiteralPrefix(*li, "$<TARGET_OBJECTS:") &&
          (*li)[li->size() - 1] == '>') {
        std::string objLibName = li->substr(17, li->size() - 18);

        if (cmGeneratorExpression::Find(objLibName) != std::string::npos) {
          continue;
        }
        cmGeneratorTarget* objLib =
          this->LocalGenerator->FindGeneratorTargetToUse(objLibName);
        if (objLib) {
          objlibs.push_back(objLib);
        }
      }
    }
  }
}

std::string cmGeneratorTarget::CheckCMP0004(std::string const& item) const
{
  // Strip whitespace off the library names because we used to do this
  // in case variables were expanded at generate time.  We no longer
  // do the expansion but users link to libraries like " ${VAR} ".
  std::string lib = item;
  std::string::size_type pos = lib.find_first_not_of(" \t\r\n");
  if (pos != std::string::npos) {
    lib = lib.substr(pos);
  }
  pos = lib.find_last_not_of(" \t\r\n");
  if (pos != std::string::npos) {
    lib = lib.substr(0, pos + 1);
  }
  if (lib != item) {
    cmake* cm = this->LocalGenerator->GetCMakeInstance();
    switch (this->GetPolicyStatusCMP0004()) {
      case cmPolicies::WARN: {
        std::ostringstream w;
        w << cmPolicies::GetPolicyWarning(cmPolicies::CMP0004) << "\n"
          << "Target \"" << this->GetName() << "\" links to item \"" << item
          << "\" which has leading or trailing whitespace.";
        cm->IssueMessage(cmake::AUTHOR_WARNING, w.str(), this->GetBacktrace());
      }
      case cmPolicies::OLD:
        break;
      case cmPolicies::NEW: {
        std::ostringstream e;
        e << "Target \"" << this->GetName() << "\" links to item \"" << item
          << "\" which has leading or trailing whitespace.  "
          << "This is now an error according to policy CMP0004.";
        cm->IssueMessage(cmake::FATAL_ERROR, e.str(), this->GetBacktrace());
      } break;
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS: {
        std::ostringstream e;
        e << cmPolicies::GetRequiredPolicyError(cmPolicies::CMP0004) << "\n"
          << "Target \"" << this->GetName() << "\" links to item \"" << item
          << "\" which has leading or trailing whitespace.";
        cm->IssueMessage(cmake::FATAL_ERROR, e.str(), this->GetBacktrace());
      } break;
    }
  }
  return lib;
}

void cmGeneratorTarget::GetLanguages(std::set<std::string>& languages,
                                     const std::string& config) const
{
  std::vector<cmSourceFile*> sourceFiles;
  this->GetSourceFiles(sourceFiles, config);
  for (std::vector<cmSourceFile*>::const_iterator i = sourceFiles.begin();
       i != sourceFiles.end(); ++i) {
    const std::string& lang = (*i)->GetLanguage();
    if (!lang.empty()) {
      languages.insert(lang);
    }
  }

  std::vector<cmGeneratorTarget*> objectLibraries;
  std::vector<cmSourceFile const*> externalObjects;
  if (!this->GlobalGenerator->GetConfigureDoneCMP0026()) {
    std::vector<cmGeneratorTarget*> objectTargets;
    this->GetObjectLibrariesCMP0026(objectTargets);
    objectLibraries.reserve(objectTargets.size());
    for (std::vector<cmGeneratorTarget*>::const_iterator it =
           objectTargets.begin();
         it != objectTargets.end(); ++it) {
      objectLibraries.push_back(*it);
    }
  } else {
    this->GetExternalObjects(externalObjects, config);
    for (std::vector<cmSourceFile const*>::const_iterator i =
           externalObjects.begin();
         i != externalObjects.end(); ++i) {
      std::string objLib = (*i)->GetObjectLibrary();
      if (cmGeneratorTarget* tgt =
            this->LocalGenerator->FindGeneratorTargetToUse(objLib)) {
        objectLibraries.push_back(tgt);
      }
    }
  }
  for (std::vector<cmGeneratorTarget*>::const_iterator i =
         objectLibraries.begin();
       i != objectLibraries.end(); ++i) {
    (*i)->GetLanguages(languages, config);
  }
}

void cmGeneratorTarget::ComputeLinkImplementationLanguages(
  const std::string& config, cmOptionalLinkImplementation& impl) const
{
  // This target needs runtime libraries for its source languages.
  std::set<std::string> languages;
  // Get languages used in our source files.
  this->GetLanguages(languages, config);
  // Copy the set of langauges to the link implementation.
  impl.Languages.insert(impl.Languages.begin(), languages.begin(),
                        languages.end());
}

bool cmGeneratorTarget::HaveBuildTreeRPATH(const std::string& config) const
{
  if (this->GetPropertyAsBool("SKIP_BUILD_RPATH")) {
    return false;
  }
  if (this->GetProperty("BUILD_RPATH")) {
    return true;
  }
  if (cmLinkImplementationLibraries const* impl =
        this->GetLinkImplementationLibraries(config)) {
    return !impl->Libraries.empty();
  }
  return false;
}

cmLinkImplementationLibraries const*
cmGeneratorTarget::GetLinkImplementationLibraries(
  const std::string& config) const
{
  return this->GetLinkImplementationLibrariesInternal(config, this);
}

cmLinkImplementationLibraries const*
cmGeneratorTarget::GetLinkImplementationLibrariesInternal(
  const std::string& config, cmGeneratorTarget const* head) const
{
  // There is no link implementation for imported targets.
  if (this->IsImported()) {
    return CM_NULLPTR;
  }

  // Populate the link implementation libraries for this configuration.
  std::string CONFIG = cmSystemTools::UpperCase(config);
  HeadToLinkImplementationMap& hm = this->LinkImplMap[CONFIG];

  // If the link implementation does not depend on the head target
  // then return the one we computed first.
  if (!hm.empty() && !hm.begin()->second.HadHeadSensitiveCondition) {
    return &hm.begin()->second;
  }

  cmOptionalLinkImplementation& impl = hm[head];
  if (!impl.LibrariesDone) {
    impl.LibrariesDone = true;
    this->ComputeLinkImplementationLibraries(config, impl, head);
  }
  return &impl;
}

bool cmGeneratorTarget::IsNullImpliedByLinkLibraries(
  const std::string& p) const
{
  return this->LinkImplicitNullProperties.find(p) !=
    this->LinkImplicitNullProperties.end();
}

void cmGeneratorTarget::ComputeLinkImplementationLibraries(
  const std::string& config, cmOptionalLinkImplementation& impl,
  cmGeneratorTarget const* head) const
{
  cmStringRange entryRange = this->Target->GetLinkImplementationEntries();
  cmBacktraceRange btRange = this->Target->GetLinkImplementationBacktraces();
  cmBacktraceRange::const_iterator btIt = btRange.begin();
  // Collect libraries directly linked in this configuration.
  for (cmStringRange::const_iterator le = entryRange.begin(),
                                     end = entryRange.end();
       le != end; ++le, ++btIt) {
    std::vector<std::string> llibs;
    cmGeneratorExpressionDAGChecker dagChecker(
      this->GetName(), "LINK_LIBRARIES", CM_NULLPTR, CM_NULLPTR);
    cmGeneratorExpression ge(*btIt);
    CM_AUTO_PTR<cmCompiledGeneratorExpression> const cge = ge.Parse(*le);
    std::string const evaluated =
      cge->Evaluate(this->LocalGenerator, config, false, head, &dagChecker);
    cmSystemTools::ExpandListArgument(evaluated, llibs);
    if (cge->GetHadHeadSensitiveCondition()) {
      impl.HadHeadSensitiveCondition = true;
    }

    for (std::vector<std::string>::const_iterator li = llibs.begin();
         li != llibs.end(); ++li) {
      // Skip entries that resolve to the target itself or are empty.
      std::string name = this->CheckCMP0004(*li);
      if (name == this->GetName() || name.empty()) {
        if (name == this->GetName()) {
          bool noMessage = false;
          cmake::MessageType messageType = cmake::FATAL_ERROR;
          std::ostringstream e;
          switch (this->GetPolicyStatusCMP0038()) {
            case cmPolicies::WARN: {
              e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0038) << "\n";
              messageType = cmake::AUTHOR_WARNING;
            } break;
            case cmPolicies::OLD:
              noMessage = true;
            case cmPolicies::REQUIRED_IF_USED:
            case cmPolicies::REQUIRED_ALWAYS:
            case cmPolicies::NEW:
              // Issue the fatal message.
              break;
          }

          if (!noMessage) {
            e << "Target \"" << this->GetName() << "\" links to itself.";
            this->LocalGenerator->GetCMakeInstance()->IssueMessage(
              messageType, e.str(), this->GetBacktrace());
            if (messageType == cmake::FATAL_ERROR) {
              return;
            }
          }
        }
        continue;
      }

      // The entry is meant for this configuration.
      impl.Libraries.push_back(cmLinkImplItem(
        name, this->FindTargetToLink(name), *btIt, evaluated != *le));
    }

    std::set<std::string> const& seenProps = cge->GetSeenTargetProperties();
    for (std::set<std::string>::const_iterator it = seenProps.begin();
         it != seenProps.end(); ++it) {
      if (!this->GetProperty(*it)) {
        this->LinkImplicitNullProperties.insert(*it);
      }
    }
    cge->GetMaxLanguageStandard(this, this->MaxLanguageStandards);
  }

  // Get the list of configurations considered to be DEBUG.
  std::vector<std::string> debugConfigs =
    this->Makefile->GetCMakeInstance()->GetDebugConfigs();

  cmTargetLinkLibraryType linkType =
    CMP0003_ComputeLinkType(config, debugConfigs);
  cmTarget::LinkLibraryVectorType const& oldllibs =
    this->Target->GetOriginalLinkLibraries();
  for (cmTarget::LinkLibraryVectorType::const_iterator li = oldllibs.begin();
       li != oldllibs.end(); ++li) {
    if (li->second != GENERAL_LibraryType && li->second != linkType) {
      std::string name = this->CheckCMP0004(li->first);
      if (name == this->GetName() || name.empty()) {
        continue;
      }
      // Support OLD behavior for CMP0003.
      impl.WrongConfigLibraries.push_back(
        cmLinkItem(name, this->FindTargetToLink(name)));
    }
  }
}

cmGeneratorTarget* cmGeneratorTarget::FindTargetToLink(
  std::string const& name) const
{
  cmGeneratorTarget* tgt =
    this->LocalGenerator->FindGeneratorTargetToUse(name);

  // Skip targets that will not really be linked.  This is probably a
  // name conflict between an external library and an executable
  // within the project.
  if (tgt && tgt->GetType() == cmStateEnums::EXECUTABLE &&
      !tgt->IsExecutableWithExports()) {
    tgt = CM_NULLPTR;
  }

  if (tgt && tgt->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    std::ostringstream e;
    e << "Target \"" << this->GetName() << "\" links to "
                                           "OBJECT library \""
      << tgt->GetName()
      << "\" but this is not "
         "allowed.  "
         "One may link only to STATIC or SHARED libraries, or to executables "
         "with the ENABLE_EXPORTS property set.";
    cmake* cm = this->LocalGenerator->GetCMakeInstance();
    cm->IssueMessage(cmake::FATAL_ERROR, e.str(), this->GetBacktrace());
    tgt = CM_NULLPTR;
  }

  return tgt;
}

std::string cmGeneratorTarget::GetPDBDirectory(const std::string& config) const
{
  if (OutputInfo const* info = this->GetOutputInfo(config)) {
    // Return the directory in which the target will be built.
    return info->PdbDir;
  }
  return "";
}

bool cmGeneratorTarget::HasImplibGNUtoMS() const
{
  return this->HasImportLibrary() && this->GetPropertyAsBool("GNUtoMS");
}

bool cmGeneratorTarget::GetImplibGNUtoMS(std::string const& gnuName,
                                         std::string& out,
                                         const char* newExt) const
{
  if (this->HasImplibGNUtoMS() && gnuName.size() > 6 &&
      gnuName.substr(gnuName.size() - 6) == ".dll.a") {
    out = gnuName.substr(0, gnuName.size() - 6);
    out += newExt ? newExt : ".lib";
    return true;
  }
  return false;
}

bool cmGeneratorTarget::IsExecutableWithExports() const
{
  return (this->GetType() == cmStateEnums::EXECUTABLE &&
          this->GetPropertyAsBool("ENABLE_EXPORTS"));
}

bool cmGeneratorTarget::HasImportLibrary() const
{
  return (this->IsDLLPlatform() &&
          (this->GetType() == cmStateEnums::SHARED_LIBRARY ||
           this->IsExecutableWithExports()));
}

std::string cmGeneratorTarget::GetSupportDirectory() const
{
  std::string dir = this->LocalGenerator->GetCurrentBinaryDirectory();
  dir += cmake::GetCMakeFilesDirectory();
  dir += "/";
  dir += this->GetName();
#if defined(__VMS)
  dir += "_dir";
#else
  dir += ".dir";
#endif
  return dir;
}

bool cmGeneratorTarget::IsLinkable() const
{
  return (this->GetType() == cmStateEnums::STATIC_LIBRARY ||
          this->GetType() == cmStateEnums::SHARED_LIBRARY ||
          this->GetType() == cmStateEnums::MODULE_LIBRARY ||
          this->GetType() == cmStateEnums::UNKNOWN_LIBRARY ||
          this->GetType() == cmStateEnums::INTERFACE_LIBRARY ||
          this->IsExecutableWithExports());
}

bool cmGeneratorTarget::IsFrameworkOnApple() const
{
  return ((this->GetType() == cmStateEnums::SHARED_LIBRARY ||
           this->GetType() == cmStateEnums::STATIC_LIBRARY) &&
          this->Makefile->IsOn("APPLE") &&
          this->GetPropertyAsBool("FRAMEWORK"));
}

bool cmGeneratorTarget::IsAppBundleOnApple() const
{
  return (this->GetType() == cmStateEnums::EXECUTABLE &&
          this->Makefile->IsOn("APPLE") &&
          this->GetPropertyAsBool("MACOSX_BUNDLE"));
}

bool cmGeneratorTarget::IsXCTestOnApple() const
{
  return (this->IsCFBundleOnApple() && this->GetPropertyAsBool("XCTEST"));
}

bool cmGeneratorTarget::IsCFBundleOnApple() const
{
  return (this->GetType() == cmStateEnums::MODULE_LIBRARY &&
          this->Makefile->IsOn("APPLE") && this->GetPropertyAsBool("BUNDLE"));
}
