/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGeneratorTarget_h
#define cmGeneratorTarget_h

#include "cmConfigure.h"

#include "cmLinkItem.h"
#include "cmListFileCache.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"

#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <utility>
#include <vector>

class cmComputeLinkInformation;
class cmCustomCommand;
class cmGlobalGenerator;
class cmLocalGenerator;
class cmMakefile;
class cmSourceFile;
class cmTarget;

class cmGeneratorTarget
{
  CM_DISABLE_COPY(cmGeneratorTarget)

public:
  cmGeneratorTarget(cmTarget*, cmLocalGenerator* lg);
  ~cmGeneratorTarget();

  cmLocalGenerator* GetLocalGenerator() const;

  cmGlobalGenerator* GetGlobalGenerator() const;

  bool IsImported() const;
  bool IsImportedGloballyVisible() const;
  const char* GetLocation(const std::string& config) const;

  std::vector<cmCustomCommand> const& GetPreBuildCommands() const;
  std::vector<cmCustomCommand> const& GetPreLinkCommands() const;
  std::vector<cmCustomCommand> const& GetPostBuildCommands() const;

#define DECLARE_TARGET_POLICY(POLICY)                                         \
  cmPolicies::PolicyStatus GetPolicyStatus##POLICY() const                    \
  {                                                                           \
    return this->PolicyMap.Get(cmPolicies::POLICY);                           \
  }

  CM_FOR_EACH_TARGET_POLICY(DECLARE_TARGET_POLICY)

#undef DECLARE_TARGET_POLICY

  /** Get the location of the target in the build tree with a placeholder
      referencing the configuration in the native build system.  This
      location is suitable for use as the LOCATION target property.  */
  const char* GetLocationForBuild() const;

  cmComputeLinkInformation* GetLinkInformation(
    const std::string& config) const;

  cmStateEnums::TargetType GetType() const;
  const std::string& GetName() const;
  std::string GetExportName() const;

  std::vector<std::string> GetPropertyKeys() const;
  const char* GetProperty(const std::string& prop) const;
  bool GetPropertyAsBool(const std::string& prop) const;
  void GetSourceFiles(std::vector<cmSourceFile*>& files,
                      const std::string& config) const;
  void GetSourceFilesWithoutObjectLibraries(std::vector<cmSourceFile*>& files,
                                            const std::string& config) const;

  /** Source file kinds (classifications).
      Generators use this to decide how to treat a source file.  */
  enum SourceKind
  {
    SourceKindAppManifest,
    SourceKindCertificate,
    SourceKindCustomCommand,
    SourceKindExternalObject,
    SourceKindExtra,
    SourceKindHeader,
    SourceKindIDL,
    SourceKindManifest,
    SourceKindModuleDefinition,
    SourceKindObjectSource,
    SourceKindResx,
    SourceKindXaml
  };

  /** A source file paired with a kind (classification).  */
  struct SourceAndKind
  {
    cmSourceFile* Source;
    SourceKind Kind;
  };

  /** All sources needed for a configuration with kinds assigned.  */
  struct KindedSources
  {
    std::vector<SourceAndKind> Sources;
    std::set<std::string> ExpectedResxHeaders;
    std::set<std::string> ExpectedXamlHeaders;
    std::set<std::string> ExpectedXamlSources;
    bool Initialized;
    KindedSources()
      : Initialized(false)
    {
    }
  };

  /** Get all sources needed for a configuration with kinds assigned.  */
  KindedSources const& GetKindedSources(std::string const& config) const;

  struct AllConfigSource
  {
    cmSourceFile const* Source;
    cmGeneratorTarget::SourceKind Kind;
    std::vector<size_t> Configs;
  };

  /** Get all sources needed for all configurations with kinds and
      per-source configurations assigned.  */
  std::vector<AllConfigSource> const& GetAllConfigSources() const;

