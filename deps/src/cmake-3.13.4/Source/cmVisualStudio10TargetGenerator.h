/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVisualStudioTargetGenerator_h
#define cmVisualStudioTargetGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class cmComputeLinkInformation;
class cmCustomCommand;
class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmGlobalVisualStudio10Generator;
class cmLocalVisualStudio10Generator;
class cmMakefile;
class cmSourceFile;
class cmSourceGroup;
class cmVS10GeneratorOptions;

class cmVisualStudio10TargetGenerator
{
  CM_DISABLE_COPY(cmVisualStudio10TargetGenerator)

public:
  cmVisualStudio10TargetGenerator(cmGeneratorTarget* target,
                                  cmGlobalVisualStudio10Generator* gg);
  ~cmVisualStudio10TargetGenerator();
  void Generate();

private:
  struct ToolSource
  {
    cmSourceFile const* SourceFile;
    bool RelativePath;
  };
  struct ToolSources : public std::vector<ToolSource>
  {
  };

  struct TargetsFileAndConfigs
  {
    std::string File;
    std::vector<std::string> Configs;
  };

  struct Elem;
  struct OptionsHelper;

  std::string ConvertPath(std::string const& path, bool forceRelative);
  std::string CalcCondition(const std::string& config) const;
  void WriteProjectConfigurations(Elem& e0);
  void WriteProjectConfigurationValues(Elem& e0);
  void WriteMSToolConfigurationValues(Elem& e1, std::string const& config);
  void WriteMSToolConfigurationValuesManaged(Elem& e1,
                                             std::string const& config);
  void WriteHeaderSource(Elem& e1, cmSourceFile const* sf);
  void WriteExtraSource(Elem& e1, cmSourceFile const* sf);
  void WriteNsightTegraConfigurationValues(Elem& e1,
                                           std::string const& config);
  void WriteSource(Elem& e2, std::string const& tool, cmSourceFile const* sf);
  void WriteExcludeFromBuild(Elem& e2,
                             std::vector<size_t> const& exclude_configs);
  void WriteAllSources(Elem& e0);
  void WriteDotNetReferences(Elem& e0);
  void WriteDotNetReference(Elem& e1, std::string const& ref,
                            std::string const& hint,
                            std::string const& config);
  void WriteDotNetReferenceCustomTags(Elem& e2, std::string const& ref);
  void WriteEmbeddedResourceGroup(Elem& e0);
  void WriteWinRTReferences(Elem& e0);
  void WriteWinRTPackageCertificateKeyFile(Elem& e0);
  void WriteXamlFilesGroup(Elem& e0);
  void WritePathAndIncrementalLinkOptions(Elem& e0);
  void WriteItemDefinitionGroups(Elem& e0);
  void VerifyNecessaryFiles();
  void WriteMissingFiles(Elem& e1);
  void WriteMissingFilesWP80(Elem& e1);
  void WriteMissingFilesWP81(Elem& e1);
  void WriteMissingFilesWS80(Elem& e1);
  void WriteMissingFilesWS81(Elem& e1);
  void WriteMissingFilesWS10_0(Elem& e1);
  void WritePlatformExtensions(Elem& e1);
  void WriteSinglePlatformExtension(Elem& e1, std::string const& extension,
                                    std::string const& version);
  void WriteSDKReferences(Elem& e0);
  void WriteSingleSDKReference(Elem& e1, std::string const& extension,
                               std::string const& version);
  void WriteCommonMissingFiles(Elem& e1, const std::string& manifestFile);
  void WriteTargetSpecificReferences(Elem& e0);
  void WriteTargetsFileReferences(Elem& e1);

  std::vector<std::string> GetIncludes(std::string const& config,
                                       std::string const& lang) const;

  bool ComputeClOptions();
  bool ComputeClOptions(std::string const& configName);
  void WriteClOptions(Elem& e1, std::string const& config);
  bool ComputeRcOptions();
  bool ComputeRcOptions(std::string const& config);
  void WriteRCOptions(Elem& e1, std::string const& config);
  bool ComputeCudaOptions();
  bool ComputeCudaOptions(std::string const& config);
  void WriteCudaOptions(Elem& e1, std::string const& config);

  bool ComputeCudaLinkOptions();
  bool ComputeCudaLinkOptions(std::string const& config);
  void WriteCudaLinkOptions(Elem& e1, std::string const& config);

  bool ComputeMasmOptions();
  bool ComputeMasmOptions(std::string const& config);
  void WriteMasmOptions(Elem& e1, std::string const& config);
  bool ComputeNasmOptions();
  bool ComputeNasmOptions(std::string const& config);
  void WriteNasmOptions(Elem& e1, std::string const& config);

