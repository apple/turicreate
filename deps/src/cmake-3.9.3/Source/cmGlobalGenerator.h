/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalGenerator_h
#define cmGlobalGenerator_h

#include "cmConfigure.h"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "cmCustomCommandLines.h"
#include "cmExportSetMap.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetDepend.h"
#include "cm_codecvt.hxx"
#include "cm_unordered_map.hxx"

#if defined(CMAKE_BUILD_WITH_CMAKE)
#include "cmFileLockPool.h"
#endif

class cmExportBuildFileGenerator;
class cmExternalMakefileProjectGenerator;
class cmGeneratorTarget;
class cmLinkLineComputer;
class cmLocalGenerator;
class cmMakefile;
class cmOutputConverter;
class cmSourceFile;
class cmStateDirectory;
class cmake;

/** \class cmGlobalGenerator
 * \brief Responsible for overseeing the generation process for the entire tree
 *
 * Subclasses of this class generate makefiles for various
 * platforms.
 */
class cmGlobalGenerator
{
public:
  ///! Free any memory allocated with the GlobalGenerator
  cmGlobalGenerator(cmake* cm);
  virtual ~cmGlobalGenerator();

  virtual cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf);

  ///! Get the name for this generator
  virtual std::string GetName() const { return "Generic"; }

  /** Check whether the given name matches the current generator.  */
  virtual bool MatchesGeneratorName(const std::string& name) const
  {
    return this->GetName() == name;
  }

  /** Get encoding used by generator for makefile files */
  virtual codecvt::Encoding GetMakefileEncoding() const
  {
    return codecvt::None;
  }

  /** Tell the generator about the target system.  */
  virtual bool SetSystemName(std::string const&, cmMakefile*) { return true; }

  /** Set the generator-specific platform name.  Returns true if platform
      is supported and false otherwise.  */
  virtual bool SetGeneratorPlatform(std::string const& p, cmMakefile* mf);

  /** Set the generator-specific toolset name.  Returns true if toolset
      is supported and false otherwise.  */
  virtual bool SetGeneratorToolset(std::string const& ts, cmMakefile* mf);

  /**
   * Create LocalGenerators and process the CMakeLists files. This does not
   * actually produce any makefiles, DSPs, etc.
   */
  virtual void Configure();

  bool Compute();
  virtual void AddExtraIDETargets() {}

  enum TargetTypes
  {
    AllTargets,
    ImportedOnly
  };

  void CreateImportedGenerationObjects(
    cmMakefile* mf, std::vector<std::string> const& targets,
    std::vector<cmGeneratorTarget const*>& exports);
  void CreateGenerationObjects(TargetTypes targetTypes = AllTargets);

  /**
   * Generate the all required files for building this project/tree. This
   * basically creates a series of LocalGenerators for each directory and
   * requests that they Generate.
   */
  virtual void Generate();

  virtual cmLinkLineComputer* CreateLinkLineComputer(
    cmOutputConverter* outputConverter,
    cmStateDirectory const& stateDir) const;

  cmLinkLineComputer* CreateMSVC60LinkLineComputer(
    cmOutputConverter* outputConverter,
    cmStateDirectory const& stateDir) const;

  /**
   * Set/Get and Clear the enabled languages.
   */
  void SetLanguageEnabled(const std::string&, cmMakefile* mf);
  bool GetLanguageEnabled(const std::string&) const;
  void ClearEnabledLanguages();
  void GetEnabledLanguages(std::vector<std::string>& lang) const;
  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);

  /**
   * Resolve the CMAKE_<lang>_COMPILER setting for the given language.
   * Intended to be called from EnableLanguage.
   */
  void ResolveLanguageCompiler(const std::string& lang, cmMakefile* mf,
                               bool optional) const;

  /**
   * Try to determine system information, get it from another generator
   */
  void EnableLanguagesFromGenerator(cmGlobalGenerator* gen, cmMakefile* mf);

  /**
   * Try running cmake and building a file. This is used for dynamically
   * loaded commands, not as part of the usual build process.
   */
  int TryCompile(const std::string& srcdir, const std::string& bindir,
                 const std::string& projectName, const std::string& targetName,
                 bool fast, std::string& output, cmMakefile* mf);

  /**
   * Build a file given the following information. This is a more direct call
   * that is used by both CTest and TryCompile. If target name is NULL or
   * empty then all is assumed. clean indicates if a "make clean" should be
   * done first.
   */
  int Build(const std::string& srcdir, const std::string& bindir,
            const std::string& projectName, const std::string& targetName,
            std::string& output, const std::string& makeProgram,
            const std::string& config, bool clean, bool fast, bool verbose,
            double timeout, cmSystemTools::OutputOption outputflag =
                              cmSystemTools::OUTPUT_NONE,
            std::vector<std::string> const& nativeOptions =
              std::vector<std::string>());

  virtual void GenerateBuildCommand(
    std::vector<std::string>& makeCommand, const std::string& makeProgram,
    const std::string& projectName, const std::string& projectDir,
    const std::string& targetName, const std::string& config, bool fast,
    bool verbose,
    std::vector<std::string> const& makeOptions = std::vector<std::string>());

  /** Generate a "cmake --build" call for a given target and config.  */
  std::string GenerateCMakeBuildCommand(const std::string& target,
                                        const std::string& config,
                                        const std::string& native,
                                        bool ignoreErrors);

  ///! Get the CMake instance
  cmake* GetCMakeInstance() const { return this->CMakeInstance; }

  void SetConfiguredFilesPath(cmGlobalGenerator* gen);
  const std::vector<cmMakefile*>& GetMakefiles() const
  {
    return this->Makefiles;
  }
  const std::vector<cmLocalGenerator*>& GetLocalGenerators() const
  {
    return this->LocalGenerators;
  }

  cmMakefile* GetCurrentMakefile() const { return this->CurrentMakefile; }

  void SetCurrentMakefile(cmMakefile* mf) { this->CurrentMakefile = mf; }

  void AddMakefile(cmMakefile* mf);

  ///! Set an generator for an "external makefile based project"
  void SetExternalMakefileProjectGenerator(
    cmExternalMakefileProjectGenerator* extraGenerator);

  std::string GetExtraGeneratorName() const;

  void AddInstallComponent(const char* component);

  const std::set<std::string>* GetInstallComponents() const
  {
    return &this->InstallComponents;
  }

  cmExportSetMap& GetExportSets() { return this->ExportSets; }

  const char* GetGlobalSetting(std::string const& name) const;
  bool GlobalSettingIsOn(std::string const& name) const;
  const char* GetSafeGlobalSetting(std::string const& name) const;

  /** Add a file to the manifest of generated targets for a configuration.  */
  void AddToManifest(std::string const& f);

  void EnableInstallTarget();

  int TryCompileTimeout;

  bool GetForceUnixPaths() const { return this->ForceUnixPaths; }
  bool GetToolSupportsColor() const { return this->ToolSupportsColor; }

  ///! return the language for the given extension
  std::string GetLanguageFromExtension(const char* ext) const;
  ///! is an extension to be ignored
  bool IgnoreFile(const char* ext) const;
  ///! What is the preference for linkers and this language (None or Preferred)
  int GetLinkerPreference(const std::string& lang) const;
  ///! What is the object file extension for a given source file?
  std::string GetLanguageOutputExtension(cmSourceFile const&) const;

  ///! What is the configurations directory variable called?
  virtual const char* GetCMakeCFGIntDir() const { return "."; }

  ///! expand CFGIntDir for a configuration
  virtual std::string ExpandCFGIntDir(const std::string& str,
                                      const std::string& config) const;

  /** Get whether the generator should use a script for link commands.  */
  bool GetUseLinkScript() const { return this->UseLinkScript; }

  /** Get whether the generator should produce special marks on rules
      producing symbolic (non-file) outputs.  */
  bool GetNeedSymbolicMark() const { return this->NeedSymbolicMark; }

  /*
   * Determine what program to use for building the project.
   */
  virtual bool FindMakeProgram(cmMakefile*);

  ///! Find a target by name by searching the local generators.
  cmTarget* FindTarget(const std::string& name,
                       bool excludeAliases = false) const;

  cmGeneratorTarget* FindGeneratorTarget(const std::string& name) const;

  void AddAlias(const std::string& name, const std::string& tgtName);
  bool IsAlias(const std::string& name) const;

  /** Determine if a name resolves to a framework on disk or a built target
      that is a framework. */
  bool NameResolvesToFramework(const std::string& libname) const;

  cmMakefile* FindMakefile(const std::string& start_dir) const;
  ///! Find a local generator by its startdirectory
  cmLocalGenerator* FindLocalGenerator(const std::string& start_dir) const;

  /** Append the subdirectory for the given configuration.  If anything is
      appended the given prefix and suffix will be appended around it, which
      is useful for leading or trailing slashes.  */
  virtual void AppendDirectoryForConfig(const std::string& prefix,
                                        const std::string& config,
                                        const std::string& suffix,
                                        std::string& dir);

  /** Get the content of a directory.  Directory listings are cached
      and re-loaded from disk only when modified.  During the generation
      step the content will include the target files to be built even if
      they do not yet exist.  */
  std::set<std::string> const& GetDirectoryContent(std::string const& dir,
                                                   bool needDisk = true);

  void IndexTarget(cmTarget* t);
  void IndexGeneratorTarget(cmGeneratorTarget* gt);

  static bool IsReservedTarget(std::string const& name);

  virtual const char* GetAllTargetName() const { return "ALL_BUILD"; }
  virtual const char* GetInstallTargetName() const { return "INSTALL"; }
  virtual const char* GetInstallLocalTargetName() const { return CM_NULLPTR; }
  virtual const char* GetInstallStripTargetName() const { return CM_NULLPTR; }
  virtual const char* GetPreinstallTargetName() const { return CM_NULLPTR; }
  virtual const char* GetTestTargetName() const { return "RUN_TESTS"; }
  virtual const char* GetPackageTargetName() const { return "PACKAGE"; }
  virtual const char* GetPackageSourceTargetName() const { return CM_NULLPTR; }
  virtual const char* GetEditCacheTargetName() const { return CM_NULLPTR; }
  virtual const char* GetRebuildCacheTargetName() const { return CM_NULLPTR; }
  virtual const char* GetCleanTargetName() const { return CM_NULLPTR; }

  // Lookup edit_cache target command preferred by this generator.
  virtual std::string GetEditCacheCommand() const { return ""; }

  // Class to track a set of dependencies.
  typedef cmTargetDependSet TargetDependSet;

  // what targets does the specified target depend on directly
  // via a target_link_libraries or add_dependencies
  TargetDependSet const& GetTargetDirectDepends(
    const cmGeneratorTarget* target);

  const std::map<std::string, std::vector<cmLocalGenerator*> >& GetProjectMap()
    const
  {
    return this->ProjectMap;
  }

  // track files replaced during a Generate
  void FileReplacedDuringGenerate(const std::string& filename);
  void GetFilesReplacedDuringGenerate(std::vector<std::string>& filenames);

  void AddRuleHash(const std::vector<std::string>& outputs,
                   std::string const& content);

  /** Return whether the given binary directory is unused.  */
  bool BinaryDirectoryIsNew(const std::string& dir)
  {
    return this->BinaryDirectories.insert(dir).second;
  }

  /** Return true if the generated build tree may contain multiple builds.
      i.e. "Can I build Debug and Release in the same tree?" */
  virtual bool IsMultiConfig() const { return false; }

  /** Return true if we know the exact location of object files.
      If false, store the reason in the given string.
      This is meaningful only after EnableLanguage has been called.  */
  virtual bool HasKnownObjectFileLocation(std::string*) const { return true; }

  virtual bool UseFolderProperty() const;

  virtual bool IsIPOSupported() const { return false; }

  /** Return whether the generator should use EFFECTIVE_PLATFORM_NAME. This is
      relevant for mixed macOS and iOS builds. */
  virtual bool UseEffectivePlatformName(cmMakefile*) const { return false; }

  /** Return whether the "Resources" folder prefix should be stripped from
      MacFolder. */
  virtual bool ShouldStripResourcePath(cmMakefile*) const;

  std::string GetSharedLibFlagsForLanguage(std::string const& lang) const;

  /** Generate an <output>.rule file path for a given command output.  */
  virtual std::string GenerateRuleFile(std::string const& output) const;

  static std::string EscapeJSON(const std::string& s);

  void ProcessEvaluationFiles();

  std::map<std::string, cmExportBuildFileGenerator*>& GetBuildExportSets()
  {
    return this->BuildExportSets;
  }
  void AddBuildExportSet(cmExportBuildFileGenerator*);
  void AddBuildExportExportSet(cmExportBuildFileGenerator*);
  bool IsExportedTargetsFile(const std::string& filename) const;
  bool GenerateImportFile(const std::string& file);
  cmExportBuildFileGenerator* GetExportedTargetsFile(
    const std::string& filename) const;
  void AddCMP0042WarnTarget(const std::string& target);
  void AddCMP0068WarnTarget(const std::string& target);

  virtual void ComputeTargetObjectDirectory(cmGeneratorTarget* gt) const;

  bool GenerateCPackPropertiesFile();

  void CreateEvaluationSourceFiles(std::string const& config) const;

  void SetFilenameTargetDepends(
    cmSourceFile* sf, std::set<cmGeneratorTarget const*> const& tgts);
  const std::set<const cmGeneratorTarget*>& GetFilenameTargetDepends(
    cmSourceFile* sf) const;

