/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmLocalGenerator_h
#define cmLocalGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_kwiml.h"
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "cmListFileCache.h"
#include "cmOutputConverter.h"
#include "cmPolicies.h"
#include "cmStateSnapshot.h"
#include "cmake.h"

class cmComputeLinkInformation;
class cmCustomCommandGenerator;
class cmGeneratorTarget;
class cmGlobalGenerator;
class cmLinkLineComputer;
class cmMakefile;
class cmRulePlaceholderExpander;
class cmSourceFile;
class cmState;

/** \class cmLocalGenerator
 * \brief Create required build files for a directory.
 *
 * Subclasses of this abstract class generate makefiles, DSP, etc for various
 * platforms. This class should never be constructed directly. A
 * GlobalGenerator will create it and invoke the appropriate commands on it.
 */
class cmLocalGenerator : public cmOutputConverter
{
public:
  cmLocalGenerator(cmGlobalGenerator* gg, cmMakefile* makefile);
  virtual ~cmLocalGenerator();

  /**
   * Generate the makefile for this directory.
   */
  virtual void Generate() {}

  virtual void ComputeHomeRelativeOutputPath() {}

  /**
   * Calls TraceVSDependencies() on all targets of this generator.
   */
  void TraceDependencies();

  virtual void AddHelperCommands() {}

  /**
   * Generate the install rules files in this directory.
   */
  void GenerateInstallRules();

  /**
   * Generate the test files for tests.
   */
  void GenerateTestFiles();

  /**
   * Generate a manifest of target files that will be built.
   */
  void ComputeTargetManifest();

  bool ComputeTargetCompileFeatures();

  bool IsRootMakefile() const;

  ///! Get the makefile for this generator
  cmMakefile* GetMakefile() { return this->Makefile; }

  ///! Get the makefile for this generator, const version
  const cmMakefile* GetMakefile() const { return this->Makefile; }

  ///! Get the GlobalGenerator this is associated with
  cmGlobalGenerator* GetGlobalGenerator() { return this->GlobalGenerator; }
  const cmGlobalGenerator* GetGlobalGenerator() const
  {
    return this->GlobalGenerator;
  }

  virtual cmRulePlaceholderExpander* CreateRulePlaceholderExpander() const;

  std::string GetLinkLibsCMP0065(std::string const& linkLanguage,
                                 cmGeneratorTarget& tgt) const;

  cmState* GetState() const;
  cmStateSnapshot GetStateSnapshot() const;

  void AddArchitectureFlags(std::string& flags,
                            cmGeneratorTarget const* target,
                            const std::string& lang,
                            const std::string& config);

  void AddLanguageFlags(std::string& flags, cmGeneratorTarget const* target,
                        const std::string& lang, const std::string& config);
  void AddLanguageFlagsForLinking(std::string& flags,
                                  cmGeneratorTarget const* target,
                                  const std::string& lang,
                                  const std::string& config);
  void AddCMP0018Flags(std::string& flags, cmGeneratorTarget const* target,
                       std::string const& lang, const std::string& config);
  void AddVisibilityPresetFlags(std::string& flags,
                                cmGeneratorTarget const* target,
                                const std::string& lang);
  void AddConfigVariableFlags(std::string& flags, const std::string& var,
                              const std::string& config);
  void AddCompilerRequirementFlag(std::string& flags,
                                  cmGeneratorTarget const* target,
                                  const std::string& lang);
  ///! Append flags to a string.
  virtual void AppendFlags(std::string& flags,
                           const std::string& newFlags) const;
  virtual void AppendFlags(std::string& flags, const char* newFlags) const;
  virtual void AppendFlagEscape(std::string& flags,
                                const std::string& rawFlag) const;
  void AppendIPOLinkerFlags(std::string& flags, cmGeneratorTarget* target,
                            const std::string& config,
                            const std::string& lang);
  ///! Get the include flags for the current makefile and language
  std::string GetIncludeFlags(const std::vector<std::string>& includes,
                              cmGeneratorTarget* target,
                              const std::string& lang,
                              bool forceFullPaths = false,
                              bool forResponseFile = false,
                              const std::string& config = "");

  const std::vector<cmGeneratorTarget*>& GetGeneratorTargets() const
  {
    return this->GeneratorTargets;
  }

  void AddGeneratorTarget(cmGeneratorTarget* gt);
  void AddImportedGeneratorTarget(cmGeneratorTarget* gt);
  void AddOwnedImportedGeneratorTarget(cmGeneratorTarget* gt);

  cmGeneratorTarget* FindLocalNonAliasGeneratorTarget(
    const std::string& name) const;
  cmGeneratorTarget* FindGeneratorTargetToUse(const std::string& name) const;

  /**
   * Process a list of include directories
   */
  void AppendIncludeDirectories(std::vector<std::string>& includes,
                                const char* includes_list,
                                const cmSourceFile& sourceFile) const;
  void AppendIncludeDirectories(std::vector<std::string>& includes,
                                std::string const& includes_list,
                                const cmSourceFile& sourceFile) const
  {
    this->AppendIncludeDirectories(includes, includes_list.c_str(),
                                   sourceFile);
  }
  void AppendIncludeDirectories(std::vector<std::string>& includes,
                                const std::vector<std::string>& includes_vec,
                                const cmSourceFile& sourceFile) const;

