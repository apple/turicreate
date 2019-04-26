/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackIFWPackage.h"

#include "cmCPackComponentGroup.h"
#include "cmCPackIFWCommon.h"
#include "cmCPackIFWGenerator.h"
#include "cmCPackIFWInstaller.h"
#include "cmCPackLog.h" // IWYU pragma: keep
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmTimestamp.h"
#include "cmXMLWriter.h"

#include <map>
#include <sstream>
#include <stddef.h>
#include <utility>

//---------------------------------------------------------- CompareStruct ---
cmCPackIFWPackage::CompareStruct::CompareStruct()
  : Type(cmCPackIFWPackage::CompareNone)
{
}

//------------------------------------------------------- DependenceStruct ---
cmCPackIFWPackage::DependenceStruct::DependenceStruct()
{
}

cmCPackIFWPackage::DependenceStruct::DependenceStruct(
  const std::string& dependence)
{
  // Search compare section
  size_t pos = std::string::npos;
  if ((pos = dependence.find("<=")) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareLessOrEqual;
    this->Compare.Value = dependence.substr(pos + 2);
  } else if ((pos = dependence.find(">=")) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareGreaterOrEqual;
    this->Compare.Value = dependence.substr(pos + 2);
  } else if ((pos = dependence.find('<')) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareLess;
    this->Compare.Value = dependence.substr(pos + 1);
  } else if ((pos = dependence.find('=')) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareEqual;
    this->Compare.Value = dependence.substr(pos + 1);
  } else if ((pos = dependence.find('>')) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareGreater;
    this->Compare.Value = dependence.substr(pos + 1);
  } else if ((pos = dependence.find('-')) != std::string::npos) {
    this->Compare.Type = cmCPackIFWPackage::CompareNone;
    this->Compare.Value = dependence.substr(pos + 1);
  }
  size_t dashPos = dependence.find('-');
  if (dashPos != std::string::npos) {
    pos = dashPos;
  }
  this->Name =
    pos == std::string::npos ? dependence : dependence.substr(0, pos);
}

std::string cmCPackIFWPackage::DependenceStruct::NameWithCompare() const
{
  if (this->Compare.Type == cmCPackIFWPackage::CompareNone) {
    return this->Name;
  }

  std::string result = this->Name;

  if (this->Compare.Type != cmCPackIFWPackage::CompareNone ||
      !this->Compare.Value.empty()) {
    result += "-";
  }

  if (this->Compare.Type == cmCPackIFWPackage::CompareLessOrEqual) {
    result += "<=";
  } else if (this->Compare.Type == cmCPackIFWPackage::CompareGreaterOrEqual) {
    result += ">=";
  } else if (this->Compare.Type == cmCPackIFWPackage::CompareLess) {
    result += "<";
  } else if (this->Compare.Type == cmCPackIFWPackage::CompareEqual) {
    result += "=";
  } else if (this->Compare.Type == cmCPackIFWPackage::CompareGreater) {
    result += ">";
  }

  result += this->Compare.Value;

  return result;
}

//------------------------------------------------------ cmCPackIFWPackage ---
cmCPackIFWPackage::cmCPackIFWPackage()
  : Installer(nullptr)
{
}

std::string cmCPackIFWPackage::GetComponentName(cmCPackComponent* component)
{
  if (!component) {
    return "";
  }
  const char* option =
    this->GetOption("CPACK_IFW_COMPONENT_" +
                    cmsys::SystemTools::UpperCase(component->Name) + "_NAME");
  return option ? option : component->Name;
}

void cmCPackIFWPackage::DefaultConfiguration()
{
  this->DisplayName.clear();
  this->Description.clear();
  this->Version.clear();
  this->ReleaseDate.clear();
  this->Script.clear();
  this->Licenses.clear();
  this->UserInterfaces.clear();
  this->Translations.clear();
  this->SortingPriority.clear();
  this->UpdateText.clear();
  this->Default.clear();
  this->Essential.clear();
  this->Virtual.clear();
  this->ForcedInstallation.clear();
  this->RequiresAdminRights.clear();
}