  void GetObjectSources(std::vector<cmSourceFile const*>&,
                        const std::string& config) const;
  const std::string& GetObjectName(cmSourceFile const* file);
  const char* GetCustomObjectExtension() const;

  bool HasExplicitObjectName(cmSourceFile const* file) const;
  void AddExplicitObjectName(cmSourceFile const* sf);

  void GetModuleDefinitionSources(std::vector<cmSourceFile const*>&,
                                  const std::string& config) const;
  void GetResxSources(std::vector<cmSourceFile const*>&,
                      const std::string& config) const;
  void GetExternalObjects(std::vector<cmSourceFile const*>&,
                          const std::string& config) const;
  void GetHeaderSources(std::vector<cmSourceFile const*>&,
                        const std::string& config) const;
  void GetExtraSources(std::vector<cmSourceFile const*>&,
                       const std::string& config) const;
  void GetCustomCommands(std::vector<cmSourceFile const*>&,
                         const std::string& config) const;
  void GetExpectedResxHeaders(std::set<std::string>&,
                              const std::string& config) const;
  void GetAppManifest(std::vector<cmSourceFile const*>&,
                      const std::string& config) const;
  void GetManifests(std::vector<cmSourceFile const*>&,
                    const std::string& config) const;
  void GetCertificates(std::vector<cmSourceFile const*>&,
                       const std::string& config) const;
  void GetXamlSources(std::vector<cmSourceFile const*>&,
                      const std::string& config) const;
  void GetExpectedXamlHeaders(std::set<std::string>&,
                              const std::string& config) const;
  void GetExpectedXamlSources(std::set<std::string>&,
                              const std::string& config) const;

  std::set<cmLinkItem> const& GetUtilityItems() const;

  void ComputeObjectMapping();

  const char* GetFeature(const std::string& feature,
                         const std::string& config) const;

  bool IsIPOEnabled(std::string const& lang, std::string const& config) const;

  bool IsLinkInterfaceDependentBoolProperty(const std::string& p,
                                            const std::string& config) const;
  bool IsLinkInterfaceDependentStringProperty(const std::string& p,
                                              const std::string& config) const;
  bool IsLinkInterfaceDependentNumberMinProperty(
    const std::string& p, const std::string& config) const;
  bool IsLinkInterfaceDependentNumberMaxProperty(
    const std::string& p, const std::string& config) const;

  bool GetLinkInterfaceDependentBoolProperty(const std::string& p,
                                             const std::string& config) const;

  const char* GetLinkInterfaceDependentStringProperty(
    const std::string& p, const std::string& config) const;
  const char* GetLinkInterfaceDependentNumberMinProperty(
    const std::string& p, const std::string& config) const;
  const char* GetLinkInterfaceDependentNumberMaxProperty(
    const std::string& p, const std::string& config) const;

  cmLinkInterface const* GetLinkInterface(
    const std::string& config, const cmGeneratorTarget* headTarget) const;
  void ComputeLinkInterface(const std::string& config,
                            cmOptionalLinkInterface& iface,
                            const cmGeneratorTarget* head) const;

  cmLinkInterfaceLibraries const* GetLinkInterfaceLibraries(
    const std::string& config, const cmGeneratorTarget* headTarget,
    bool usage_requirements_only) const;

  void ComputeLinkInterfaceLibraries(const std::string& config,
                                     cmOptionalLinkInterface& iface,
                                     const cmGeneratorTarget* head,
                                     bool usage_requirements_only) const;

  /** Get the library name for an imported interface library.  */
  std::string GetImportedLibName(std::string const& config) const;

  /** Get the full path to the target according to the settings in its
      makefile and the configuration type.  */
  std::string GetFullPath(
    const std::string& config = "",
    cmStateEnums::ArtifactType artifact = cmStateEnums::RuntimeBinaryArtifact,
    bool realname = false) const;
  std::string NormalGetFullPath(const std::string& config,
                                cmStateEnums::ArtifactType artifact,
                                bool realname) const;
  std::string NormalGetRealName(const std::string& config) const;

