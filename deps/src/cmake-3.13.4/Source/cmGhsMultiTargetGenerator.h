/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGhsMultiTargetGenerator_h
#define cmGhsMultiTargetGenerator_h

#include "cmGhsMultiGpj.h"

#include "cmTarget.h"

class cmCustomCommand;
class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmGlobalGhsMultiGenerator;
class cmLocalGhsMultiGenerator;
class cmMakefile;
class cmSourceFile;

class cmGhsMultiTargetGenerator
{
public:
  cmGhsMultiTargetGenerator(cmGeneratorTarget* target);

  virtual ~cmGhsMultiTargetGenerator();

  virtual void Generate();

  bool IncludeThisTarget();
  std::vector<cmSourceFile*> GetSources() const;
  GhsMultiGpj::Types GetGpjTag() const;
  static GhsMultiGpj::Types GetGpjTag(const cmGeneratorTarget* target);
  const char* GetAbsBuildFilePath() const
  {
    return this->AbsBuildFilePath.c_str();
  }
  const char* GetRelBuildFileName() const
  {
    return this->RelBuildFileName.c_str();
  }
  const char* GetAbsBuildFileName() const
  {
    return this->AbsBuildFileName.c_str();
  }
  const char* GetAbsOutputFileName() const
  {
    return this->AbsOutputFileName.c_str();
  }

  static std::string GetRelBuildFilePath(const cmGeneratorTarget* target);
  static std::string GetAbsPathToRoot(const cmGeneratorTarget* target);
  static std::string GetAbsBuildFilePath(const cmGeneratorTarget* target);
  static std::string GetRelBuildFileName(const cmGeneratorTarget* target);
  static std::string GetBuildFileName(const cmGeneratorTarget* target);
  static std::string AddSlashIfNeededToPath(std::string const& input);

private:
  cmGlobalGhsMultiGenerator* GetGlobalGenerator() const;
  cmGeneratedFileStream* GetFolderBuildStreams()
  {
    return this->FolderBuildStreams[""];
  };
  bool IsTargetGroup() const { return this->TargetGroup; }

  void WriteTypeSpecifics(const std::string& config, bool notKernel);
  void WriteCompilerFlags(const std::string& config,
                          const std::string& language);
  void WriteCompilerDefinitions(const std::string& config,
                                const std::string& language);

  void SetCompilerFlags(std::string const& config, const std::string& language,
                        bool const notKernel);
  std::string GetDefines(const std::string& langugae,
                         std::string const& config);

  void WriteIncludes(const std::string& config, const std::string& language);
  void WriteTargetLinkLibraries(std::string const& config,
                                std::string const& language);
  void WriteCustomCommands();
  void WriteCustomCommandsHelper(
    std::vector<cmCustomCommand> const& commandsSet,
    cmTarget::CustomCommandType commandType);
  void WriteSources(
    std::vector<cmSourceFile*> const& objectSources,
    std::map<const cmSourceFile*, std::string> const& objectNames);
  static std::map<const cmSourceFile*, std::string> GetObjectNames(
    std::vector<cmSourceFile*>* objectSources,
    cmLocalGhsMultiGenerator* localGhsMultiGenerator,
    cmGeneratorTarget* generatorTarget);
  static void WriteObjectLangOverride(cmGeneratedFileStream* fileStream,
                                      cmSourceFile* sourceFile);
  static void WriteObjectDir(cmGeneratedFileStream* fileStream,
                             std::string const& dir);
  std::string GetOutputDirectory(const std::string& config) const;
  std::string GetOutputFilename(const std::string& config) const;
  static std::string ComputeLongestObjectDirectory(
    cmLocalGhsMultiGenerator const* localGhsMultiGenerator,
    cmGeneratorTarget* generatorTarget, cmSourceFile* const sourceFile);

  bool IsNotKernel(std::string const& config, const std::string& language);
  static bool DetermineIfTargetGroup(const cmGeneratorTarget* target);
  bool DetermineIfDynamicDownload(std::string const& config,
                                  const std::string& language);

  cmGeneratorTarget* GeneratorTarget;
  cmLocalGhsMultiGenerator* LocalGenerator;
  cmMakefile* Makefile;
  std::string AbsBuildFilePath;
  std::string RelBuildFilePath;
  std::string AbsBuildFileName;
  std::string RelBuildFileName;
  std::string RelOutputFileName;
  std::string AbsOutputFileName;
  std::map<std::string, cmGeneratedFileStream*> FolderBuildStreams;
  bool TargetGroup;
  bool DynamicDownload;
  static std::string const DDOption;
  std::map<std::string, std::string> FlagsByLanguage;
  std::map<std::string, std::string> DefinesByLanguage;
};

#endif // ! cmGhsMultiTargetGenerator_h