// Default configuration (all in one package)
int cmCPackIFWPackage::ConfigureFromOptions()
{
  // Restore default configuration
  this->DefaultConfiguration();

  // Name
  this->Name = this->Generator->GetRootPackageName();

  // Display name
  if (const char* option = this->GetOption("CPACK_PACKAGE_NAME")) {
    this->DisplayName[""] = option;
  } else {
    this->DisplayName[""] = "Your package";
  }

  // Description
  if (const char* option =
        this->GetOption("CPACK_PACKAGE_DESCRIPTION_SUMMARY")) {
    this->Description[""] = option;
  } else {
    this->Description[""] = "Your package description";
  }

  // Version
  if (const char* option = this->GetOption("CPACK_PACKAGE_VERSION")) {
    this->Version = option;
  } else {
    this->Version = "1.0.0";
  }

  this->ForcedInstallation = "true";

  return 1;
}

int cmCPackIFWPackage::ConfigureFromComponent(cmCPackComponent* component)
{
  if (!component) {
    return 0;
  }

  // Restore default configuration
  this->DefaultConfiguration();

  std::string prefix = "CPACK_IFW_COMPONENT_" +
    cmsys::SystemTools::UpperCase(component->Name) + "_";

  // Display name
  this->DisplayName[""] = component->DisplayName;

  // Description
  this->Description[""] = component->Description;

  // Version
  if (const char* optVERSION = this->GetOption(prefix + "VERSION")) {
    this->Version = optVERSION;
  } else if (const char* optPACKAGE_VERSION =
               this->GetOption("CPACK_PACKAGE_VERSION")) {
    this->Version = optPACKAGE_VERSION;
  } else {
    this->Version = "1.0.0";
  }

  // Script
  if (const char* option = this->GetOption(prefix + "SCRIPT")) {
    this->Script = option;
  }

  // User interfaces
  if (const char* option = this->GetOption(prefix + "USER_INTERFACES")) {
    this->UserInterfaces.clear();
    cmSystemTools::ExpandListArgument(option, this->UserInterfaces);
  }

  // CMake dependencies
  if (!component->Dependencies.empty()) {
    for (cmCPackComponent* dep : component->Dependencies) {
      this->Dependencies.insert(this->Generator->ComponentPackages[dep]);
    }
  }

  // Licenses
  if (const char* option = this->GetOption(prefix + "LICENSES")) {
    this->Licenses.clear();
    cmSystemTools::ExpandListArgument(option, this->Licenses);
    if (this->Licenses.size() % 2 != 0) {
      cmCPackIFWLogger(
        WARNING,
        prefix << "LICENSES"
               << " should contain pairs of <display_name> and <file_path>."
               << std::endl);
      this->Licenses.clear();
    }
  }

  // Priority
  if (const char* option = this->GetOption(prefix + "PRIORITY")) {
    this->SortingPriority = option;
    cmCPackIFWLogger(
      WARNING,
      "The \"PRIORITY\" option is set "
        << "for component \"" << component->Name << "\", but there option is "
        << "deprecated. Please use \"SORTING_PRIORITY\" option instead."
        << std::endl);
  }

  // Default
  this->Default = component->IsDisabledByDefault ? "false" : "true";

  // Essential
  if (this->IsOn(prefix + "ESSENTIAL")) {
    this->Essential = "true";
  }

  // Virtual
  this->Virtual = component->IsHidden ? "true" : "";

  // ForcedInstallation
  this->ForcedInstallation = component->IsRequired ? "true" : "false";

  return this->ConfigureFromPrefix(prefix);
}

int cmCPackIFWPackage::ConfigureFromGroup(cmCPackComponentGroup* group)
{
  if (!group) {
    return 0;
  }

  // Restore default configuration
  this->DefaultConfiguration();

  std::string prefix = "CPACK_IFW_COMPONENT_GROUP_" +
    cmsys::SystemTools::UpperCase(group->Name) + "_";

  this->DisplayName[""] = group->DisplayName;
  this->Description[""] = group->Description;

  // Version
  if (const char* optVERSION = this->GetOption(prefix + "VERSION")) {
    this->Version = optVERSION;
  } else if (const char* optPACKAGE_VERSION =
               this->GetOption("CPACK_PACKAGE_VERSION")) {
    this->Version = optPACKAGE_VERSION;
  } else {
    this->Version = "1.0.0";
  }

  // Script
  if (const char* option = this->GetOption(prefix + "SCRIPT")) {
    this->Script = option;
  }

  // User interfaces
  if (const char* option = this->GetOption(prefix + "USER_INTERFACES")) {
    this->UserInterfaces.clear();
    cmSystemTools::ExpandListArgument(option, this->UserInterfaces);
  }

  // Licenses
  if (const char* option = this->GetOption(prefix + "LICENSES")) {
    this->Licenses.clear();
    cmSystemTools::ExpandListArgument(option, this->Licenses);
    if (this->Licenses.size() % 2 != 0) {
      cmCPackIFWLogger(
        WARNING,
        prefix << "LICENSES"
               << " should contain pairs of <display_name> and <file_path>."
               << std::endl);
      this->Licenses.clear();
    }
  }

  // Priority
  if (const char* option = this->GetOption(prefix + "PRIORITY")) {
    this->SortingPriority = option;
    cmCPackIFWLogger(
      WARNING,
      "The \"PRIORITY\" option is set "
        << "for component group \"" << group->Name
        << "\", but there option is "
        << "deprecated. Please use \"SORTING_PRIORITY\" option instead."
        << std::endl);
  }

  return this->ConfigureFromPrefix(prefix);
}