  /** Get the names of an object library's object files underneath
      its object file directory.  */
  void GetTargetObjectNames(std::string const& config,
                            std::vector<std::string>& objects) const;

  /** What hierarchy level should the reported directory contain */
  enum BundleDirectoryLevel
  {
    BundleDirLevel,
    ContentLevel,
    FullLevel
  };

  /** @return the Mac App directory without the base */
  std::string GetAppBundleDirectory(const std::string& config,
                                    BundleDirectoryLevel level) const;

  /** Return whether this target is an executable Bundle, a framework
      or CFBundle on Apple.  */
  bool IsBundleOnApple() const;

  /** Get the full name of the target according to the settings in its
      makefile.  */
  std::string GetFullName(const std::string& config = "",
                          cmStateEnums::ArtifactType artifact =
                            cmStateEnums::RuntimeBinaryArtifact) const;

  /** @return the Mac framework directory without the base. */
  std::string GetFrameworkDirectory(const std::string& config,
                                    BundleDirectoryLevel level) const;

  /** Return the framework version string.  Undefined if
      IsFrameworkOnApple returns false.  */
  std::string GetFrameworkVersion() const;

  /** @return the Mac CFBundle directory without the base */
  std::string GetCFBundleDirectory(const std::string& config,
                                   BundleDirectoryLevel level) const;

  /** Return the install name directory for the target in the
    * build tree.  For example: "\@rpath/", "\@loader_path/",
    * or "/full/path/to/library".  */
  std::string GetInstallNameDirForBuildTree(const std::string& config) const;

  /** Return the install name directory for the target in the
    * install tree.  For example: "\@rpath/" or "\@loader_path/". */
  std::string GetInstallNameDirForInstallTree() const;

  cmListFileBacktrace GetBacktrace() const;

  const std::vector<std::string>& GetLinkDirectories() const;

  std::set<std::string> const& GetUtilities() const;
  cmListFileBacktrace const* GetUtilityBacktrace(const std::string& u) const;

  bool LinkLanguagePropagatesToDependents() const
  {
    return this->GetType() == cmStateEnums::STATIC_LIBRARY;
  }

  /** Get the macro to define when building sources in this target.
      If no macro should be defined null is returned.  */
  const char* GetExportMacro() const;

  /** Get the soname of the target.  Allowed only for a shared library.  */
  std::string GetSOName(const std::string& config) const;

  void GetFullNameComponents(std::string& prefix, std::string& base,
                             std::string& suffix,
                             const std::string& config = "",
                             cmStateEnums::ArtifactType artifact =
                               cmStateEnums::RuntimeBinaryArtifact) const;

  /** Append to @a base the bundle directory hierarchy up to a certain @a level
   * and return it. */
  std::string BuildBundleDirectory(const std::string& base,
                                   const std::string& config,
                                   BundleDirectoryLevel level) const;

  /** @return the mac content directory for this target. */
  std::string GetMacContentDirectory(
    const std::string& config, cmStateEnums::ArtifactType artifact) const;

  /** @return folder prefix for IDEs. */
  std::string GetEffectiveFolderName() const;

  cmTarget* Target;
  cmMakefile* Makefile;
  cmLocalGenerator* LocalGenerator;
  cmGlobalGenerator const* GlobalGenerator;

  struct ModuleDefinitionInfo
  {
    std::string DefFile;
    bool DefFileGenerated;
    bool WindowsExportAllSymbols;
    std::vector<cmSourceFile const*> Sources;
  };
  ModuleDefinitionInfo const* GetModuleDefinitionInfo(
    std::string const& config) const;

  /** Return whether or not the target is for a DLL platform.  */
  bool IsDLLPlatform() const;

  /** @return whether this target have a well defined output file name. */
  bool HaveWellDefinedOutputFiles() const;

