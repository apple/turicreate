/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackNuGetGenerator.h"

#include "cmAlgorithms.h"
#include "cmCPackComponentGroup.h"
#include "cmCPackLog.h"
#include "cmSystemTools.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

bool cmCPackNuGetGenerator::SupportsComponentInstallation() const
{
  return IsOn("CPACK_NUGET_COMPONENT_INSTALL");
}

int cmCPackNuGetGenerator::PackageFiles()
{
  cmCPackLogger(cmCPackLog::LOG_DEBUG, "Toplevel: " << toplevel << std::endl);

  /* Reset package file name list it will be populated after the
   * `CPackNuGet.cmake` run */
  packageFileNames.clear();

  /* Are we in the component packaging case */
  if (WantsComponentInstallation()) {
    if (componentPackageMethod == ONE_PACKAGE) {
      // CASE 1 : COMPONENT ALL-IN-ONE package
      // Meaning that all per-component pre-installed files
      // goes into the single package.
      this->SetOption("CPACK_NUGET_ALL_IN_ONE", "TRUE");
      SetupGroupComponentVariables(true);
    } else {
      // CASE 2 : COMPONENT CLASSICAL package(s) (i.e. not all-in-one)
      // There will be 1 package for each component group
      // however one may require to ignore component group and
      // in this case you'll get 1 package for each component.
      SetupGroupComponentVariables(componentPackageMethod ==
                                   ONE_PACKAGE_PER_COMPONENT);
    }
  } else {
    // CASE 3 : NON COMPONENT package.
    this->SetOption("CPACK_NUGET_ORDINAL_MONOLITIC", "TRUE");
  }

  auto retval = this->ReadListFile("Internal/CPack/CPackNuGet.cmake");
  if (retval) {
    AddGeneratedPackageNames();
  } else {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error while execution CPackNuGet.cmake" << std::endl);
  }

  return retval;
}

void cmCPackNuGetGenerator::SetupGroupComponentVariables(bool ignoreGroup)
{
  // The default behavior is to have one package by component group
  // unless CPACK_COMPONENTS_IGNORE_GROUP is specified.
  if (!ignoreGroup) {
    std::vector<std::string> groups;
    for (auto const& compG : this->ComponentGroups) {
      cmCPackLogger(cmCPackLog::LOG_VERBOSE,
                    "Packaging component group: " << compG.first << std::endl);
      groups.push_back(compG.first);
      auto compGUp =
        cmSystemTools::UpperCase(cmSystemTools::MakeCidentifier(compG.first));

      // Collect components for this group
      std::vector<std::string> components;
      std::transform(begin(compG.second.Components),
                     end(compG.second.Components),
                     std::back_inserter(components),
                     [](cmCPackComponent const* comp) { return comp->Name; });
      this->SetOption("CPACK_NUGET_" + compGUp + "_GROUP_COMPONENTS",
                      cmJoin(components, ";").c_str());
    }
    if (!groups.empty()) {
      this->SetOption("CPACK_NUGET_GROUPS", cmJoin(groups, ";").c_str());
    }

    // Handle Orphan components (components not belonging to any groups)
    std::vector<std::string> components;
    for (auto const& comp : this->Components) {
      // Does the component belong to a group?
      if (comp.second.Group == nullptr) {
        cmCPackLogger(
          cmCPackLog::LOG_VERBOSE,
          "Component <"
            << comp.second.Name
            << "> does not belong to any group, package it separately."
            << std::endl);
        components.push_back(comp.first);
      }
    }
    if (!components.empty()) {
      this->SetOption("CPACK_NUGET_COMPONENTS",
                      cmJoin(components, ";").c_str());
    }

  } else {
    std::vector<std::string> components;
    components.reserve(this->Components.size());
    std::transform(begin(this->Components), end(this->Components),
                   std::back_inserter(components),
                   [](std::pair<std::string, cmCPackComponent> const& comp) {
                     return comp.first;
                   });
    this->SetOption("CPACK_NUGET_COMPONENTS", cmJoin(components, ";").c_str());
  }
}

void cmCPackNuGetGenerator::AddGeneratedPackageNames()
{
  const char* const files_list = this->GetOption("GEN_CPACK_OUTPUT_FILES");
  if (!files_list) {
    cmCPackLogger(
      cmCPackLog::LOG_ERROR,
      "Error while execution CPackNuGet.cmake: No NuGet package has generated"
        << std::endl);
    return;
  }
  // add the generated packages to package file names list
  std::string fileNames{ files_list };
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