int cmCPackIFWPackage::ConfigureFromGroup(const std::string& groupName)
{
  // Group configuration

  cmCPackComponentGroup group;
  std::string prefix =
    "CPACK_COMPONENT_GROUP_" + cmsys::SystemTools::UpperCase(groupName) + "_";

  if (const char* option = this->GetOption(prefix + "DISPLAY_NAME")) {
    group.DisplayName = option;
  } else {
    group.DisplayName = group.Name;
  }

  if (const char* option = this->GetOption(prefix + "DESCRIPTION")) {
    group.Description = option;
  }
  group.IsBold = this->IsOn(prefix + "BOLD_TITLE");
  group.IsExpandedByDefault = this->IsOn(prefix + "EXPANDED");

  // Package configuration

  group.Name = groupName;

  if (Generator) {
    this->Name = this->Generator->GetGroupPackageName(&group);
  } else {
    this->Name = group.Name;
  }

  return this->ConfigureFromGroup(&group);
}

// Common options for components and groups
int cmCPackIFWPackage::ConfigureFromPrefix(const std::string& prefix)
{
  // Temporary variable for full option name
  std::string option;

  // Display name
  option = prefix + "DISPLAY_NAME";
  if (this->IsSetToEmpty(option)) {
    this->DisplayName.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->ExpandListArgument(value, this->DisplayName);
  }

  // Description
  option = prefix + "DESCRIPTION";
  if (this->IsSetToEmpty(option)) {
    this->Description.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->ExpandListArgument(value, this->Description);
  }

  // Release date
  option = prefix + "RELEASE_DATE";
  if (this->IsSetToEmpty(option)) {
    this->ReleaseDate.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->ReleaseDate = value;
  }

  // Sorting priority
  option = prefix + "SORTING_PRIORITY";
  if (this->IsSetToEmpty(option)) {
    this->SortingPriority.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->SortingPriority = value;
  }

  // Update text
  option = prefix + "UPDATE_TEXT";
  if (this->IsSetToEmpty(option)) {
    this->UpdateText.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->UpdateText = value;
  }

  // Translations
  option = prefix + "TRANSLATIONS";
  if (this->IsSetToEmpty(option)) {
    this->Translations.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->Translations.clear();
    cmSystemTools::ExpandListArgument(value, this->Translations);
  }

  // QtIFW dependencies
  std::vector<std::string> deps;
  option = prefix + "DEPENDS";
  if (const char* value = this->GetOption(option)) {
    cmSystemTools::ExpandListArgument(value, deps);
  }
  option = prefix + "DEPENDENCIES";
  if (const char* value = this->GetOption(option)) {
    cmSystemTools::ExpandListArgument(value, deps);
  }
  for (std::string const& d : deps) {
    DependenceStruct dep(d);
    if (this->Generator->Packages.count(dep.Name)) {
      cmCPackIFWPackage& depPkg = this->Generator->Packages[dep.Name];
      dep.Name = depPkg.Name;
    }
    bool hasDep = this->Generator->DependentPackages.count(dep.Name) > 0;
    DependenceStruct& depRef = this->Generator->DependentPackages[dep.Name];
    if (!hasDep) {
      depRef = dep;
    }
    this->AlienDependencies.insert(&depRef);
  }

  // Automatic dependency on
  option = prefix + "AUTO_DEPEND_ON";
  if (this->IsSetToEmpty(option)) {
    this->AlienAutoDependOn.clear();
  } else if (const char* value = this->GetOption(option)) {
    std::vector<std::string> depsOn;
    cmSystemTools::ExpandListArgument(value, depsOn);
    for (std::string const& d : depsOn) {
      DependenceStruct dep(d);
      if (this->Generator->Packages.count(dep.Name)) {
        cmCPackIFWPackage& depPkg = this->Generator->Packages[dep.Name];
        dep.Name = depPkg.Name;
      }
      bool hasDep = this->Generator->DependentPackages.count(dep.Name) > 0;
      DependenceStruct& depRef = this->Generator->DependentPackages[dep.Name];
      if (!hasDep) {
        depRef = dep;
      }
      this->AlienAutoDependOn.insert(&depRef);
    }
  }

  // Visibility
  option = prefix + "VIRTUAL";
  if (this->IsSetToEmpty(option)) {
    this->Virtual.clear();
  } else if (this->IsOn(option)) {
    this->Virtual = "true";
  }

  // Default selection
  option = prefix + "DEFAULT";
  if (this->IsSetToEmpty(option)) {
    this->Default.clear();
  } else if (const char* value = this->GetOption(option)) {
    std::string lowerValue = cmsys::SystemTools::LowerCase(value);
    if (lowerValue == "true") {
      this->Default = "true";
    } else if (lowerValue == "false") {
      this->Default = "false";
    } else if (lowerValue == "script") {
      this->Default = "script";
    } else {
      this->Default = value;
    }
  }

  // Forsed installation
  option = prefix + "FORCED_INSTALLATION";
  if (this->IsSetToEmpty(option)) {
    this->ForcedInstallation.clear();
  } else if (this->IsOn(option)) {
    this->ForcedInstallation = "true";
  } else if (this->IsSetToOff(option)) {
    this->ForcedInstallation = "false";
  }

  // Replaces
  option = prefix + "REPLACES";
  if (this->IsSetToEmpty(option)) {
    this->Replaces.clear();
  } else if (const char* value = this->GetOption(option)) {
    this->Replaces.clear();
    cmSystemTools::ExpandListArgument(value, this->Replaces);
  }

  // Requires admin rights
  option = prefix + "REQUIRES_ADMIN_RIGHTS";
  if (this->IsSetToEmpty(option)) {
    this->RequiresAdminRights.clear();
  } else if (this->IsOn(option)) {
    this->RequiresAdminRights = "true";
  } else if (this->IsSetToOff(option)) {
    this->RequiresAdminRights = "false";
  }

  // Checkable
  option = prefix + "CHECKABLE";
  if (this->IsSetToEmpty(option)) {
    this->Checkable.clear();
  } else if (this->IsOn(option)) {
    this->Checkable = "true";
  } else if (this->IsSetToOff(option)) {
    this->Checkable = "false";
  }

  return 1;
}