  /** Link information from the transitive closure of the link
      implementation and the interfaces of its dependencies.  */
  struct LinkClosure
  {
    // The preferred linker language.
    std::string LinkerLanguage;

    // Languages whose runtime libraries must be linked.
    std::vector<std::string> Languages;
  };

  LinkClosure const* GetLinkClosure(const std::string& config) const;
  void ComputeLinkClosure(const std::string& config, LinkClosure& lc) const;

  cmLinkImplementation const* GetLinkImplementation(
    const std::string& config) const;

  void ComputeLinkImplementationLanguages(
    const std::string& config, cmOptionalLinkImplementation& impl) const;

  cmLinkImplementationLibraries const* GetLinkImplementationLibraries(
    const std::string& config) const;

  void ComputeLinkImplementationLibraries(const std::string& config,
                                          cmOptionalLinkImplementation& impl,
                                          const cmGeneratorTarget* head) const;

  cmGeneratorTarget* FindTargetToLink(std::string const& name) const;

  // Compute the set of languages compiled by the target.  This is
  // computed every time it is called because the languages can change
  // when source file properties are changed and we do not have enough
  // information to forward these property changes to the targets
  // until we have per-target object file properties.
  void GetLanguages(std::set<std::string>& languages,
                    std::string const& config) const;

  void GetObjectLibrariesCMP0026(
    std::vector<cmGeneratorTarget*>& objlibs) const;

  std::string GetFullNameImported(const std::string& config,
                                  cmStateEnums::ArtifactType artifact) const;

  /** Get source files common to all configurations and diagnose cases
      with per-config sources.  Excludes sources added by a TARGET_OBJECTS
      generator expression.  */
  bool GetConfigCommonSourceFiles(std::vector<cmSourceFile*>& files) const;

  bool HaveBuildTreeRPATH(const std::string& config) const;

  /** Full path with trailing slash to the top-level directory
      holding object files for this target.  Includes the build
      time config name placeholder if needed for the generator.  */
  std::string ObjectDirectory;

  /** Full path with trailing slash to the top-level directory
      holding object files for the given configuration.  */
  std::string GetObjectDirectory(std::string const& config) const;

  void GetAppleArchs(const std::string& config,
                     std::vector<std::string>& archVec) const;

  std::string GetFeatureSpecificLinkRuleVariable(
    std::string const& var, std::string const& lang,
    std::string const& config) const;

  /** Return the rule variable used to create this type of target.  */
  std::string GetCreateRuleVariable(std::string const& lang,
                                    std::string const& config) const;

  /** Get the include directories for this target.  */
  std::vector<std::string> GetIncludeDirectories(
    const std::string& config, const std::string& lang) const;

  void GetCompileOptions(std::vector<std::string>& result,
                         const std::string& config,
                         const std::string& language) const;

  void GetCompileFeatures(std::vector<std::string>& features,
                          const std::string& config) const;

  void GetCompileDefinitions(std::vector<std::string>& result,
                             const std::string& config,
                             const std::string& language) const;

  bool IsSystemIncludeDirectory(const std::string& dir,
                                const std::string& config) const;

  /** Add the target output files to the global generator manifest.  */
  void ComputeTargetManifest(const std::string& config) const;

  bool ComputeCompileFeatures(std::string const& config) const;

  /**
   * Trace through the source files in this target and add al source files
   * that they depend on, used by all generators
   */
  void TraceDependencies();

  /** Get the directory in which this target will be built.  If the
      configuration name is given then the generator will add its
      subdirectory for that configuration.  Otherwise just the canonical
      output directory is given.  */
  std::string GetDirectory(const std::string& config = "",
                           cmStateEnums::ArtifactType artifact =
                             cmStateEnums::RuntimeBinaryArtifact) const;

  /** Get the directory in which to place the target compiler .pdb file.
      If the configuration name is given then the generator will add its
      subdirectory for that configuration.  Otherwise just the canonical
      compiler pdb output directory is given.  */
  std::string GetCompilePDBDirectory(const std::string& config = "") const;

