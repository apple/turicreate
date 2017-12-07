/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportBuildAndroidMKGenerator_h
#define cmExportBuildAndroidMKGenerator_h

#include "cmConfigure.h"

#include <iosfwd>
#include <string>
#include <vector>

#include "cmExportBuildFileGenerator.h"
#include "cmExportFileGenerator.h"

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
};

#endif