  /**
   * Encode a list of preprocessor definitions for the compiler
   * command line.
   */
  void AppendDefines(std::set<std::string>& defines,
                     const char* defines_list) const;
  void AppendDefines(std::set<std::string>& defines,
                     std::string const& defines_list) const
  {
    this->AppendDefines(defines, defines_list.c_str());
  }
  void AppendDefines(std::set<std::string>& defines,
                     const std::vector<std::string>& defines_vec) const;

  /**
   * Encode a list of compile options for the compiler
   * command line.
   */
  void AppendCompileOptions(std::string& options, const char* options_list,
                            const char* regex = nullptr) const;
  void AppendCompileOptions(std::string& options,
                            std::string const& options_list,
                            const char* regex = nullptr) const
  {
    this->AppendCompileOptions(options, options_list.c_str(), regex);
  }
  void AppendCompileOptions(std::string& options,
                            const std::vector<std::string>& options_vec,
                            const char* regex = nullptr) const;

  /**
   * Join a set of defines into a definesString with a space separator.
   */
  void JoinDefines(const std::set<std::string>& defines,
                   std::string& definesString, const std::string& lang);

  /** Lookup and append options associated with a particular feature.  */
  void AppendFeatureOptions(std::string& flags, const std::string& lang,
                            const char* feature);

  const char* GetFeature(const std::string& feature,
                         const std::string& config);

  /** \brief Get absolute path to dependency \a name
   *
   * Translate a dependency as given in CMake code to the name to
   * appear in a generated build file.
   * - If \a name is a utility target, returns false.
   * - If \a name is a CMake target, it will be transformed to the real output
   *   location of that target for the given configuration.
   * - If \a name is the full path to a file, it will be returned.
   * - Otherwise \a name is treated as a relative path with respect to
   *   the source directory of this generator.  This should only be
   *   used for dependencies of custom commands.
   */
  bool GetRealDependency(const std::string& name, const std::string& config,
                         std::string& dep);

  virtual std::string ConvertToIncludeReference(
    std::string const& path,
    cmOutputConverter::OutputFormat format = cmOutputConverter::SHELL,
    bool forceFullPaths = false);

  /** Called from command-line hook to clear dependencies.  */
  virtual void ClearDependencies(cmMakefile* /* mf */, bool /* verbose */) {}

  /** Called from command-line hook to update dependencies.  */
  virtual bool UpdateDependencies(const char* /* tgtInfo */, bool /*verbose*/,
                                  bool /*color*/)
  {
    return true;
  }

  /** @brief Get the include directories for the current makefile and language.
   * @arg stripImplicitDirs Strip all directories found in
   *      CMAKE_<LANG>_IMPLICIT_INCLUDE_DIRECTORIES from the result.
   * @arg appendAllImplicitDirs Append all directories found in
   *      CMAKE_<LANG>_IMPLICIT_INCLUDE_DIRECTORIES to the result.
   */
  void GetIncludeDirectories(std::vector<std::string>& dirs,
                             cmGeneratorTarget const* target,
                             const std::string& lang = "C",
                             const std::string& config = "",
                             bool stripImplicitDirs = true,
                             bool appendAllImplicitDirs = false) const;
  void AddCompileOptions(std::string& flags, cmGeneratorTarget* target,
                         const std::string& lang, const std::string& config);
  void AddCompileDefinitions(std::set<std::string>& defines,
                             cmGeneratorTarget const* target,
                             const std::string& config,
                             const std::string& lang) const;

  std::string GetProjectName() const;

  /** Compute the language used to compile the given source file.  */
  std::string GetSourceFileLanguage(const cmSourceFile& source);

  // Fill the vector with the target names for the object files,
  // preprocessed files and assembly files.
  void GetIndividualFileTargets(std::vector<std::string>&) {}

  /**
   * Get the relative path from the generator output directory to a
   * per-target support directory.
   */
  virtual std::string GetTargetDirectory(
    cmGeneratorTarget const* target) const;

  /**
   * Get the level of backwards compatibility requested by the project
   * in this directory.  This is the value of the CMake variable
   * CMAKE_BACKWARDS_COMPATIBILITY whose format is
   * "major.minor[.patch]".  The returned integer is encoded as
   *
   *   CMake_VERSION_ENCODE(major, minor, patch)
   *
   * and is monotonically increasing with the CMake version.
   */
  KWIML_INT_uint64_t GetBackwardsCompatibility();

  /**
   * Test whether compatibility is set to a given version or lower.
   */
  bool NeedBackwardsCompatibility_2_4();

  cmPolicies::PolicyStatus GetPolicyStatus(cmPolicies::PolicyID id) const;

  cmake* GetCMakeInstance() const;

  std::string const& GetSourceDirectory() const;
  std::string const& GetBinaryDirectory() const;

  std::string const& GetCurrentBinaryDirectory() const;
  std::string const& GetCurrentSourceDirectory() const;

