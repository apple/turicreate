/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExtraEclipseCDT4Generator_h
#define cmExtraEclipseCDT4Generator_h

#include "cmConfigure.h"

#include "cmExternalMakefileProjectGenerator.h"

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

class cmLocalGenerator;
class cmMakefile;
class cmSourceGroup;
class cmXMLWriter;

/** \class cmExtraEclipseCDT4Generator
 * \brief Write Eclipse project files for Makefile based projects
 */
class cmExtraEclipseCDT4Generator : public cmExternalMakefileProjectGenerator
{
public:
  enum LinkType
  {
    VirtualFolder,
    LinkToFolder,
    LinkToFile
  };

  cmExtraEclipseCDT4Generator();

  static cmExternalMakefileProjectGeneratorFactory* GetFactory();

  void EnableLanguage(std::vector<std::string> const& languages, cmMakefile*,
                      bool optional) CM_OVERRIDE;

  void Generate() CM_OVERRIDE;

private:
  // create .project file in the source tree
  void CreateSourceProjectFile();

  // create .project file
  void CreateProjectFile();

  // create .cproject file
  void CreateCProjectFile() const;

  // If built with cygwin cmake, convert posix to windows path.
  static std::string GetEclipsePath(const std::string& path);

  // Extract basename.
  static std::string GetPathBasename(const std::string& path);

  // Generate the project name as: <name>-<type>@<path>
  static std::string GenerateProjectName(const std::string& name,
                                         const std::string& type,
                                         const std::string& path);

  // Helper functions
  static void AppendStorageScanners(cmXMLWriter& xml,
                                    const cmMakefile& makefile);
  static void AppendTarget(cmXMLWriter& xml, const std::string& target,
                           const std::string& make,
                           const std::string& makeArguments,
                           const std::string& path, const char* prefix = "",
                           const char* makeTarget = CM_NULLPTR);
  static void AppendScannerProfile(
    cmXMLWriter& xml, const std::string& profileID, bool openActionEnabled,
    const std::string& openActionFilePath, bool pParserEnabled,
    const std::string& scannerInfoProviderID,
    const std::string& runActionArguments, const std::string& runActionCommand,
    bool runActionUseDefault, bool sipParserEnabled);

  static void AppendLinkedResource(cmXMLWriter& xml, const std::string& name,
                                   const std::string& path, LinkType linkType);

  static void AppendIncludeDirectories(
    cmXMLWriter& xml, const std::vector<std::string>& includeDirs,
    std::set<std::string>& emittedDirs);

  static void AddEnvVar(std::ostream& out, const char* envVar,
                        cmLocalGenerator* lg);

  void WriteGroups(std::vector<cmSourceGroup> const& sourceGroups,
                   std::string& linkName, cmXMLWriter& xml);
  void CreateLinksToSubprojects(cmXMLWriter& xml, const std::string& baseDir);
  void CreateLinksForTargets(cmXMLWriter& xml);

  std::vector<std::string> SrcLinkedResources;
  std::set<std::string> Natures;
  std::string HomeDirectory;
  std::string HomeOutputDirectory;
  bool IsOutOfSourceBuild;
  bool GenerateSourceProject;
  bool GenerateLinkedResources;
  bool SupportsVirtualFolders;
  bool SupportsGmakeErrorParser;
  bool SupportsMachO64Parser;
  bool CEnabled;
  bool CXXEnabled;
};

#endif
