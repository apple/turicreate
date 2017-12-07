/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalVisualStudio7Generator_h
#define cmGlobalVisualStudio7Generator_h

#include "cmGlobalVisualStudioGenerator.h"

#include "cmGlobalGeneratorFactory.h"

class cmTarget;
struct cmIDEFlagTable;

/** \class cmGlobalVisualStudio7Generator
 * \brief Write a Unix makefiles.
 *
 * cmGlobalVisualStudio7Generator manages UNIX build process for a tree
 */
class cmGlobalVisualStudio7Generator : public cmGlobalVisualStudioGenerator
{
public:
  cmGlobalVisualStudio7Generator(cmake* cm,
                                 const std::string& platformName = "");
  ~cmGlobalVisualStudio7Generator();

  ///! Get the name for the platform.
  std::string const& GetPlatformName() const;

  ///! Create a local generator appropriate to this Global Generator
  virtual cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf);

  virtual bool SetSystemName(std::string const& s, cmMakefile* mf);

  virtual bool SetGeneratorPlatform(std::string const& p, cmMakefile* mf);

  /**
   * Utilized by the generator factory to determine if this generator
   * supports toolsets.
   */
  static bool SupportsToolset() { return false; }

  /**
   * Utilized by the generator factory to determine if this generator
   * supports platforms.
   */
  static bool SupportsPlatform() { return false; }

  /**
   * Try to determine system information such as shared library
   * extension, pthreads, byte order etc.
   */
  virtual void EnableLanguage(std::vector<std::string> const& languages,
                              cmMakefile*, bool optional);

  /**
   * Try running cmake and building a file. This is used for dynamically
   * loaded commands, not as part of the usual build process.
   */
  virtual void GenerateBuildCommand(
    std::vector<std::string>& makeCommand, const std::string& makeProgram,
    const std::string& projectName, const std::string& projectDir,
    const std::string& targetName, const std::string& config, bool fast,
    bool verbose,
    std::vector<std::string> const& makeOptions = std::vector<std::string>());

  /**
   * Generate the DSW workspace file.
   */
  virtual void OutputSLNFile();

  ///! Lookup a stored GUID or compute one deterministically.
  std::string GetGUID(std::string const& name);

  /** Append the subdirectory for the given configuration.  */
  virtual void AppendDirectoryForConfig(const std::string& prefix,
                                        const std::string& config,
                                        const std::string& suffix,
                                        std::string& dir);

  ///! What is the configurations directory variable called?
  virtual const char* GetCMakeCFGIntDir() const
  {
    return "$(ConfigurationName)";
  }

  /** Return true if the target project file should have the option
      LinkLibraryDependencies and link to .sln dependencies. */
  virtual bool NeedLinkLibraryDependencies(cmGeneratorTarget*)
  {
    return false;
  }

  const char* GetIntelProjectVersion();

  bool FindMakeProgram(cmMakefile* mf) CM_OVERRIDE;

  /** Is the Microsoft Assembler enabled?  */
  bool IsMasmEnabled() const { return this->MasmEnabled; }
  bool IsNasmEnabled() const { return this->NasmEnabled; }

  // Encoding for Visual Studio files
  virtual std::string Encoding();

  cmIDEFlagTable const* ExtraFlagTable;

protected:
  virtual void Generate();
  virtual const char* GetIDEVersion() = 0;

  std::string const& GetDevEnvCommand();
  virtual std::string FindDevEnvCommand();

  static const char* ExternalProjectType(const char* location);

  virtual void OutputSLNFile(cmLocalGenerator* root,
                             std::vector<cmLocalGenerator*>& generators);
  virtual void WriteSLNFile(std::ostream& fout, cmLocalGenerator* root,
                            std::vector<cmLocalGenerator*>& generators) = 0;
  virtual void WriteProject(std::ostream& fout, const std::string& name,
                            const char* path, const cmGeneratorTarget* t) = 0;
  virtual void WriteProjectDepends(std::ostream& fout, const std::string& name,
                                   const char* path,
                                   cmGeneratorTarget const* t) = 0;
  virtual void WriteProjectConfigurations(
    std::ostream& fout, const std::string& name,
    cmGeneratorTarget const& target, std::vector<std::string> const& configs,
    const std::set<std::string>& configsPartOfDefaultBuild,
    const std::string& platformMapping = "") = 0;
  virtual void WriteSLNGlobalSections(std::ostream& fout,
                                      cmLocalGenerator* root);
  virtual void WriteSLNFooter(std::ostream& fout);
  virtual void WriteSLNHeader(std::ostream& fout) = 0;
  virtual std::string WriteUtilityDepend(const cmGeneratorTarget* target);

  virtual void WriteTargetsToSolution(
    std::ostream& fout, cmLocalGenerator* root,
    OrderedTargetDependSet const& projectTargets);
  virtual void WriteTargetDepends(
    std::ostream& fout, OrderedTargetDependSet const& projectTargets);
  virtual void WriteTargetConfigurations(
    std::ostream& fout, std::vector<std::string> const& configs,
    OrderedTargetDependSet const& projectTargets);

  virtual void WriteExternalProject(
    std::ostream& fout, const std::string& name, const char* path,
    const char* typeGuid, const std::set<std::string>& dependencies) = 0;

  std::string ConvertToSolutionPath(const char* path);

  std::set<std::string> IsPartOfDefaultBuild(
    std::vector<std::string> const& configs,
    OrderedTargetDependSet const& projectTargets,
    cmGeneratorTarget const* target);
  bool IsDependedOn(OrderedTargetDependSet const& projectTargets,
                    cmGeneratorTarget const* target);
  std::map<std::string, std::string> GUIDMap;

  virtual void WriteFolders(std::ostream& fout);
  virtual void WriteFoldersContent(std::ostream& fout);
  std::map<std::string, std::set<std::string> > VisualStudioFolders;

  // Set during OutputSLNFile with the name of the current project.
  // There is one SLN file per project.
  std::string CurrentProject;
  std::string GeneratorPlatform;
  std::string DefaultPlatformName;
  bool MasmEnabled;
  bool NasmEnabled;

private:
  char* IntelProjectVersion;
  std::string DevEnvCommand;
  bool DevEnvCommandInitialized;
  virtual std::string GetVSMakeProgram() { return this->GetDevEnvCommand(); }
};

#define CMAKE_CHECK_BUILD_SYSTEM_TARGET "ZERO_CHECK"

#endif
