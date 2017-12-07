/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportInstallAndroidMKGenerator_h
#define cmExportInstallAndroidMKGenerator_h

#include "cmConfigure.h"

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "cmExportFileGenerator.h"
#include "cmExportInstallFileGenerator.h"

class cmGeneratorTarget;
class cmInstallExportGenerator;

/** \class cmExportInstallAndroidMKGenerator
 * \brief Generate a file exporting targets from an install tree.
 *
 * cmExportInstallAndroidMKGenerator generates files exporting targets from
 * install an installation tree.  The files are placed in a temporary
 * location for installation by cmInstallExportGenerator.  The file format
 * is for the ndk build system and is a makefile fragment specifing prebuilt
 * libraries to the ndk build system.
 *
 * This is used to implement the INSTALL(EXPORT_ANDROID_MK) command.
 */
class cmExportInstallAndroidMKGenerator : public cmExportInstallFileGenerator
{
public:
  /** Construct with the export installer that will install the
      files.  */
  cmExportInstallAndroidMKGenerator(cmInstallExportGenerator* iegen);

protected:
  // Implement virtual methods from the superclass.
  void GeneratePolicyHeaderCode(std::ostream&) CM_OVERRIDE {}
  void GeneratePolicyFooterCode(std::ostream&) CM_OVERRIDE {}
  void GenerateImportHeaderCode(std::ostream& os,
                                const std::string& config = "") CM_OVERRIDE;
  void GenerateImportFooterCode(std::ostream& os) CM_OVERRIDE;
  void GenerateImportTargetCode(std::ostream& os,
                                const cmGeneratorTarget* target) CM_OVERRIDE;
  void GenerateExpectedTargetsCode(
    std::ostream& os, const std::string& expectedTargets) CM_OVERRIDE;
  void GenerateImportPropertyCode(std::ostream& os, const std::string& config,
                                  cmGeneratorTarget const* target,
                                  ImportPropertyMap const& properties)
    CM_OVERRIDE;
  void GenerateMissingTargetsCheckCode(
    std::ostream& os,
    const std::vector<std::string>& missingTargets) CM_OVERRIDE;
  void GenerateInterfaceProperties(
    cmGeneratorTarget const* target, std::ostream& os,
    const ImportPropertyMap& properties) CM_OVERRIDE;
  void GenerateImportPrefix(std::ostream& os) CM_OVERRIDE;
  void LoadConfigFiles(std::ostream&) CM_OVERRIDE;
  void GenerateRequiredCMakeVersion(std::ostream& os,
                                    const char* versionString) CM_OVERRIDE;
  void CleanupTemporaryVariables(std::ostream&) CM_OVERRIDE;
  void GenerateImportedFileCheckLoop(std::ostream& os) CM_OVERRIDE;
  void GenerateImportedFileChecksCode(
    std::ostream& os, cmGeneratorTarget* target,
    ImportPropertyMap const& properties,
    const std::set<std::string>& importedLocations) CM_OVERRIDE;
  bool GenerateImportFileConfig(const std::string& config,
                                std::vector<std::string>&) CM_OVERRIDE;
};

#endif