  /** Get sources that must be built before the given source.  */
  std::vector<cmSourceFile*> const* GetSourceDepends(
    cmSourceFile const* sf) const;

  /** Return whether this target uses the default value for its output
      directory.  */
  bool UsesDefaultOutputDir(const std::string& config,
                            cmStateEnums::ArtifactType artifact) const;

  // Cache target output paths for each configuration.
  struct OutputInfo
  {
    std::string OutDir;
    std::string ImpDir;
    std::string PdbDir;
    bool empty() const
    {
      return OutDir.empty() && ImpDir.empty() && PdbDir.empty();
    }
  };

  OutputInfo const* GetOutputInfo(const std::string& config) const;

  /** Get the name of the pdb file for the target.  */
  std::string GetPDBName(const std::string& config = "") const;

  /** Whether this library has soname enabled and platform supports it.  */
  bool HasSOName(const std::string& config) const;

  struct CompileInfo
  {
    std::string CompilePdbDir;
  };

  CompileInfo const* GetCompileInfo(const std::string& config) const;

  typedef std::map<std::string, CompileInfo> CompileInfoMapType;
  mutable CompileInfoMapType CompileInfoMap;

  bool IsNullImpliedByLinkLibraries(const std::string& p) const;

  /** Get the name of the compiler pdb file for the target.  */
  std::string GetCompilePDBName(const std::string& config = "") const;

  /** Get the path for the MSVC /Fd option for this target.  */
  std::string GetCompilePDBPath(const std::string& config = "") const;

  // Get the target base name.
  std::string GetOutputName(const std::string& config,
                            cmStateEnums::ArtifactType artifact) const;

  void AddSource(const std::string& src);
  void AddTracedSources(std::vector<std::string> const& srcs);

  /**
   * Adds an entry to the INCLUDE_DIRECTORIES list.
   * If before is true the entry is pushed at the front.
   */
  void AddIncludeDirectory(const std::string& src, bool before = false);

  /**
   * Flags for a given source file as used in this target. Typically assigned
   * via SET_TARGET_PROPERTIES when the property is a list of source files.
   */
  enum SourceFileType
  {
    SourceFileTypeNormal,
    SourceFileTypePrivateHeader, // is in "PRIVATE_HEADER" target property
    SourceFileTypePublicHeader,  // is in "PUBLIC_HEADER" target property
    SourceFileTypeResource,      // is in "RESOURCE" target property *or*
                                 // has MACOSX_PACKAGE_LOCATION=="Resources"
    SourceFileTypeDeepResource,  // MACOSX_PACKAGE_LOCATION starts with
                                 // "Resources/"
    SourceFileTypeMacContent     // has MACOSX_PACKAGE_LOCATION!="Resources[/]"
  };
  struct SourceFileFlags
  {
    SourceFileFlags()
      : Type(SourceFileTypeNormal)
      , MacFolder(CM_NULLPTR)
    {
    }
    SourceFileType Type;
    const char* MacFolder; // location inside Mac content folders
  };
  void GetAutoUicOptions(std::vector<std::string>& result,
                         const std::string& config) const;

  /** Get the names of the executable needed to generate a build rule
      that takes into account executable version numbers.  This should
      be called only on an executable target.  */
  void GetExecutableNames(std::string& name, std::string& realName,
                          std::string& impName, std::string& pdbName,
                          const std::string& config) const;

  /** Get the names of the library needed to generate a build rule
      that takes into account shared library version numbers.  This
      should be called only on a library target.  */
  void GetLibraryNames(std::string& name, std::string& soName,
                       std::string& realName, std::string& impName,
                       std::string& pdbName, const std::string& config) const;

  /**
   * Compute whether this target must be relinked before installing.
   */
  bool NeedRelinkBeforeInstall(const std::string& config) const;