#if defined(CMAKE_BUILD_WITH_CMAKE)
  cmFileLockPool& GetFileLockPool() { return FileLockPool; }
#endif

  bool GetConfigureDoneCMP0026() const
  {
    return this->ConfigureDoneCMP0026AndCMP0024;
  }

  std::string MakeSilentFlag;

protected:
  typedef std::vector<cmLocalGenerator*> GeneratorVector;
  // for a project collect all its targets by following depend
  // information, and also collect all the targets
  void GetTargetSets(TargetDependSet& projectTargets,
                     TargetDependSet& originalTargets, cmLocalGenerator* root,
                     GeneratorVector const&);
  bool IsRootOnlyTarget(cmGeneratorTarget* target) const;
  void AddTargetDepends(const cmGeneratorTarget* target,
                        TargetDependSet& projectTargets);
  void SetLanguageEnabledFlag(const std::string& l, cmMakefile* mf);
  void SetLanguageEnabledMaps(const std::string& l, cmMakefile* mf);
  void FillExtensionToLanguageMap(const std::string& l, cmMakefile* mf);
  virtual bool CheckLanguages(std::vector<std::string> const& languages,
                              cmMakefile* mf) const;
  virtual void PrintCompilerAdvice(std::ostream& os, std::string const& lang,
                                   const char* envVar) const;

  virtual bool ComputeTargetDepends();

  virtual bool CheckALLOW_DUPLICATE_CUSTOM_TARGETS() const;

  std::vector<const cmGeneratorTarget*> CreateQtAutoGeneratorsTargets();

  std::string SelectMakeProgram(const std::string& makeProgram,
                                const std::string& makeDefault = "") const;

  // Fill the ProjectMap, this must be called after LocalGenerators
  // has been populated.
  void FillProjectMap();
  void CheckTargetProperties();
  bool IsExcluded(cmStateSnapshot const& root,
                  cmStateSnapshot const& snp) const;
  bool IsExcluded(cmLocalGenerator* root, cmLocalGenerator* gen) const;
  bool IsExcluded(cmLocalGenerator* root, cmGeneratorTarget* target) const;
  virtual void InitializeProgressMarks() {}

  struct GlobalTargetInfo
  {
    std::string Name;
    std::string Message;
    cmCustomCommandLines CommandLines;
    std::vector<std::string> Depends;
    std::string WorkingDir;
    bool UsesTerminal;
    GlobalTargetInfo()
      : UsesTerminal(false)
    {
    }
  };

  void CreateDefaultGlobalTargets(std::vector<GlobalTargetInfo>& targets);

  void AddGlobalTarget_Package(std::vector<GlobalTargetInfo>& targets);
  void AddGlobalTarget_PackageSource(std::vector<GlobalTargetInfo>& targets);
  void AddGlobalTarget_Test(std::vector<GlobalTargetInfo>& targets);
  void AddGlobalTarget_EditCache(std::vector<GlobalTargetInfo>& targets);
  void AddGlobalTarget_RebuildCache(std::vector<GlobalTargetInfo>& targets);
  void AddGlobalTarget_Install(std::vector<GlobalTargetInfo>& targets);
  cmTarget CreateGlobalTarget(GlobalTargetInfo const& gti, cmMakefile* mf);

  std::string FindMakeProgramFile;
  std::string ConfiguredFilesPath;
  cmake* CMakeInstance;
  std::vector<cmMakefile*> Makefiles;
  std::vector<cmLocalGenerator*> LocalGenerators;
  cmMakefile* CurrentMakefile;
  // map from project name to vector of local generators in that project
  std::map<std::string, std::vector<cmLocalGenerator*> > ProjectMap;

  // Set of named installation components requested by the project.
  std::set<std::string> InstallComponents;
  // Sets of named target exports
  cmExportSetMap ExportSets;
  std::map<std::string, cmExportBuildFileGenerator*> BuildExportSets;
  std::map<std::string, cmExportBuildFileGenerator*> BuildExportExportSets;

  std::map<std::string, std::string> AliasTargets;

  cmTarget* FindTargetImpl(std::string const& name) const;

  cmGeneratorTarget* FindGeneratorTargetImpl(std::string const& name) const;
  cmGeneratorTarget* FindImportedGeneratorTargetImpl(
    std::string const& name) const;

  const char* GetPredefinedTargetsFolder();

