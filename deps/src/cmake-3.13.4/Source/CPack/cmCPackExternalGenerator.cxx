/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackExternalGenerator.h"

#include "cmAlgorithms.h"
#include "cmCPackComponentGroup.h"
#include "cmCPackLog.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

#include "cm_jsoncpp_value.h"
#include "cm_jsoncpp_writer.h"

#include "cmsys/FStream.hxx"

#include <utility>
#include <vector>

int cmCPackExternalGenerator::InitializeInternal()
{
  this->SetOption("CPACK_EXTERNAL_KNOWN_VERSIONS", "1.0");

  if (!this->ReadListFile("Internal/CPack/CPackExternal.cmake")) {
    cmCPackLogger(cmCPackLog::LOG_ERROR,
                  "Error while executing CPackExternal.cmake" << std::endl);
    return 0;
  }

  std::string major = this->GetOption("CPACK_EXTERNAL_SELECTED_MAJOR");
  if (major == "1") {
    this->Generator = cm::make_unique<cmCPackExternalVersion1Generator>(this);
  }

  return this->Superclass::InitializeInternal();
}

int cmCPackExternalGenerator::PackageFiles()
{
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  ";

  std::string filename = "package.json";
  if (!this->packageFileNames.empty()) {
    filename = this->packageFileNames[0];
  }

  cmsys::ofstream fout(filename.c_str());
  std::unique_ptr<Json::StreamWriter> jout(builder.newStreamWriter());

  Json::Value root(Json::objectValue);

  if (!this->Generator->WriteToJSON(root)) {
    return 0;
  }

  if (jout->write(root, &fout)) {
    return 0;
  }

  const char* packageScript = this->GetOption("CPACK_EXTERNAL_PACKAGE_SCRIPT");
  if (packageScript && *packageScript) {
    if (!cmSystemTools::FileIsFullPath(packageScript)) {
      cmCPackLogger(
        cmCPackLog::LOG_ERROR,
        "CPACK_EXTERNAL_PACKAGE_SCRIPT does not contain a full file path"
          << std::endl);
      return 0;
    }

    int res = this->MakefileMap->ReadListFile(packageScript);

    if (cmSystemTools::GetErrorOccuredFlag() || !res) {
      return 0;
    }
  }

  return 1;
}

bool cmCPackExternalGenerator::SupportsComponentInstallation() const
{
  return true;
}

int cmCPackExternalGenerator::InstallProjectViaInstallCommands(
  bool setDestDir, const std::string& tempInstallDirectory)
{
  if (this->StagingEnabled()) {
    return cmCPackGenerator::InstallProjectViaInstallCommands(
      setDestDir, tempInstallDirectory);
  }

  return 1;
}

int cmCPackExternalGenerator::InstallProjectViaInstallScript(
  bool setDestDir, const std::string& tempInstallDirectory)
{
  if (this->StagingEnabled()) {
    return cmCPackGenerator::InstallProjectViaInstallScript(
      setDestDir, tempInstallDirectory);
  }

  return 1;
}

int cmCPackExternalGenerator::InstallProjectViaInstalledDirectories(
  bool setDestDir, const std::string& tempInstallDirectory,
  const mode_t* default_dir_mode)
{
  if (this->StagingEnabled()) {
    return cmCPackGenerator::InstallProjectViaInstalledDirectories(
      setDestDir, tempInstallDirectory, default_dir_mode);
  }

  return 1;
}

int cmCPackExternalGenerator::RunPreinstallTarget(
  const std::string& installProjectName, const std::string& installDirectory,
  cmGlobalGenerator* globalGenerator, const std::string& buildConfig)
{
  if (this->StagingEnabled()) {
    return cmCPackGenerator::RunPreinstallTarget(
      installProjectName, installDirectory, globalGenerator, buildConfig);
  }

  return 1;
}

int cmCPackExternalGenerator::InstallCMakeProject(
  bool setDestDir, const std::string& installDirectory,
  const std::string& baseTempInstallDirectory, const mode_t* default_dir_mode,
  const std::string& component, bool componentInstall,
  const std::string& installSubDirectory, const std::string& buildConfig,
  std::string& absoluteDestFiles)
{
  if (this->StagingEnabled()) {
    return cmCPackGenerator::InstallCMakeProject(
      setDestDir, installDirectory, baseTempInstallDirectory, default_dir_mode,
      component, componentInstall, installSubDirectory, buildConfig,
      absoluteDestFiles);
  }

  return 1;
}

bool cmCPackExternalGenerator::StagingEnabled() const
{
  return !cmSystemTools::IsOff(
    this->GetOption("CPACK_EXTERNAL_ENABLE_STAGING"));
}

cmCPackExternalGenerator::cmCPackExternalVersionGenerator::
  cmCPackExternalVersionGenerator(cmCPackExternalGenerator* parent)
  : Parent(parent)
{
}

int cmCPackExternalGenerator::cmCPackExternalVersionGenerator::WriteVersion(
  Json::Value& root)
{
  root["formatVersionMajor"] = this->GetVersionMajor();
  root["formatVersionMinor"] = this->GetVersionMinor();

  return 1;
}