  /** Return true if builtin chrpath will work for this target */
  bool IsChrpathUsed(const std::string& config) const;

  /** Get the directory in which this targets .pdb files will be placed.
      If the configuration name is given then the generator will add its
      subdirectory for that configuration.  Otherwise just the canonical
      pdb output directory is given.  */
  std::string GetPDBDirectory(const std::string& config) const;

  ///! Return the preferred linker language for this target
  std::string GetLinkerLanguage(const std::string& config) const;

  /** Does this target have a GNU implib to convert to MS format?  */
  bool HasImplibGNUtoMS() const;

  /** Convert the given GNU import library name (.dll.a) to a name with a new
      extension (.lib or ${CMAKE_IMPORT_LIBRARY_SUFFIX}).  */
  bool GetImplibGNUtoMS(std::string const& gnuName, std::string& out,
                        const char* newExt = CM_NULLPTR) const;

  bool IsExecutableWithExports() const;

  /** Return whether or not the target has a DLL import library.  */
  bool HasImportLibrary() const;

  /** Get a build-tree directory in which to place target support files.  */
  std::string GetSupportDirectory() const;

  /** Return whether this target may be used to link another target.  */
  bool IsLinkable() const;

  /** Return whether this target is a shared library Framework on
      Apple.  */
  bool IsFrameworkOnApple() const;

  /** Return whether this target is an executable Bundle on Apple.  */
  bool IsAppBundleOnApple() const;

  /** Return whether this target is a XCTest on Apple.  */
  bool IsXCTestOnApple() const;

  /** Return whether this target is a CFBundle (plugin) on Apple.  */
  bool IsCFBundleOnApple() const;

  struct SourceFileFlags GetTargetSourceFileFlags(
    const cmSourceFile* sf) const;

  void ReportPropertyOrigin(const std::string& p, const std::string& result,
                            const std::string& report,
                            const std::string& compatibilityType) const;

  class TargetPropertyEntry;

  bool HaveInstallTreeRPATH() const;

  /** Whether this library has \@rpath and platform supports it.  */
  bool HasMacOSXRpathInstallNameDir(const std::string& config) const;

  /** Whether this library defaults to \@rpath.  */
  bool MacOSXRpathInstallNameDirDefault() const;

  enum InstallNameType
  {
    INSTALL_NAME_FOR_BUILD,
    INSTALL_NAME_FOR_INSTALL
  };
  /** Whether to use INSTALL_NAME_DIR. */
  bool MacOSXUseInstallNameDir() const;
  /** Whether to generate an install_name. */
  bool CanGenerateInstallNameDir(InstallNameType t) const;

  /** Test for special case of a third-party shared library that has
      no soname at all.  */
  bool IsImportedSharedLibWithoutSOName(const std::string& config) const;

  const char* ImportedGetLocation(const std::string& config) const;

  /** Get the target major and minor version numbers interpreted from
      the VERSION property.  Version 0 is returned if the property is
      not set or cannot be parsed.  */
  void GetTargetVersion(int& major, int& minor) const;

  /** Get the target major, minor, and patch version numbers
      interpreted from the VERSION or SOVERSION property.  Version 0
      is returned if the property is not set or cannot be parsed.  */
  void GetTargetVersion(bool soversion, int& major, int& minor,
                        int& patch) const;

  std::string GetFortranModuleDirectory(std::string const& working_dir) const;

  const char* GetSourcesProperty() const;

private:
  void AddSourceCommon(const std::string& src);

  std::string CreateFortranModuleDirectory(
    std::string const& working_dir) const;
  mutable bool FortranModuleDirectoryCreated;
  mutable std::string FortranModuleDirectory;

