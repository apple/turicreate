/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalCodeLiteGenerator_h
#define cmGlobalCodeLiteGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmExternalMakefileProjectGenerator.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class cmLocalGenerator;
class cmMakefile;
class cmGeneratorTarget;
class cmXMLWriter;
class cmSourceFile;

class cmExtraCodeLiteGenerator : public cmExternalMakefileProjectGenerator
{
protected:
  std::string ConfigName;
  std::string WorkspacePath;
  unsigned int CpuCount;

protected:
  std::string GetCodeLiteCompilerName(const cmMakefile* mf) const;
  std::string GetConfigurationName(const cmMakefile* mf) const;
  std::string GetBuildCommand(const cmMakefile* mf,
                              const std::string& targetName) const;
  std::string GetCleanCommand(const cmMakefile* mf,
                              const std::string& targetName) const;
  std::string GetRebuildCommand(const cmMakefile* mf,
                                const std::string& targetName) const;
  std::string GetSingleFileBuildCommand(const cmMakefile* mf) const;
  std::vector<std::string> CreateProjectsByTarget(cmXMLWriter* xml);
  std::vector<std::string> CreateProjectsByProjectMaps(cmXMLWriter* xml);
  std::string CollectSourceFiles(const cmMakefile* makefile,
                                 const cmGeneratorTarget* gt,
                                 std::map<std::string, cmSourceFile*>& cFiles,
                                 std::set<std::string>& otherFiles);
  void FindMatchingHeaderfiles(std::map<std::string, cmSourceFile*>& cFiles,
                               std::set<std::string>& otherFiles);
  void CreateProjectSourceEntries(std::map<std::string, cmSourceFile*>& cFiles,
                                  std::set<std::string>& otherFiles,
                                  cmXMLWriter* xml,
                                  const std::string& projectPath,
                                  const cmMakefile* mf,
                                  const std::string& projectType,
                                  const std::string& targetName);
  void CreateFoldersAndFiles(std::set<std::string>& cFiles, cmXMLWriter& xml,
                             const std::string& projectPath);
  void CreateFoldersAndFiles(std::map<std::string, cmSourceFile*>& cFiles,
                             cmXMLWriter& xml, const std::string& projectPath);

public:
  cmExtraCodeLiteGenerator();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  void Generate() override;
  void CreateProjectFile(const std::vector<cmLocalGenerator*>& lgs);

  void CreateNewProjectFile(const std::vector<cmLocalGenerator*>& lgs,
                            const std::string& filename);
  void CreateNewProjectFile(const cmGeneratorTarget* lg,
                            const std::string& filename);
};

#endif
