/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportBuildAndroidMKGenerator_h
#define cmExportBuildAndroidMKGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <iosfwd>
#include <string>
#include <vector>

#include "cmExportBuildFileGenerator.h"
#include "cmExportFileGenerator.h"
#include "cmStateTypes.h"

class cmGeneratorTarget;

/** \class cmExportBuildAndroidMKGenerator
 * \brief Generate a file exporting targets from a build tree.
 *
 * cmExportBuildAndroidMKGenerator generates a file exporting targets from
 * a build tree.  This exports the targets to the Android ndk build tool
 * makefile format for prebuilt libraries.
 *
 * This is used to implement the EXPORT() command.
 */
class cmExportBuildAndroidMKGenerator : public cmExportBuildFileGenerator
{
public:
  cmExportBuildAndroidMKGenerator();
  // this is so cmExportInstallAndroidMKGenerator can share this
  // function as they are almost the same
  enum GenerateType
  {
    BUILD,
    INSTALL
  };
  static void GenerateInterfaceProperties(cmGeneratorTarget const* target,
                                          std::ostream& os,
                                          const ImportPropertyMap& properties,
                                          GenerateType type,
                                          std::string const& config);

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
};

#endif
