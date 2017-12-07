/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportBuildFileGenerator.h"

#include "cmAlgorithms.h"
#include "cmExportSet.h"
#include "cmGeneratorExpression.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetExport.h"
#include "cmake.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <utility>

class cmSourceFile;

cmExportBuildFileGenerator::cmExportBuildFileGenerator()
{
  this->LG = CM_NULLPTR;
  this->ExportSet = CM_NULLPTR;
}

void cmExportBuildFileGenerator::Compute(cmLocalGenerator* lg)
{
  this->LG = lg;
  if (this->ExportSet) {
    this->ExportSet->Compute(lg);
  }
}

bool cmExportBuildFileGenerator::GenerateMainFile(std::ostream& os)
{
  {
    std::string expectedTargets;
    std::string sep;
    std::vector<std::string> targets;
    this->GetTargets(targets);
    for (std::vector<std::string>::const_iterator tei = targets.begin();
         tei != targets.end(); ++tei) {
      cmGeneratorTarget* te = this->LG->FindGeneratorTargetToUse(*tei);
      expectedTargets += sep + this->Namespace + te->GetExportName();
      sep = " ";
      if (this->ExportedTargets.insert(te).second) {
        this->Exports.push_back(te);
      } else {
        std::ostringstream e;
        e << "given target \"" << te->GetName() << "\" more than once.";
        this->LG->GetGlobalGenerator()->GetCMakeInstance()->IssueMessage(
          cmake::FATAL_ERROR, e.str(),
          this->LG->GetMakefile()->GetBacktrace());
        return false;
      }
      if (te->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
        this->GenerateRequiredCMakeVersion(os, "3.0.0");
      }
    }

    this->GenerateExpectedTargetsCode(os, expectedTargets);
  }

  std::vector<std::string> missingTargets;

  // Create all the imported targets.
  for (std::vector<cmGeneratorTarget*>::const_iterator tei =
         this->Exports.begin();
       tei != this->Exports.end(); ++tei) {
    cmGeneratorTarget* gte = *tei;
    this->GenerateImportTargetCode(os, gte);

    gte->Target->AppendBuildInterfaceIncludes();

    ImportPropertyMap properties;

    this->PopulateInterfaceProperty("INTERFACE_INCLUDE_DIRECTORIES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_SOURCES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_DEFINITIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_OPTIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_AUTOUIC_OPTIONS", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_COMPILE_FEATURES", gte,
                                    cmGeneratorExpression::BuildInterface,
                                    properties, missingTargets);
    this->PopulateInterfaceProperty("INTERFACE_POSITION_INDEPENDENT_CODE", gte,
                                    properties);
    const bool newCMP0022Behavior =
      gte->GetPolicyStatusCMP0022() != cmPolicies::WARN &&
      gte->GetPolicyStatusCMP0022() != cmPolicies::OLD;
    if (newCMP0022Behavior) {
      this->PopulateInterfaceLinkLibrariesProperty(
        gte, cmGeneratorExpression::BuildInterface, properties,
        missingTargets);
    }
    this->PopulateCompatibleInterfaceProperties(gte, properties);

    this->GenerateInterfaceProperties(gte, os, properties);
  }

  // Generate import file content for each configuration.
  for (std::vector<std::string>::const_iterator ci =
         this->Configurations.begin();
       ci != this->Configurations.end(); ++ci) {
    this->GenerateImportConfig(os, *ci, missingTargets);
  }

  this->GenerateMissingTargetsCheckCode(os, missingTargets);

  return true;
}

void cmExportBuildFileGenerator::GenerateImportTargetsConfig(
  std::ostream& os, const std::string& config, std::string const& suffix,
  std::vector<std::string>& missingTargets)
{
  for (std::vector<cmGeneratorTarget*>::const_iterator tei =
         this->Exports.begin();
       tei != this->Exports.end(); ++tei) {
    // Collect import properties for this target.
    cmGeneratorTarget* target = *tei;
    ImportPropertyMap properties;

    if (target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
      this->SetImportLocationProperty(config, suffix, target, properties);
    }
    if (!properties.empty()) {
      // Get the rest of the target details.
      if (target->GetType() != cmStateEnums::INTERFACE_LIBRARY) {
        this->SetImportDetailProperties(config, suffix, target, properties,
                                        missingTargets);
        this->SetImportLinkInterface(config, suffix,
                                     cmGeneratorExpression::BuildInterface,
                                     target, properties, missingTargets);
      }

      // TOOD: PUBLIC_HEADER_LOCATION
      // This should wait until the build feature propagation stuff
      // is done.  Then this can be a propagated include directory.
      // this->GenerateImportProperty(config, te->HeaderGenerator,
      //                              properties);

      // Generate code in the export file.
      this->GenerateImportPropertyCode(os, config, target, properties);
    }
  }
}

void cmExportBuildFileGenerator::SetExportSet(cmExportSet* exportSet)
{
  this->ExportSet = exportSet;
}

