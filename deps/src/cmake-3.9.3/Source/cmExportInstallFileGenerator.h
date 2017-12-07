/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportInstallFileGenerator_h
#define cmExportInstallFileGenerator_h

#include "cmConfigure.h"

#include "cmExportFileGenerator.h"

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

class cmGeneratorTarget;
class cmGlobalGenerator;
class cmInstallExportGenerator;
class cmInstallTargetGenerator;

/** \class cmExportInstallFileGenerator
 * \brief Generate a file exporting targets from an install tree.
 *
 * cmExportInstallFileGenerator generates files exporting targets from
 * install an installation tree.  The files are placed in a temporary
 * location for installation by cmInstallExportGenerator.  One main
 * file is generated that creates the imported targets and loads
 * per-configuration files.  Target locations and settings for each
 * configuration are written to these per-configuration files.  After
 * installation the main file loads the configurations that have been
 * installed.
 *
 * This is used to implement the INSTALL(EXPORT) command.
 */
class cmExportInstallFileGenerator : public cmExportFileGenerator
{
public:
  /** Construct with the export installer that will install the
      files.  */
  cmExportInstallFileGenerator(cmInstallExportGenerator* iegen);

  /** Get the per-config file generated for each configuraiton.  This
      maps from the configuration name to the file temporary location
      for installation.  */
  std::map<std::string, std::string> const& GetConfigImportFiles()
  {
    return this->ConfigImportFiles;
  }

  /** Compute the globbing expression used to load per-config import
      files from the main file.  */
  std::string GetConfigImportFileGlob();

protected:
  // Implement virtual methods from the superclass.
  bool GenerateMainFile(std::ostream& os) CM_OVERRIDE;
  void GenerateImportTargetsConfig(
    std::ostream& os, const std::string& config, std::string const& suffix,
    std::vector<std::string>& missingTargets) CM_OVERRIDE;
  void HandleMissingTarget(std::string& link_libs,
                           std::vector<std::string>& missingTargets,
                           cmGeneratorTarget* depender,
                           cmGeneratorTarget* dependee) CM_OVERRIDE;

  void ReplaceInstallPrefix(std::string& input) CM_OVERRIDE;

  void ComplainAboutMissingTarget(cmGeneratorTarget* depender,
                                  cmGeneratorTarget* dependee,
                                  int occurrences);

  std::vector<std::string> FindNamespaces(cmGlobalGenerator* gg,
                                          const std::string& name);

  /** Generate the relative import prefix.  */
  virtual void GenerateImportPrefix(std::ostream&);

  /** Generate the relative import prefix.  */
  virtual void LoadConfigFiles(std::ostream&);

  virtual void CleanupTemporaryVariables(std::ostream&);

  /** Generate a per-configuration file for the targets.  */
  virtual bool GenerateImportFileConfig(
    const std::string& config, std::vector<std::string>& missingTargets);

  /** Fill in properties indicating installed file locations.  */
  void SetImportLocationProperty(const std::string& config,
                                 std::string const& suffix,
                                 cmInstallTargetGenerator* itgen,
                                 ImportPropertyMap& properties,
                                 std::set<std::string>& importedLocations);

  std::string InstallNameDir(cmGeneratorTarget* target,
                             const std::string& config) CM_OVERRIDE;

  cmInstallExportGenerator* IEGen;

  // The import file generated for each configuration.
  std::map<std::string, std::string> ConfigImportFiles;
};

#endif