int cmCPackExternalGenerator::cmCPackExternalVersionGenerator::WriteToJSON(
  Json::Value& root)
{
  if (!this->WriteVersion(root)) {
    return 0;
  }

  const char* packageName = this->Parent->GetOption("CPACK_PACKAGE_NAME");
  if (packageName) {
    root["packageName"] = packageName;
  }

  const char* packageVersion =
    this->Parent->GetOption("CPACK_PACKAGE_VERSION");
  if (packageVersion) {
    root["packageVersion"] = packageVersion;
  }

  const char* packageDescriptionFile =
    this->Parent->GetOption("CPACK_PACKAGE_DESCRIPTION_FILE");
  if (packageDescriptionFile) {
    root["packageDescriptionFile"] = packageDescriptionFile;
  }

  const char* packageDescriptionSummary =
    this->Parent->GetOption("CPACK_PACKAGE_DESCRIPTION_SUMMARY");
  if (packageDescriptionSummary) {
    root["packageDescriptionSummary"] = packageDescriptionSummary;
  }

  const char* buildConfigCstr = this->Parent->GetOption("CPACK_BUILD_CONFIG");
  if (buildConfigCstr) {
    root["buildConfig"] = buildConfigCstr;
  }

  const char* defaultDirectoryPermissions =
    this->Parent->GetOption("CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS");
  if (defaultDirectoryPermissions && *defaultDirectoryPermissions) {
    root["defaultDirectoryPermissions"] = defaultDirectoryPermissions;
  }
  if (cmSystemTools::IsInternallyOn(
        this->Parent->GetOption("CPACK_SET_DESTDIR"))) {
    root["setDestdir"] = true;
    root["packagingInstallPrefix"] =
      this->Parent->GetOption("CPACK_PACKAGING_INSTALL_PREFIX");
  } else {
    root["setDestdir"] = false;
  }

  root["stripFiles"] =
    !cmSystemTools::IsOff(this->Parent->GetOption("CPACK_STRIP_FILES"));
  root["warnOnAbsoluteInstallDestination"] =
    this->Parent->IsOn("CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION");
  root["errorOnAbsoluteInstallDestination"] =
    this->Parent->IsOn("CPACK_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION");

  Json::Value& projects = root["projects"] = Json::Value(Json::arrayValue);
  for (auto& project : this->Parent->CMakeProjects) {
    Json::Value jsonProject(Json::objectValue);

    jsonProject["projectName"] = project.ProjectName;
    jsonProject["component"] = project.Component;
    jsonProject["directory"] = project.Directory;
    jsonProject["subDirectory"] = project.SubDirectory;

    Json::Value& installationTypes = jsonProject["installationTypes"] =
      Json::Value(Json::arrayValue);
    for (auto& installationType : project.InstallationTypes) {
      installationTypes.append(installationType->Name);
    }

    Json::Value& components = jsonProject["components"] =
      Json::Value(Json::arrayValue);
    for (auto& component : project.Components) {
      components.append(component->Name);
    }

    projects.append(jsonProject);
  }

  Json::Value& installationTypes = root["installationTypes"] =
    Json::Value(Json::objectValue);
  for (auto& installationType : this->Parent->InstallationTypes) {
    Json::Value& jsonInstallationType =
      installationTypes[installationType.first] =
        Json::Value(Json::objectValue);

    jsonInstallationType["name"] = installationType.second.Name;
    jsonInstallationType["displayName"] = installationType.second.DisplayName;
    jsonInstallationType["index"] = installationType.second.Index;
  }

  Json::Value& components = root["components"] =
    Json::Value(Json::objectValue);
  for (auto& component : this->Parent->Components) {
    Json::Value& jsonComponent = components[component.first] =
      Json::Value(Json::objectValue);

    jsonComponent["name"] = component.second.Name;
    jsonComponent["displayName"] = component.second.DisplayName;
    if (component.second.Group) {
      jsonComponent["group"] = component.second.Group->Name;
    }
    jsonComponent["isRequired"] = component.second.IsRequired;
    jsonComponent["isHidden"] = component.second.IsHidden;
    jsonComponent["isDisabledByDefault"] =
      component.second.IsDisabledByDefault;
    jsonComponent["isDownloaded"] = component.second.IsDownloaded;
    jsonComponent["description"] = component.second.Description;
    jsonComponent["archiveFile"] = component.second.ArchiveFile;

    Json::Value& cmpInstallationTypes = jsonComponent["installationTypes"] =
      Json::Value(Json::arrayValue);
    for (auto& installationType : component.second.InstallationTypes) {
      cmpInstallationTypes.append(installationType->Name);
    }

    Json::Value& dependencies = jsonComponent["dependencies"] =
      Json::Value(Json::arrayValue);
    for (auto& dep : component.second.Dependencies) {
      dependencies.append(dep->Name);
    }
  }

  Json::Value& groups = root["componentGroups"] =
    Json::Value(Json::objectValue);
  for (auto& group : this->Parent->ComponentGroups) {
    Json::Value& jsonGroup = groups[group.first] =
      Json::Value(Json::objectValue);

    jsonGroup["name"] = group.second.Name;
    jsonGroup["displayName"] = group.second.DisplayName;
    jsonGroup["description"] = group.second.Description;
    jsonGroup["isBold"] = group.second.IsBold;
    jsonGroup["isExpandedByDefault"] = group.second.IsExpandedByDefault;
    if (group.second.ParentGroup) {
      jsonGroup["parentGroup"] = group.second.ParentGroup->Name;
    }

    Json::Value& subgroups = jsonGroup["subgroups"] =
      Json::Value(Json::arrayValue);
    for (auto& subgroup : group.second.Subgroups) {
      subgroups.append(subgroup->Name);
    }

    Json::Value& groupComponents = jsonGroup["components"] =
      Json::Value(Json::arrayValue);
    for (auto& component : group.second.Components) {
      groupComponents.append(component->Name);
    }
  }

  return 1;
}