void cmExportBuildFileGenerator::SetImportLocationProperty(
  const std::string& config, std::string const& suffix,
  cmGeneratorTarget* target, ImportPropertyMap& properties)
{
  // Get the makefile in which to lookup target information.
  cmMakefile* mf = target->Makefile;

  if (target->GetType() == cmStateEnums::OBJECT_LIBRARY) {
    std::string prop = "IMPORTED_OBJECTS";
    prop += suffix;

    // Compute all the object files inside this target and setup
    // IMPORTED_OBJECTS as a list of object files
    std::vector<cmSourceFile const*> objectSources;
    target->GetObjectSources(objectSources, config);
    std::string const obj_dir = target->GetObjectDirectory(config);
    std::vector<std::string> objects;
    for (std::vector<cmSourceFile const*>::const_iterator si =
           objectSources.begin();
         si != objectSources.end(); ++si) {
      const std::string& obj = target->GetObjectName(*si);
      objects.push_back(obj_dir + obj);
    }

    // Store the property.
    properties[prop] = cmJoin(objects, ";");
  } else {
    // Add the main target file.
    {
      std::string prop = "IMPORTED_LOCATION";
      prop += suffix;
      std::string value;
      if (target->IsAppBundleOnApple()) {
        value =
          target->GetFullPath(config, cmStateEnums::RuntimeBinaryArtifact);
      } else {
        value = target->GetFullPath(config,
                                    cmStateEnums::RuntimeBinaryArtifact, true);
      }
      properties[prop] = value;
    }

    // Add the import library for windows DLLs.
    if (target->HasImportLibrary() &&
        mf->GetDefinition("CMAKE_IMPORT_LIBRARY_SUFFIX")) {
      std::string prop = "IMPORTED_IMPLIB";
      prop += suffix;
      std::string value =
        target->GetFullPath(config, cmStateEnums::ImportLibraryArtifact);
      target->GetImplibGNUtoMS(value, value, "${CMAKE_IMPORT_LIBRARY_SUFFIX}");
      properties[prop] = value;
    }
  }
}

void cmExportBuildFileGenerator::HandleMissingTarget(
  std::string& link_libs, std::vector<std::string>& missingTargets,
  cmGeneratorTarget* depender, cmGeneratorTarget* dependee)
{
  // The target is not in the export.
  if (!this->AppendMode) {
    const std::string name = dependee->GetName();
    cmGlobalGenerator* gg =
      dependee->GetLocalGenerator()->GetGlobalGenerator();
    std::vector<std::string> namespaces = this->FindNamespaces(gg, name);

    int targetOccurrences = (int)namespaces.size();
    if (targetOccurrences == 1) {
      std::string missingTarget = namespaces[0];

      missingTarget += dependee->GetExportName();
      link_libs += missingTarget;
      missingTargets.push_back(missingTarget);
      return;
    }
    // We are not appending, so all exported targets should be
    // known here.  This is probably user-error.
    this->ComplainAboutMissingTarget(depender, dependee, targetOccurrences);
  }
  // Assume the target will be exported by another command.
  // Append it with the export namespace.
  link_libs += this->Namespace;
  link_libs += dependee->GetExportName();
}

void cmExportBuildFileGenerator::GetTargets(
  std::vector<std::string>& targets) const
{
  if (this->ExportSet) {
    for (std::vector<cmTargetExport*>::const_iterator tei =
           this->ExportSet->GetTargetExports()->begin();
         tei != this->ExportSet->GetTargetExports()->end(); ++tei) {
      targets.push_back((*tei)->TargetName);
    }
    return;
  }
  targets = this->Targets;
}

std::vector<std::string> cmExportBuildFileGenerator::FindNamespaces(
  cmGlobalGenerator* gg, const std::string& name)
{
  std::vector<std::string> namespaces;

  std::map<std::string, cmExportBuildFileGenerator*>& exportSets =
    gg->GetBuildExportSets();

  for (std::map<std::string, cmExportBuildFileGenerator*>::const_iterator
         expIt = exportSets.begin();
       expIt != exportSets.end(); ++expIt) {
    const cmExportBuildFileGenerator* exportSet = expIt->second;
    std::vector<std::string> targets;
    exportSet->GetTargets(targets);
    if (std::find(targets.begin(), targets.end(), name) != targets.end()) {
      namespaces.push_back(exportSet->GetNamespace());
    }
  }

  return namespaces;
}

void cmExportBuildFileGenerator::ComplainAboutMissingTarget(
  cmGeneratorTarget* depender, cmGeneratorTarget* dependee, int occurrences)
{
  if (cmSystemTools::GetErrorOccuredFlag()) {
    return;
  }

  std::ostringstream e;
  e << "export called with target \"" << depender->GetName()
    << "\" which requires target \"" << dependee->GetName() << "\" ";
  if (occurrences == 0) {
    e << "that is not in the export set.\n";
  } else {
    e << "that is not in this export set, but " << occurrences
      << " times in others.\n";
  }
  e << "If the required target is not easy to reference in this call, "
    << "consider using the APPEND option with multiple separate calls.";

  this->LG->GetGlobalGenerator()->GetCMakeInstance()->IssueMessage(
    cmake::FATAL_ERROR, e.str(), this->LG->GetMakefile()->GetBacktrace());
}

std::string cmExportBuildFileGenerator::InstallNameDir(
  cmGeneratorTarget* target, const std::string& config)
{
  std::string install_name_dir;

  cmMakefile* mf = target->Target->GetMakefile();
  if (mf->IsOn("CMAKE_PLATFORM_HAS_INSTALLNAME")) {
    install_name_dir = target->GetInstallNameDirForBuildTree(config);
  }

  return install_name_dir;
}
