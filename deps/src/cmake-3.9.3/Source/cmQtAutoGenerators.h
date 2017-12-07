/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGenerators_h
#define cmQtAutoGenerators_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmFilePathChecksum.h"
#include "cmsys/RegularExpression.hxx"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class cmMakefile;

class cmQtAutoGenerators
{
public:
  cmQtAutoGenerators();
  bool Run(const std::string& targetDirectory, const std::string& config);

private:
  // -- Types

  /// @brief Used to extract additional dependencies from content text
  struct MocDependFilter
  {
    std::string key;
    cmsys::RegularExpression regExp;
  };
  typedef std::pair<std::string, cmsys::RegularExpression> MocMacroFilter;

  // -- Configuration
  bool MocDependFilterPush(const std::string& key, const std::string& regExp);
  bool ReadAutogenInfoFile(cmMakefile* makefile,
                           const std::string& targetDirectory,
                           const std::string& config);

  bool MocEnabled() const { return !this->MocExecutable.empty(); }
  bool UicEnabled() const { return !this->UicExecutable.empty(); }
  bool RccEnabled() const { return !this->RccExecutable.empty(); }

  // -- Settings file
  void SettingsFileRead(cmMakefile* makefile);
  bool SettingsFileWrite();

  bool AnySettingsChanged() const
  {
    return (this->MocSettingsChanged || this->RccSettingsChanged ||
            this->UicSettingsChanged);
  }

  // -- Init and run
  void Init(cmMakefile* makefile);
  bool RunAutogen();

  // -- Content analysis
  bool MocRequired(const std::string& contentText,
                   std::string* macroName = CM_NULLPTR);
  void MocFindDepends(
    const std::string& absFilename, const std::string& contentText,
    std::map<std::string, std::set<std::string> >& mocDepends);

  bool MocSkip(const std::string& absFilename) const;
  bool UicSkip(const std::string& absFilename) const;

  bool ParseSourceFile(
    const std::string& absFilename,
    std::map<std::string, std::string>& mocsIncluded,
    std::map<std::string, std::set<std::string> >& mocDepends,
    std::map<std::string, std::vector<std::string> >& includedUis,
    bool relaxed);

  void SearchHeadersForSourceFile(const std::string& absFilename,
                                  std::set<std::string>& mocHeaderFiles,
                                  std::set<std::string>& uicHeaderFiles) const;

  bool ParseHeaders(
    const std::set<std::string>& mocHeaderFiles,
    const std::set<std::string>& uicHeaderFiles,
    const std::map<std::string, std::string>& mocsIncluded,
    std::map<std::string, std::string>& mocsNotIncluded,
    std::map<std::string, std::set<std::string> >& mocDepends,
    std::map<std::string, std::vector<std::string> >& includedUis);

  void UicParseContent(
    const std::string& fileName, const std::string& contentText,
    std::map<std::string, std::vector<std::string> >& includedUis);

  bool MocParseSourceContent(
    const std::string& absFilename, const std::string& contentText,
    std::map<std::string, std::string>& mocsIncluded,
    std::map<std::string, std::set<std::string> >& mocDepends, bool relaxed);

  void MocParseHeaderContent(
    const std::string& absFilename, const std::string& contentText,
    std::map<std::string, std::string>& mocsNotIncluded,
    std::map<std::string, std::set<std::string> >& mocDepends);

  // -- Moc file generation
  bool MocGenerateAll(
    const std::map<std::string, std::string>& mocsIncluded,
    const std::map<std::string, std::string>& mocsNotIncluded,
    const std::map<std::string, std::set<std::string> >& mocDepends);
  bool MocGenerateFile(
    const std::string& sourceFile, const std::string& mocFileName,
    const std::map<std::string, std::set<std::string> >& mocDepends,
    bool included);

  // -- Uic file generation
  bool UicFindIncludedFile(std::string& absFile, const std::string& sourceFile,
                           const std::string& searchPath,
                           const std::string& searchFile);
  bool UicGenerateAll(
    const std::map<std::string, std::vector<std::string> >& includedUis);
  bool UicGenerateFile(const std::string& realName,
                       const std::string& uiInputFile,
                       const std::string& uiOutputFile);

