/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackRPMGenerator.h"

#include <algorithm>
#include <ctype.h>
#include <map>
#include <ostream>
#include <utility>
#include <vector>

#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"
#include "cmCPackLog.h"
#include "cmSystemTools.h"

cmCPackRPMGenerator::cmCPackRPMGenerator()
{
}

cmCPackRPMGenerator::~cmCPackRPMGenerator()
{
}

int cmCPackRPMGenerator::InitializeInternal()
{
  this->SetOptionIfNotSet("CPACK_PACKAGING_INSTALL_PREFIX", "/usr");
  if (cmSystemTools::IsOff(this->GetOption("CPACK_SET_DESTDIR"))) {
    this->SetOption("CPACK_SET_DESTDIR", "I_ON");
  }
  /* Replace space in CPACK_PACKAGE_NAME in order to avoid
   * rpmbuild scream on unwanted space in filename issue
   * Moreover RPM file do not usually embed space in filename
   */
  if (this->GetOption("CPACK_PACKAGE_NAME")) {
    std::string packageName = this->GetOption("CPACK_PACKAGE_NAME");
    std::replace(packageName.begin(), packageName.end(), ' ', '-');
    this->SetOption("CPACK_PACKAGE_NAME", packageName.c_str());
  }
  /* same for CPACK_PACKAGE_FILE_NAME */
  if (this->GetOption("CPACK_PACKAGE_FILE_NAME")) {
    std::string packageName = this->GetOption("CPACK_PACKAGE_FILE_NAME");
    std::replace(packageName.begin(), packageName.end(), ' ', '-');
    this->SetOption("CPACK_PACKAGE_FILE_NAME", packageName.c_str());
  }
  return this->Superclass::InitializeInternal();
}