  friend class cmTargetTraceDependencies;
  struct SourceEntry
  {
    std::vector<cmSourceFile*> Depends;
  };
  typedef std::map<cmSourceFile const*, SourceEntry> SourceEntriesType;
  SourceEntriesType SourceDepends;
  mutable std::map<cmSourceFile const*, std::string> Objects;
  std::set<cmSourceFile const*> ExplicitObjectName;
  mutable std::map<std::string, std::vector<std::string> > SystemIncludesCache;

  mutable std::string ExportMacro;

  void ConstructSourceFileFlags() const;
  mutable bool SourceFileFlagsConstructed;
  mutable std::map<cmSourceFile const*, SourceFileFlags> SourceFlagsMap;

  mutable std::map<std::string, bool> DebugCompatiblePropertiesDone;

  std::string GetFullNameInternal(const std::string& config,
                                  cmStateEnums::ArtifactType artifact) const;
  void GetFullNameInternal(const std::string& config,
                           cmStateEnums::ArtifactType artifact,
                           std::string& outPrefix, std::string& outBase,
                           std::string& outSuffix) const;

  typedef std::map<std::string, LinkClosure> LinkClosureMapType;
  mutable LinkClosureMapType LinkClosureMap;

  // Returns ARCHIVE, LIBRARY, or RUNTIME based on platform and type.
  const char* GetOutputTargetType(cmStateEnums::ArtifactType artifact) const;

  void ComputeVersionedName(std::string& vName, std::string const& prefix,
                            std::string const& base, std::string const& suffix,
                            std::string const& name,
                            const char* version) const;

  struct CompatibleInterfacesBase
  {
    std::set<std::string> PropsBool;
    std::set<std::string> PropsString;
    std::set<std::string> PropsNumberMax;
    std::set<std::string> PropsNumberMin;
  };
  CompatibleInterfacesBase const& GetCompatibleInterfaces(
    std::string const& config) const;

  struct CompatibleInterfaces : public CompatibleInterfacesBase
  {
    CompatibleInterfaces()
      : Done(false)
    {
    }
    bool Done;
  };
  mutable std::map<std::string, CompatibleInterfaces> CompatibleInterfacesMap;

  typedef std::map<std::string, cmComputeLinkInformation*>
    cmTargetLinkInformationMap;
  mutable cmTargetLinkInformationMap LinkInformation;

  void CheckPropertyCompatibility(cmComputeLinkInformation* info,
                                  const std::string& config) const;

  struct LinkImplClosure : public std::vector<cmGeneratorTarget const*>
  {
    LinkImplClosure()
      : Done(false)
    {
    }
    bool Done;
  };
  mutable std::map<std::string, LinkImplClosure> LinkImplClosureMap;

  typedef std::map<std::string, cmHeadToLinkInterfaceMap> LinkInterfaceMapType;
  mutable LinkInterfaceMapType LinkInterfaceMap;
  mutable LinkInterfaceMapType LinkInterfaceUsageRequirementsOnlyMap;

  cmHeadToLinkInterfaceMap& GetHeadToLinkInterfaceMap(
    std::string const& config) const;
  cmHeadToLinkInterfaceMap& GetHeadToLinkInterfaceUsageRequirementsMap(
    std::string const& config) const;

  // Cache import information from properties for each configuration.
  struct ImportInfo
  {
    ImportInfo()
      : NoSOName(false)
      , Multiplicity(0)
    {
    }
    bool NoSOName;
    unsigned int Multiplicity;
    std::string Location;
    std::string SOName;
    std::string ImportLibrary;
    std::string LibName;
    std::string Languages;
    std::string Libraries;
    std::string LibrariesProp;
    std::string SharedDeps;
  };

  typedef std::map<std::string, ImportInfo> ImportInfoMapType;
  mutable ImportInfoMapType ImportInfoMap;
  void ComputeImportInfo(std::string const& desired_config,
                         ImportInfo& info) const;
  ImportInfo const* GetImportInfo(const std::string& config) const;

  /** Strip off leading and trailing whitespace from an item named in
      the link dependencies of this target.  */
  std::string CheckCMP0004(std::string const& item) const;