  bool ComputeLinkOptions();
  bool ComputeLinkOptions(std::string const& config);
  bool ComputeLibOptions();
  bool ComputeLibOptions(std::string const& config);
  void WriteLinkOptions(Elem& e1, std::string const& config);
  void WriteMidlOptions(Elem& e1, std::string const& config);
  void WriteAntBuildOptions(Elem& e1, std::string const& config);
  void OutputLinkIncremental(Elem& e1, std::string const& configName);
  void WriteCustomRule(Elem& e0, cmSourceFile const* source,
                       cmCustomCommand const& command);
  void WriteCustomRuleCpp(Elem& e2, std::string const& config,
                          std::string const& script, std::string const& inputs,
                          std::string const& outputs,
                          std::string const& comment);
  void WriteCustomRuleCSharp(Elem& e0, std::string const& config,
                             std::string const& commandName,
                             std::string const& script,
                             std::string const& inputs,
                             std::string const& outputs,
                             std::string const& comment);
  void WriteCustomCommands(Elem& e0);
  void WriteCustomCommand(Elem& e0, cmSourceFile const* sf);
  void WriteGroups();
  void WriteProjectReferences(Elem& e0);
  void WriteApplicationTypeSettings(Elem& e1);
  void OutputSourceSpecificFlags(Elem& e2, cmSourceFile const* source);
  void AddLibraries(const cmComputeLinkInformation& cli,
                    std::vector<std::string>& libVec,
                    std::vector<std::string>& vsTargetVec,
                    const std::string& config);
  void AddTargetsFileAndConfigPair(std::string const& targetsFile,
                                   std::string const& config);
  void WriteLibOptions(Elem& e1, std::string const& config);
  void WriteManifestOptions(Elem& e1, std::string const& config);
  void WriteEvents(Elem& e1, std::string const& configName);
  void WriteEvent(Elem& e1, const char* name,
                  std::vector<cmCustomCommand> const& commands,
                  std::string const& configName);
  void WriteGroupSources(Elem& e0, std::string const& name,
                         ToolSources const& sources,
                         std::vector<cmSourceGroup>&);
  void AddMissingSourceGroups(std::set<cmSourceGroup*>& groupsUsed,
                              const std::vector<cmSourceGroup>& allGroups);
  bool IsResxHeader(const std::string& headerFile);
  bool IsXamlHeader(const std::string& headerFile);
  bool IsXamlSource(const std::string& headerFile);

  bool ForceOld(const std::string& source) const;

  void GetCSharpSourceProperties(cmSourceFile const* sf,
                                 std::map<std::string, std::string>& tags);
  void WriteCSharpSourceProperties(
    Elem& e2, const std::map<std::string, std::string>& tags);
  void GetCSharpSourceLink(cmSourceFile const* sf, std::string& link);

private:
  friend class cmVS10GeneratorOptions;
  typedef cmVS10GeneratorOptions Options;
  typedef std::map<std::string, std::unique_ptr<Options>> OptionsMap;
  OptionsMap ClOptions;
  OptionsMap RcOptions;
  OptionsMap CudaOptions;
  OptionsMap CudaLinkOptions;
  OptionsMap MasmOptions;
  OptionsMap NasmOptions;
  OptionsMap LinkOptions;
  std::string LangForClCompile;
  enum VsProjectType
  {
    vcxproj,
    csproj
  } ProjectType;
  bool InSourceBuild;
  std::vector<std::string> Configurations;
  std::vector<TargetsFileAndConfigs> TargetsFileAndConfigsVec;
  cmGeneratorTarget* const GeneratorTarget;
  cmMakefile* const Makefile;
  std::string const Platform;
  std::string const Name;
  std::string const GUID;
  bool MSTools;
  bool Managed;
  bool NsightTegra;
  unsigned int NsightTegraVersion[4];
  bool TargetCompileAsWinRT;
  std::set<std::string> IPOEnabledConfigurations;
  std::set<std::string> SpectreMitigationConfigurations;
  cmGlobalVisualStudio10Generator* const GlobalGenerator;
  cmLocalVisualStudio10Generator* const LocalGenerator;
  std::set<std::string> CSharpCustomCommandNames;
  bool IsMissingFiles;
  std::vector<std::string> AddedFiles;
  std::string DefaultArtifactDir;
  bool AddedDefaultCertificate = false;
  // managed C++/C# relevant members
  typedef std::pair<std::string, std::string> DotNetHintReference;
  typedef std::vector<DotNetHintReference> DotNetHintReferenceList;
  typedef std::map<std::string, DotNetHintReferenceList>
    DotNetHintReferenceMap;
  DotNetHintReferenceMap DotNetHintReferences;
  typedef std::set<std::string> UsingDirectories;
  typedef std::map<std::string, UsingDirectories> UsingDirectoriesMap;
  UsingDirectoriesMap AdditionalUsingDirectories;

  typedef std::map<std::string, ToolSources> ToolSourceMap;
  ToolSourceMap Tools;
  std::string GetCMakeFilePath(const char* name) const;
};

#endif