void cmCPackRPMGenerator::AddGeneratedPackageNames()
{
  // add the generated packages to package file names list
  std::string fileNames(this->GetOption("GEN_CPACK_OUTPUT_FILES"));
  const char sep = ';';
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = fileNames.find(sep, pos1 + 1);
  while (pos2 != std::string::npos) {
    packageFileNames.push_back(fileNames.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = fileNames.find(sep, pos1 + 1);
  }
  packageFileNames.push_back(fileNames.substr(pos1, pos2 - pos1));
}

int cmCPackRPMGenerator::PackageOnePack(std::string const& initialToplevel,
                                        std::string const& packageName)
{
  int retval = 1;
  // Begin the archive for this pack
  std::string localToplevel(initialToplevel);
  std::string packageFileName(cmSystemTools::GetParentDirectory(toplevel));
  std::string outputFileName(
    GetComponentPackageFileName(this->GetOption("CPACK_PACKAGE_FILE_NAME"),
                                packageName, true) +
    this->GetOutputExtension());

  localToplevel += "/" + packageName;
  /* replace the TEMP DIRECTORY with the component one */
  this->SetOption("CPACK_TEMPORARY_DIRECTORY", localToplevel.c_str());
  packageFileName += "/" + outputFileName;
  /* replace proposed CPACK_OUTPUT_FILE_NAME */
  this->SetOption("CPACK_OUTPUT_FILE_NAME", outputFileName.c_str());
  /* replace the TEMPORARY package file name */
  this->SetOption("CPACK_TEMPORARY_PACKAGE_FILE_NAME",
                  packageFileName.c_str());
  // Tell CPackRPM.cmake the name of the component NAME.
  this->SetOption("CPACK_RPM_PACKAGE_COMPONENT", packageName.c_str());
  // Tell CPackRPM.cmake the path where the component is.
  std::string component_path = "/";
  component_path += packageName;
  this->SetOption("CPACK_RPM_PACKAGE_COMPONENT_PART_PATH",
                  component_path.c_str());
  if (!this->ReadListFile("CPackRPM.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error while execution CPackRPM.cmake"
                    << std::endl);
    retval = 0;
  }

  return retval;
}

int cmCPackRPMGenerator::PackageComponents(bool ignoreGroup)
{
  int retval = 1;
  /* Reset package file name list it will be populated during the
   * component packaging run*/
  packageFileNames.clear();
  std::string initialTopLevel(this->GetOption("CPACK_TEMPORARY_DIRECTORY"));

  const char* mainComponent = this->GetOption("CPACK_RPM_MAIN_COMPONENT");

  if (this->IsOn("CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE") &&
      !this->IsOn("CPACK_RPM_DEBUGINFO_PACKAGE")) {
    // check if we need to set CPACK_RPM_DEBUGINFO_PACKAGE because non of
    // the components is setting per component debuginfo package variable
    bool shouldSet = true;

    if (ignoreGroup) {
      std::map<std::string, cmCPackComponent>::iterator compIt;
      for (compIt = this->Components.begin(); compIt != this->Components.end();
           ++compIt) {
        std::string component(compIt->first);
        std::transform(component.begin(), component.end(), component.begin(),
                       ::toupper);

        if (this->IsOn("CPACK_RPM_" + compIt->first + "_DEBUGINFO_PACKAGE") ||
            this->IsOn("CPACK_RPM_" + component + "_DEBUGINFO_PACKAGE")) {
          shouldSet = false;
          break;
        }
      }
    } else {
      std::map<std::string, cmCPackComponentGroup>::iterator compGIt;
      for (compGIt = this->ComponentGroups.begin();
           compGIt != this->ComponentGroups.end(); ++compGIt) {
        std::string component(compGIt->first);
        std::transform(component.begin(), component.end(), component.begin(),
                       ::toupper);

        if (this->IsOn("CPACK_RPM_" + compGIt->first + "_DEBUGINFO_PACKAGE") ||
            this->IsOn("CPACK_RPM_" + component + "_DEBUGINFO_PACKAGE")) {
          shouldSet = false;
          break;
        }
      }

      if (shouldSet) {
        std::map<std::string, cmCPackComponent>::iterator compIt;
        for (compIt = this->Components.begin();
             compIt != this->Components.end(); ++compIt) {
          // Does the component belong to a group?
          if (compIt->second.Group == CM_NULLPTR) {
            std::string component(compIt->first);
            std::transform(component.begin(), component.end(),
                           component.begin(), ::toupper);

            if (this->IsOn("CPACK_RPM_" + compIt->first +
                           "_DEBUGINFO_PACKAGE") ||
                this->IsOn("CPACK_RPM_" + component + "_DEBUGINFO_PACKAGE")) {
              shouldSet = false;
              break;
            }
          }
        }
      }
    }

    if (shouldSet) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Setting "
                      << "CPACK_RPM_DEBUGINFO_PACKAGE because "
                      << "CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE is set but "
                      << " none of the "
                      << "CPACK_RPM_<component>_DEBUGINFO_PACKAGE variables "
                      << "are set." << std::endl);
      this->SetOption("CPACK_RPM_DEBUGINFO_PACKAGE", "ON");
    }
  }

  if (mainComponent) {
    if (this->IsOn("CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE")) {
      this->SetOption("GENERATE_SPEC_PARTS", "ON");
    }

    std::string mainComponentUpper(mainComponent);
    std::transform(mainComponentUpper.begin(), mainComponentUpper.end(),
                   mainComponentUpper.begin(), ::toupper);

    // The default behavior is to have one package by component group
    // unless CPACK_COMPONENTS_IGNORE_GROUP is specified.
    if (!ignoreGroup) {
      std::map<std::string, cmCPackComponentGroup>::iterator mainCompGIt =
        this->ComponentGroups.end();

      std::map<std::string, cmCPackComponentGroup>::iterator compGIt;
      for (compGIt = this->ComponentGroups.begin();
           compGIt != this->ComponentGroups.end(); ++compGIt) {
        std::string component(compGIt->first);
        std::transform(component.begin(), component.end(), component.begin(),
                       ::toupper);

        if (mainComponentUpper == component) {
          // main component will be handled last
          mainCompGIt = compGIt;
          continue;
        }

        cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Packaging component group: "
                        << compGIt->first << std::endl);
        retval &= PackageOnePack(initialTopLevel, compGIt->first);
      }
      // Handle Orphan components (components not belonging to any groups)
      std::map<std::string, cmCPackComponent>::iterator mainCompIt =
        this->Components.end();
      std::map<std::string, cmCPackComponent>::iterator compIt;
      for (compIt = this->Components.begin(); compIt != this->Components.end();
           ++compIt) {
        // Does the component belong to a group?
        if (compIt->second.Group == CM_NULLPTR) {
          std::string component(compIt->first);
          std::transform(component.begin(), component.end(), component.begin(),
                         ::toupper);

          if (mainComponentUpper == component) {
            // main component will be handled last
            mainCompIt = compIt;
            continue;
          }

          cmCPackLogger(
            cmCPackLog::LOG_VERBOSE, "Component <"
              << compIt->second.Name
              << "> does not belong to any group, package it separately."
              << std::endl);
          retval &= PackageOnePack(initialTopLevel, compIt->first);
        }
      }

      if (retval) {
        this->SetOption("GENERATE_SPEC_PARTS", "OFF");

        if (mainCompGIt != this->ComponentGroups.end()) {
          retval &= PackageOnePack(initialTopLevel, mainCompGIt->first);
        } else if (mainCompIt != this->Components.end()) {
          retval &= PackageOnePack(initialTopLevel, mainCompIt->first);
        } else {
          cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_RPM_MAIN_COMPONENT set"
                          << " to non existing component.\n");
          retval = 0;
        }
      }
    }
    // CPACK_COMPONENTS_IGNORE_GROUPS is set
    // We build 1 package per component
    else {
      std::map<std::string, cmCPackComponent>::iterator mainCompIt =
        this->Components.end();

      std::map<std::string, cmCPackComponent>::iterator compIt;
      for (compIt = this->Components.begin(); compIt != this->Components.end();
           ++compIt) {
        std::string component(compIt->first);
        std::transform(component.begin(), component.end(), component.begin(),
                       ::toupper);

        if (mainComponentUpper == component) {
          // main component will be handled last
          mainCompIt = compIt;
          continue;
        }

        retval &= PackageOnePack(initialTopLevel, compIt->first);
      }

      if (retval) {
        this->SetOption("GENERATE_SPEC_PARTS", "OFF");

        if (mainCompIt != this->Components.end()) {
          retval &= PackageOnePack(initialTopLevel, mainCompIt->first);
        } else {
          cmCPackLogger(cmCPackLog::LOG_ERROR, "CPACK_RPM_MAIN_COMPONENT set"
                          << " to non existing component.\n");
          retval = 0;
        }
      }
    }
  } else if (!this->IsOn("CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE") ||
             this->Components.size() == 1) {
    // The default behavior is to have one package by component group
    // unless CPACK_COMPONENTS_IGNORE_GROUP is specified.
    if (!ignoreGroup) {
      std::map<std::string, cmCPackComponentGroup>::iterator compGIt;
      for (compGIt = this->ComponentGroups.begin();
           compGIt != this->ComponentGroups.end(); ++compGIt) {
        cmCPackLogger(cmCPackLog::LOG_VERBOSE, "Packaging component group: "
                        << compGIt->first << std::endl);
        retval &= PackageOnePack(initialTopLevel, compGIt->first);
      }
      // Handle Orphan components (components not belonging to any groups)
      std::map<std::string, cmCPackComponent>::iterator compIt;
      for (compIt = this->Components.begin(); compIt != this->Components.end();
           ++compIt) {
        // Does the component belong to a group?
        if (compIt->second.Group == CM_NULLPTR) {
          cmCPackLogger(
            cmCPackLog::LOG_VERBOSE, "Component <"
              << compIt->second.Name
              << "> does not belong to any group, package it separately."
              << std::endl);
          retval &= PackageOnePack(initialTopLevel, compIt->first);
        }
      }
    }
    // CPACK_COMPONENTS_IGNORE_GROUPS is set
    // We build 1 package per component
    else {
      std::map<std::string, cmCPackComponent>::iterator compIt;
      for (compIt = this->Components.begin(); compIt != this->Components.end();
           ++compIt) {
        retval &= PackageOnePack(initialTopLevel, compIt->first);
      }
    }
  } else {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR, "CPACK_RPM_MAIN_COMPONENT not set but"
        << " it is mandatory with CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE"
        << " being set.\n");
    retval = 0;
  }

  if (retval) {
    AddGeneratedPackageNames();
  }

  return retval;
}