void cmCPackIFWPackage::GeneratePackageFile()
{
  // Lazy directory initialization
  if (this->Directory.empty()) {
    if (this->Installer) {
      this->Directory = this->Installer->Directory + "/packages/" + this->Name;
    } else if (this->Generator) {
      this->Directory = this->Generator->toplevel + "/packages/" + this->Name;
    }
  }

  // Output stream
  cmGeneratedFileStream fout(this->Directory + "/meta/package.xml");
  cmXMLWriter xout(fout);

  xout.StartDocument();

  WriteGeneratedByToStrim(xout);

  xout.StartElement("Package");

  // DisplayName (with translations)
  for (auto const& dn : this->DisplayName) {
    xout.StartElement("DisplayName");
    if (!dn.first.empty()) {
      xout.Attribute("xml:lang", dn.first);
    }
    xout.Content(dn.second);
    xout.EndElement();
  }

  // Description (with translations)
  for (auto const& d : this->Description) {
    xout.StartElement("Description");
    if (!d.first.empty()) {
      xout.Attribute("xml:lang", d.first);
    }
    xout.Content(d.second);
    xout.EndElement();
  }

  // Update text
  if (!this->UpdateText.empty()) {
    xout.Element("UpdateText", this->UpdateText);
  }

  xout.Element("Name", this->Name);
  xout.Element("Version", this->Version);

  if (!this->ReleaseDate.empty()) {
    xout.Element("ReleaseDate", this->ReleaseDate);
  } else {
    xout.Element("ReleaseDate", cmTimestamp().CurrentTime("%Y-%m-%d", true));
  }

  // Script (copy to meta dir)
  if (!this->Script.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->Script);
    std::string path = this->Directory + "/meta/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->Script, path);
    xout.Element("Script", name);
  }

  // User Interfaces (copy to meta dir)
  std::vector<std::string> userInterfaces = UserInterfaces;
  for (std::string& userInterface : userInterfaces) {
    std::string name = cmSystemTools::GetFilenameName(userInterface);
    std::string path = this->Directory + "/meta/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(userInterface, path);
    userInterface = name;
  }
  if (!userInterfaces.empty()) {
    xout.StartElement("UserInterfaces");
    for (std::string const& userInterface : userInterfaces) {
      xout.Element("UserInterface", userInterface);
    }
    xout.EndElement();
  }

  // Translations (copy to meta dir)
  std::vector<std::string> translations = Translations;
  for (std::string& translation : translations) {
    std::string name = cmSystemTools::GetFilenameName(translation);
    std::string path = this->Directory + "/meta/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(translation, path);
    translation = name;
  }
  if (!translations.empty()) {
    xout.StartElement("Translations");
    for (std::string const& translation : translations) {
      xout.Element("Translation", translation);
    }
    xout.EndElement();
  }

  // Dependencies
  std::set<DependenceStruct> compDepSet;
  for (DependenceStruct* ad : this->AlienDependencies) {
    compDepSet.insert(*ad);
  }
  for (cmCPackIFWPackage* d : this->Dependencies) {
    compDepSet.insert(DependenceStruct(d->Name));
  }
  // Write dependencies
  if (!compDepSet.empty()) {
    std::ostringstream dependencies;
    std::set<DependenceStruct>::iterator it = compDepSet.begin();
    dependencies << it->NameWithCompare();
    ++it;
    while (it != compDepSet.end()) {
      dependencies << "," << it->NameWithCompare();
      ++it;
    }
    xout.Element("Dependencies", dependencies.str());
  }

  // Automatic dependency on
  std::set<DependenceStruct> compAutoDepSet;
  for (DependenceStruct* aad : this->AlienAutoDependOn) {
    compAutoDepSet.insert(*aad);
  }
  // Write automatic dependency on
  if (!compAutoDepSet.empty()) {
    std::ostringstream dependencies;
    std::set<DependenceStruct>::iterator it = compAutoDepSet.begin();
    dependencies << it->NameWithCompare();
    ++it;
    while (it != compAutoDepSet.end()) {
      dependencies << "," << it->NameWithCompare();
      ++it;
    }
    xout.Element("AutoDependOn", dependencies.str());
  }

  // Licenses (copy to meta dir)
  std::vector<std::string> licenses = this->Licenses;
  for (size_t i = 1; i < licenses.size(); i += 2) {
    std::string name = cmSystemTools::GetFilenameName(licenses[i]);
    std::string path = this->Directory + "/meta/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(licenses[i], path);
    licenses[i] = name;
  }
  if (!licenses.empty()) {
    xout.StartElement("Licenses");
    for (size_t i = 0; i < licenses.size(); i += 2) {
      xout.StartElement("License");
      xout.Attribute("name", licenses[i]);
      xout.Attribute("file", licenses[i + 1]);
      xout.EndElement();
    }
    xout.EndElement();
  }

  if (!this->ForcedInstallation.empty()) {
    xout.Element("ForcedInstallation", this->ForcedInstallation);
  }

  // Replaces
  if (!this->Replaces.empty()) {
    std::ostringstream replaces;
    std::vector<std::string>::iterator it = this->Replaces.begin();
    replaces << *it;
    ++it;
    while (it != this->Replaces.end()) {
      replaces << "," << *it;
      ++it;
    }
    xout.Element("Replaces", replaces.str());
  }

  if (!this->RequiresAdminRights.empty()) {
    xout.Element("RequiresAdminRights", this->RequiresAdminRights);
  }

  if (!this->Virtual.empty()) {
    xout.Element("Virtual", this->Virtual);
  } else if (!this->Default.empty()) {
    xout.Element("Default", this->Default);
  }

  // Essential
  if (!this->Essential.empty()) {
    xout.Element("Essential", this->Essential);
  }

  // Priority
  if (!this->SortingPriority.empty()) {
    xout.Element("SortingPriority", this->SortingPriority);
  }

  // Checkable
  if (!this->Checkable.empty()) {
    xout.Element("Checkable", this->Checkable);
  }

  xout.EndElement();
  xout.EndDocument();
}