private:
  typedef CM_UNORDERED_MAP<std::string, cmTarget*> TargetMap;
  typedef CM_UNORDERED_MAP<std::string, cmGeneratorTarget*> GeneratorTargetMap;
  typedef CM_UNORDERED_MAP<std::string, cmMakefile*> MakefileMap;
  // Map efficiently from target name to cmTarget instance.
  // Do not use this structure for looping over all targets.
  // It contains both normal and globally visible imported targets.
  TargetMap TargetSearchIndex;
  GeneratorTargetMap GeneratorTargetSearchIndex;

  // Map efficiently from source directory path to cmMakefile instance.
  // Do not use this structure for looping over all directories.
  // It may not contain all of them (see note in IndexMakefile method).
  MakefileMap MakefileSearchIndex;

  cmMakefile* TryCompileOuterMakefile;
  // If you add a new map here, make sure it is copied
  // in EnableLanguagesFromGenerator
  std::map<std::string, bool> IgnoreExtensions;
  std::set<std::string> LanguagesReady; // Ready for try_compile
  std::set<std::string> LanguagesInProgress;
  std::map<std::string, std::string> OutputExtensions;
  std::map<std::string, std::string> LanguageToOutputExtension;
  std::map<std::string, std::string> ExtensionToLanguage;
  std::map<std::string, int> LanguageToLinkerPreference;
  std::map<std::string, std::string> LanguageToOriginalSharedLibFlags;

  // Record hashes for rules and outputs.
  struct RuleHash
  {
    char Data[32];
  };
  std::map<std::string, RuleHash> RuleHashes;
  void CheckRuleHashes();
  void CheckRuleHashes(std::string const& pfile, std::string const& home);
  void WriteRuleHashes(std::string const& pfile);

  void WriteSummary();
  void WriteSummary(cmGeneratorTarget* target);
  void FinalizeTargetCompileInfo();

  virtual void ForceLinkerLanguages();

  void CreateLocalGenerators();

  void CheckCompilerIdCompatibility(cmMakefile* mf,
                                    std::string const& lang) const;

  void ComputeBuildFileGenerators();

  cmExternalMakefileProjectGenerator* ExtraGenerator;

  // track files replaced during a Generate
  std::vector<std::string> FilesReplacedDuringGenerate;

  // Store computed inter-target dependencies.
  typedef std::map<cmGeneratorTarget const*, TargetDependSet> TargetDependMap;
  TargetDependMap TargetDependencies;

  friend class cmake;
  void CreateGeneratorTargets(
    TargetTypes targetTypes, cmMakefile* mf, cmLocalGenerator* lg,
    std::map<cmTarget*, cmGeneratorTarget*> const& importedMap);
  void CreateGeneratorTargets(TargetTypes targetTypes);

  void ClearGeneratorMembers();

  void IndexMakefile(cmMakefile* mf);

  virtual const char* GetBuildIgnoreErrorsFlag() const { return CM_NULLPTR; }

  // Cache directory content and target files to be built.
  struct DirectoryContent
  {
    long LastDiskTime;
    std::set<std::string> All;
    std::set<std::string> Generated;
    DirectoryContent()
      : LastDiskTime(-1)
    {
    }
  };
  std::map<std::string, DirectoryContent> DirectoryContentMap;

  // Set of binary directories on disk.
  std::set<std::string> BinaryDirectories;

  // track targets to issue CMP0042 warning for.
  std::set<std::string> CMP0042WarnTargets;
  // track targets to issue CMP0068 warning for.
  std::set<std::string> CMP0068WarnTargets;

  mutable std::map<cmSourceFile*, std::set<cmGeneratorTarget const*> >
    FilenameTargetDepends;

#if defined(CMAKE_BUILD_WITH_CMAKE)
  // Pool of file locks
  cmFileLockPool FileLockPool;
#endif

protected:
  float FirstTimeProgress;
  bool NeedSymbolicMark;
  bool UseLinkScript;
  bool ForceUnixPaths;
  bool ToolSupportsColor;
  bool InstallTargetEnabled;
  bool ConfigureDoneCMP0026AndCMP0024;
};

#endif