int cmCPackRPMGenerator::PackageComponentsAllInOne(
  const std::string& compInstDirName)
{
  int retval = 1;
  /* Reset package file name list it will be populated during the
   * component packaging run*/
  packageFileNames.clear();
  std::string initialTopLevel(this->GetOption("CPACK_TEMPORARY_DIRECTORY"));

  if (this->IsOn("CPACK_RPM_DEBUGINFO_SINGLE_PACKAGE")) {
    this->SetOption("CPACK_RPM_DEBUGINFO_PACKAGE", "ON");
  }

  cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                "Packaging all groups in one package..."
                "(CPACK_COMPONENTS_ALL_[GROUPS_]IN_ONE_PACKAGE is set)"
                  << std::endl);

  // The ALL GROUPS in ONE package case
  std::string localToplevel(initialTopLevel);
  std::string packageFileName(cmSystemTools::GetParentDirectory(toplevel));
  std::string outputFileName(
    std::string(this->GetOption("CPACK_PACKAGE_FILE_NAME")) +
    this->GetOutputExtension());
  // all GROUP in one vs all COMPONENT in one
  localToplevel += "/" + compInstDirName;

  /* replace the TEMP DIRECTORY with the component one */
  this->SetOption("CPACK_TEMPORARY_DIRECTORY", localToplevel.c_str());
  packageFileName += "/" + outputFileName;
  /* replace proposed CPACK_OUTPUT_FILE_NAME */
  this->SetOption("CPACK_OUTPUT_FILE_NAME", outputFileName.c_str());
  /* replace the TEMPORARY package file name */
  this->SetOption("CPACK_TEMPORARY_PACKAGE_FILE_NAME",
                  packageFileName.c_str());

  if (!compInstDirName.empty()) {
    // Tell CPackRPM.cmake the path where the component is.
    std::string component_path = "/";
    component_path += compInstDirName;
    this->SetOption("CPACK_RPM_PACKAGE_COMPONENT_PART_PATH",
                    component_path.c_str());
  }

  if (this->ReadListFile("CPackRPM.cmake")) {
    AddGeneratedPackageNames();
  } else {
    cmCPackLogger(cmCPackLog::LOG_ERROR, "Error while execution CPackRPM.cmake"
                    << std::endl);
    retval = 0;
  }

  return retval;
}

