/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGhsMultiGenerator_h
#define cmGhsMultiGenerator_h

#include "cmGlobalGenerator.h"

#include "cmGhsMultiGpj.h"
#include "cmGlobalGeneratorFactory.h"

class cmGeneratedFileStream;

class cmGlobalGhsMultiGenerator : public cmGlobalGenerator
{
public:
  /// The default name of GHS MULTI's build file. Typically: monolith.gpj.
  static const char* FILE_EXTENSION;

  cmGlobalGhsMultiGenerator(cmake* cm);
  ~cmGlobalGhsMultiGenerator();

  static cmGlobalGeneratorFactory* NewFactory()
  {
    return new cmGlobalGeneratorSimpleFactory<cmGlobalGhsMultiGenerator>();
  }

  ///! create the correct local generator
  virtual cmLocalGenerator* CreateLocalGenerator(cmMakefile* mf);

  /// @return the name of this generator.
  static std::string GetActualName() { return "Green Hills MULTI"; }

  ///! Get the name for this generator
  virtual std::string GetName() const { return this->GetActualName(); }

  /// Overloaded methods. @see cmGlobalGenerator::GetDocumentation()
  static void GetDocumentation(cmDocumentationEntry& entry);

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
  /*
  * Determine what program to use for building the project.
  */
  bool FindMakeProgram(cmMakefile* mf) CM_OVERRIDE;

  cmGeneratedFileStream* GetBuildFileStream()
  {
    return this->TargetFolderBuildStreams[""];
  }

  static void OpenBuildFileStream(std::string const& filepath,
                                  cmGeneratedFileStream** filestream);
  static void OpenBuildFileStream(cmGeneratedFileStream* filestream);
  static void CloseBuildFileStream(cmGeneratedFileStream** filestream);
  /// Write the common disclaimer text at the top of each build file.
  static void WriteDisclaimer(std::ostream* os);
  std::vector<std::string> GetLibDirs() { return this->LibDirs; }

  static void AddFilesUpToPath(
    cmGeneratedFileStream* mainBuildFile,
    std::map<std::string, cmGeneratedFileStream*>* targetFolderBuildStreams,
    char const* homeOutputDirectory, std::string const& path,
    GhsMultiGpj::Types projType, std::string const& relPath = "");
  static void Open(std::string const& mapKeyName, std::string const& fileName,
                   std::map<std::string, cmGeneratedFileStream*>* fileMap);

  static std::string trimQuotes(std::string const& str);
  inline bool IsOSDirRelative() { return this->OSDirRelative; }

protected:
  virtual void Generate();
  virtual void GenerateBuildCommand(
    std::vector<std::string>& makeCommand, const std::string& makeProgram,
    const std::string& projectName, const std::string& projectDir,
    const std::string& targetName, const std::string& config, bool fast,
    bool verbose,
    std::vector<std::string> const& makeOptions = std::vector<std::string>());

private:
  std::string const& GetGhsBuildCommand();
  std::string FindGhsBuildCommand();
  std::string GetCompRoot();
  std::vector<std::string> GetCompRootHardPaths();
  std::vector<std::string> GetCompRootRegistry();
  void OpenBuildFileStream();

  void WriteMacros();
  void WriteHighLevelDirectives();
  void WriteCompilerOptions(std::string const& fOSDir);

  static void AddFilesUpToPathNewBuildFile(
    cmGeneratedFileStream* mainBuildFile,
    std::map<std::string, cmGeneratedFileStream*>* targetFolderBuildStreams,
    char const* homeOutputDirectory, std::string const& pathUpTo, bool isFirst,
    std::string const& relPath, GhsMultiGpj::Types projType);
  static void AddFilesUpToPathAppendNextFile(
    std::map<std::string, cmGeneratedFileStream*>* targetFolderBuildStreams,
    std::string const& pathUpTo,
    std::vector<cmsys::String>::const_iterator splitPathI,
    std::vector<cmsys::String>::const_iterator end,
    GhsMultiGpj::Types projType);
  static std::string GetFileNameFromPath(std::string const& path);
  void UpdateBuildFiles(std::vector<cmGeneratorTarget*> tgts);
  bool IsTgtForBuild(const cmGeneratorTarget* tgt);

  std::vector<cmGeneratedFileStream*> TargetSubProjects;
  std::map<std::string, cmGeneratedFileStream*> TargetFolderBuildStreams;

  std::vector<std::string> LibDirs;

  bool OSDirRelative;
  bool GhsBuildCommandInitialized;
  std::string GhsBuildCommand;
  static const char* DEFAULT_MAKE_PROGRAM;
};

#endif