  /**
   * Generate a macOS application bundle Info.plist file.
   */
  void GenerateAppleInfoPList(cmGeneratorTarget* target,
                              const std::string& targetName,
                              const char* fname);

  /**
   * Generate a macOS framework Info.plist file.
   */
  void GenerateFrameworkInfoPList(cmGeneratorTarget* target,
                                  const std::string& targetName,
                                  const char* fname);
  /** Construct a comment for a custom command.  */
  std::string ConstructComment(cmCustomCommandGenerator const& ccg,
                               const char* default_comment = "");
  // Compute object file names.
  std::string GetObjectFileNameWithoutTarget(
    const cmSourceFile& source, std::string const& dir_max,
    bool* hasSourceExtension = nullptr,
    char const* customOutputExtension = nullptr);

  /** Fill out the static linker flags for the given target.  */
  void GetStaticLibraryFlags(std::string& flags, std::string const& config,
                             std::string const& linkLanguage,
                             cmGeneratorTarget* target);

  /** Fill out these strings for the given target.  Libraries to link,
   *  flags, and linkflags. */
  void GetTargetFlags(cmLinkLineComputer* linkLineComputer,
                      const std::string& config, std::string& linkLibs,
                      std::string& flags, std::string& linkFlags,
                      std::string& frameworkPath, std::string& linkPath,
                      cmGeneratorTarget* target);
  void GetTargetDefines(cmGeneratorTarget const* target,
                        std::string const& config, std::string const& lang,
                        std::set<std::string>& defines) const;
  void GetTargetCompileFlags(cmGeneratorTarget* target,
                             std::string const& config,
                             std::string const& lang, std::string& flags);

  std::string GetFrameworkFlags(std::string const& l,
                                std::string const& config,
                                cmGeneratorTarget* target);
  virtual std::string GetTargetFortranFlags(cmGeneratorTarget const* target,
                                            std::string const& config);

  virtual void ComputeObjectFilenames(
    std::map<cmSourceFile const*, std::string>& mapping,
    cmGeneratorTarget const* gt = nullptr);

  bool IsWindowsShell() const;
  bool IsWatcomWMake() const;
  bool IsMinGWMake() const;
  bool IsNMake() const;

  void IssueMessage(cmake::MessageType t, std::string const& text) const;

  void CreateEvaluationFileOutputs(const std::string& config);
  void ProcessEvaluationFiles(std::vector<std::string>& generatedFiles);

  const char* GetRuleLauncher(cmGeneratorTarget* target,
                              const std::string& prop);

protected:
  ///! put all the libraries for a target on into the given stream
  void OutputLinkLibraries(cmComputeLinkInformation* pcli,
                           cmLinkLineComputer* linkLineComputer,
                           std::string& linkLibraries,
                           std::string& frameworkPath, std::string& linkPath);

  // Handle old-style install rules stored in the targets.
  void GenerateTargetInstallRules(
    std::ostream& os, const std::string& config,
    std::vector<std::string> const& configurationTypes);

  std::string& CreateSafeUniqueObjectFileName(const std::string& sin,
                                              std::string const& dir_max);

  /** Check whether the native build system supports the given
      definition.  Issues a warning.  */
  virtual bool CheckDefinition(std::string const& define) const;

  cmMakefile* Makefile;
  cmStateSnapshot StateSnapshot;
  cmListFileBacktrace DirectoryBacktrace;
  cmGlobalGenerator* GlobalGenerator;
  std::map<std::string, std::string> UniqueObjectNamesMap;
  std::string::size_type ObjectPathMax;
  std::set<std::string> ObjectMaxPathViolations;

  typedef std::unordered_map<std::string, cmGeneratorTarget*>
    GeneratorTargetMap;
  GeneratorTargetMap GeneratorTargetSearchIndex;
  std::vector<cmGeneratorTarget*> GeneratorTargets;

  std::set<cmGeneratorTarget const*> WarnCMP0063;
  GeneratorTargetMap ImportedGeneratorTargets;
  std::vector<cmGeneratorTarget*> OwnedImportedGeneratorTargets;
  std::map<std::string, std::string> AliasTargets;

  std::map<std::string, std::string> Compilers;
  std::map<std::string, std::string> VariableMappings;
  std::string CompilerSysroot;
  std::string LinkerSysroot;

  bool EmitUniversalBinaryFlags;

  KWIML_INT_uint64_t BackwardsCompatibility;
  bool BackwardsCompatibilityFinal;

private:
  void AddSharedFlags(std::string& flags, const std::string& lang,
                      bool shared);
  bool GetShouldUseOldFlags(bool shared, const std::string& lang) const;
  void AddPositionIndependentFlags(std::string& flags, std::string const& l,
                                   int targetType);

  void ComputeObjectMaxPath();
  void MoveSystemIncludesToEnd(std::vector<std::string>& includeDirs,
                               const std::string& config,
                               const std::string& lang,
                               cmGeneratorTarget const* target) const;
};

#if defined(CMAKE_BUILD_WITH_CMAKE)
bool cmLocalGeneratorCheckObjectName(std::string& objName,
                                     std::string::size_type dir_len,
                                     std::string::size_type max_total_len);
#endif

#endif