  // -- Rcc file generation
  bool RccGenerateAll();
  bool RccGenerateFile(const std::string& qrcInputFile,
                       const std::string& qrcOutputFile, bool unique_n);

  // -- Logging
  void LogErrorNameCollision(
    const std::string& message,
    const std::multimap<std::string, std::string>& collisions) const;
  void LogBold(const std::string& message) const;
  void LogInfo(const std::string& message) const;
  void LogWarning(const std::string& message) const;
  void LogError(const std::string& message) const;
  void LogCommand(const std::vector<std::string>& command) const;

  // -- Utility
  bool NameCollisionTest(
    const std::map<std::string, std::string>& genFiles,
    std::multimap<std::string, std::string>& collisions) const;
  std::string ChecksumedPath(const std::string& sourceFile,
                             const std::string& basePrefix,
                             const std::string& baseSuffix) const;
  bool MakeParentDirectory(const char* logPrefix,
                           const std::string& filename) const;
  bool FileDiffers(const std::string& filename, const std::string& content);
  bool FileWrite(const char* logPrefix, const std::string& filename,
                 const std::string& content);

  bool RunCommand(const std::vector<std::string>& command, std::string& output,
                  bool verbose = true) const;

  bool FindHeader(std::string& header, const std::string& testBasePath) const;

  std::string MocFindHeader(const std::string& sourcePath,
                            const std::string& includeBase) const;
  bool MocFindIncludedFile(std::string& absFile, const std::string& sourceFile,
                           const std::string& includeString) const;

  // -- Meta
  std::string ConfigSuffix;
  // -- Directories
  std::string ProjectSourceDir;
  std::string ProjectBinaryDir;
  std::string CurrentSourceDir;
  std::string CurrentBinaryDir;
  std::string AutogenBuildDir;
  std::string AutogenIncludeDir;
  // -- Qt environment
  std::string QtMajorVersion;
  std::string MocExecutable;
  std::string UicExecutable;
  std::string RccExecutable;
  // -- File lists
  std::vector<std::string> Sources;
  std::vector<std::string> Headers;
  std::vector<std::string> HeaderExtensions;
  cmFilePathChecksum FPathChecksum;
  // -- Settings
  bool IncludeProjectDirsBefore;
  bool Verbose;
  bool ColorOutput;
  std::string SettingsFile;
  std::string SettingsStringMoc;
  std::string SettingsStringUic;
  std::string SettingsStringRcc;
  // -- Moc
  bool MocSettingsChanged;
  bool MocPredefsChanged;
  bool MocRelaxedMode;
  bool MocRunFailed;
  std::string MocCompFileRel;
  std::string MocCompFileAbs;
  std::string MocPredefsFileRel;
  std::string MocPredefsFileAbs;
  std::vector<std::string> MocSkipList;
  std::vector<std::string> MocIncludePaths;
  std::vector<std::string> MocIncludes;
  std::vector<std::string> MocDefinitions;
  std::vector<std::string> MocOptions;
  std::vector<std::string> MocPredefsCmd;
  std::vector<MocDependFilter> MocDependFilters;
  MocMacroFilter MocMacroFilters[2];
  cmsys::RegularExpression MocRegExpInclude;
  // -- Uic
  bool UicSettingsChanged;
  bool UicRunFailed;
  std::vector<std::string> UicSkipList;
  std::vector<std::string> UicTargetOptions;
  std::map<std::string, std::string> UicOptions;
  std::vector<std::string> UicSearchPaths;
  cmsys::RegularExpression UicRegExpInclude;
  // -- Rcc
  bool RccSettingsChanged;
  bool RccRunFailed;
  std::vector<std::string> RccSources;
  std::map<std::string, std::string> RccOptions;
  std::map<std::string, std::vector<std::string> > RccInputs;
};

#endif
