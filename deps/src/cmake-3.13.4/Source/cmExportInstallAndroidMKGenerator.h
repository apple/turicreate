/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportInstallAndroidMKGenerator_h
#define cmExportInstallAndroidMKGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

#include "cmExportFileGenerator.h"
#include "cmExportInstallFileGenerator.h"
#include "cmStateTypes.h"

class cmGeneratorTarget;
class cmInstallExportGenerator;

/** \class cmExportInstallAndroidMKGenerator
 * \brief Generate a file exporting targets from an install tree.
 *
 * cmExportInstallAndroidMKGenerator generates files exporting targets from
 * install an installation tree.  The files are placed in a temporary
 * location for installation by cmInstallExportGenerator.  The file format
 * is for the ndk build system and is a makefile fragment specifying prebuilt
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
  void GeneratePolicyHeaderCode(std::ostream&) override {}
  void GeneratePolicyFooterCode(std::ostream&) override {}
  void GenerateImportHeaderCode(std::ostream& os,
                                const std::string& config = "") override;
  void GenerateImportFooterCode(std::ostream& os) override;
  void GenerateImportTargetCode(
    std::ostream& os, cmGeneratorTarget const* target,
    cmStateEnums::TargetType /*targetType*/) override;
  void GenerateExpectedTargetsCode(
    std::ostream& os, const std::string& expectedTargets) override;
  void GenerateImportPropertyCode(
    std::ostream& os, const std::string& config,
    cmGeneratorTarget const* target,
    ImportPropertyMap const& properties) override;
  void GenerateMissingTargetsCheckCode(
    std::ostream& os, const std::vector<std::string>& missingTargets) override;
  void GenerateInterfaceProperties(
    cmGeneratorTarget const* target, std::ostream& os,
    const ImportPropertyMap& properties) override;
  void GenerateImportPrefix(std::ostream& os) override;
  void LoadConfigFiles(std::ostream&) override;
  void GenerateRequiredCMakeVersion(std::ostream& os,
                                    const char* versionString) override;
  void CleanupTemporaryVariables(std::ostream&) override;
  void GenerateImportedFileCheckLoop(std::ostream& os) override;
  void GenerateImportedFileChecksCode(
    std::ostream& os, cmGeneratorTarget* target,
    ImportPropertyMap const& properties,
    const std::set<std::string>& importedLocations) override;
  bool GenerateImportFileConfig(const std::string& config,
                                std::vector<std::string>&) override;
};

#endif