  cmLinkInterface const* GetImportLinkInterface(
    const std::string& config, const cmGeneratorTarget* head,
    bool usage_requirements_only) const;

  typedef std::map<std::string, KindedSources> KindedSourcesMapType;
  mutable KindedSourcesMapType KindedSourcesMap;
  void ComputeKindedSources(KindedSources& files,
                            std::string const& config) const;

  mutable std::vector<AllConfigSource> AllConfigSources;
  void ComputeAllConfigSources() const;

  std::vector<TargetPropertyEntry*> IncludeDirectoriesEntries;
  std::vector<TargetPropertyEntry*> CompileOptionsEntries;
  std::vector<TargetPropertyEntry*> CompileFeaturesEntries;
  std::vector<TargetPropertyEntry*> CompileDefinitionsEntries;
  std::vector<TargetPropertyEntry*> SourceEntries;
  mutable std::set<std::string> LinkImplicitNullProperties;

  void ExpandLinkItems(std::string const& prop, std::string const& value,
                       std::string const& config,
                       const cmGeneratorTarget* headTarget,
                       bool usage_requirements_only,
                       std::vector<cmLinkItem>& items,
                       bool& hadHeadSensitiveCondition) const;
  void LookupLinkItems(std::vector<std::string> const& names,
                       std::vector<cmLinkItem>& items) const;

  void GetSourceFiles(std::vector<std::string>& files,
                      const std::string& config) const;

  struct HeadToLinkImplementationMap
    : public std::map<cmGeneratorTarget const*, cmOptionalLinkImplementation>
  {
  };
  typedef std::map<std::string, HeadToLinkImplementationMap> LinkImplMapType;
  mutable LinkImplMapType LinkImplMap;

  cmLinkImplementationLibraries const* GetLinkImplementationLibrariesInternal(
    const std::string& config, const cmGeneratorTarget* head) const;
  bool ComputeOutputDir(const std::string& config,
                        cmStateEnums::ArtifactType artifact,
                        std::string& out) const;

  typedef std::map<std::string, OutputInfo> OutputInfoMapType;
  mutable OutputInfoMapType OutputInfoMap;

  typedef std::map<std::string, ModuleDefinitionInfo>
    ModuleDefinitionInfoMapType;
  mutable ModuleDefinitionInfoMapType ModuleDefinitionInfoMap;
  void ComputeModuleDefinitionInfo(std::string const& config,
                                   ModuleDefinitionInfo& info) const;

  typedef std::pair<std::string, cmStateEnums::ArtifactType> OutputNameKey;
  typedef std::map<OutputNameKey, std::string> OutputNameMapType;
  mutable OutputNameMapType OutputNameMap;
  mutable std::set<cmLinkItem> UtilityItems;
  cmPolicies::PolicyMap PolicyMap;
  mutable bool PolicyWarnedCMP0022;
  mutable bool PolicyReportedCMP0069;
  mutable bool DebugIncludesDone;
  mutable bool DebugCompileOptionsDone;
  mutable bool DebugCompileFeaturesDone;
  mutable bool DebugCompileDefinitionsDone;
  mutable bool DebugSourcesDone;
  mutable bool LinkImplementationLanguageIsContextDependent;
  mutable bool UtilityItemsDone;
  bool DLLPlatform;

  bool ComputePDBOutputDir(const std::string& kind, const std::string& config,
                           std::string& out) const;

public:
  const std::vector<const cmGeneratorTarget*>& GetLinkImplementationClosure(
    const std::string& config) const;

  mutable std::map<std::string, std::string> MaxLanguageStandards;
  std::map<std::string, std::string> const& GetMaxLanguageStandards() const
  {
    return this->MaxLanguageStandards;
  }

  struct StrictTargetComparison
  {
    bool operator()(cmGeneratorTarget const* t1,
                    cmGeneratorTarget const* t2) const;
  };
};

#endif