int cmCPackRPMGenerator::PackageFiles()
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Toplevel: " << toplevel << std::endl);

  /* Are we in the component packaging case */
  if (WantsComponentInstallation()) {
    // CASE 1 : COMPONENT ALL-IN-ONE package
    // If ALL COMPONENTS in ONE package has been requested
    // then the package file is unique and should be open here.
    if (componentPackageMethod == ONE_PACKAGE) {
      return PackageComponentsAllInOne("ALL_COMPONENTS_IN_ONE");
    }
    // CASE 2 : COMPONENT CLASSICAL package(s) (i.e. not all-in-one)
    // There will be 1 package for each component group
    // however one may require to ignore component group and
    // in this case you'll get 1 package for each component.
    return PackageComponents(componentPackageMethod ==
                             ONE_PACKAGE_PER_COMPONENT);
  }
  // CASE 3 : NON COMPONENT package.
  return PackageComponentsAllInOne("");
}

bool cmCPackRPMGenerator::SupportsComponentInstallation() const
{
  return IsOn("CPACK_RPM_COMPONENT_INSTALL");
}

std::string cmCPackRPMGenerator::GetComponentInstallDirNameSuffix(
  const std::string& componentName)
{
  if (componentPackageMethod == ONE_PACKAGE_PER_COMPONENT) {
    return componentName;
  }

  if (componentPackageMethod == ONE_PACKAGE) {
    return std::string("ALL_COMPONENTS_IN_ONE");
  }
  // We have to find the name of the COMPONENT GROUP
  // the current COMPONENT belongs to.
  std::string groupVar =
    "CPACK_COMPONENT_" + cmSystemTools::UpperCase(componentName) + "_GROUP";
  if (CM_NULLPTR != GetOption(groupVar)) {
    return std::string(GetOption(groupVar));
  }
  return componentName;
}
