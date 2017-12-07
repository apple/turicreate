/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackIFWGenerator.h"

#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"
#include "cmCPackIFWCommon.h"
#include "cmCPackIFWInstaller.h"
#include "cmCPackIFWPackage.h"
#include "cmCPackIFWRepository.h"
#include "cmCPackLog.h" // IWYU pragma: keep
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"

#include <sstream>
#include <utility>

cmCPackIFWGenerator::cmCPackIFWGenerator()
{
  this->Generator = this;
}

cmCPackIFWGenerator::~cmCPackIFWGenerator()
{
}

int cmCPackIFWGenerator::PackageFiles()
{
  cmCPackIFWLogger(OUTPUT, "- Configuration" << std::endl);

  // Installer configuragion
  this->Installer.GenerateInstallerFile();

  // Packages configuration
  this->Installer.GeneratePackageFiles();

  std::string ifwTLD = this->GetOption("CPACK_TOPLEVEL_DIRECTORY");
  std::string ifwTmpFile = ifwTLD;
  ifwTmpFile += "/IFWOutput.log";

  // Run repogen
  if (!this->Installer.RemoteRepositories.empty()) {
    std::string ifwCmd = this->RepoGen;

    if (this->IsVersionLess("2.0.0")) {
      ifwCmd += " -c " + this->toplevel + "/config/config.xml";
    }

    ifwCmd += " -p " + this->toplevel + "/packages";

    if (!this->PkgsDirsVector.empty()) {
      for (std::vector<std::string>::iterator it =
             this->PkgsDirsVector.begin();
           it != this->PkgsDirsVector.end(); ++it) {
        ifwCmd += " -p " + *it;
      }
    }

    if (!this->OnlineOnly && !this->DownloadedPackages.empty()) {
      ifwCmd += " -i ";
      std::set<cmCPackIFWPackage*>::iterator it =
        this->DownloadedPackages.begin();
      ifwCmd += (*it)->Name;
      ++it;
      while (it != this->DownloadedPackages.end()) {
        ifwCmd += "," + (*it)->Name;
        ++it;
      }
    }
    ifwCmd += " " + this->toplevel + "/repository";
    cmCPackIFWLogger(VERBOSE, "Execute: " << ifwCmd << std::endl);
    std::string output;
    int retVal = 1;
    cmCPackIFWLogger(OUTPUT, "- Generate repository" << std::endl);
    bool res = cmSystemTools::RunSingleCommand(ifwCmd.c_str(), &output,
                                               &output, &retVal, CM_NULLPTR,
                                               this->GeneratorVerbose, 0);
    if (!res || retVal) {
      cmGeneratedFileStream ofs(ifwTmpFile.c_str());
      ofs << "# Run command: " << ifwCmd << std::endl
          << "# Output:" << std::endl
          << output << std::endl;
      cmCPackIFWLogger(ERROR, "Problem running IFW command: "
                         << ifwCmd << std::endl
                         << "Please check " << ifwTmpFile << " for errors"
                         << std::endl);
      return 0;
    }

    if (!this->Repository.RepositoryUpdate.empty() &&
        !this->Repository.PatchUpdatesXml()) {
      cmCPackIFWLogger(WARNING, "Problem patch IFW \"Updates\" "
                         << "file: "
                         << this->toplevel + "/repository/Updates.xml"
                         << std::endl);
    }

    cmCPackIFWLogger(OUTPUT, "- repository: " << this->toplevel
                                              << "/repository generated"
                                              << std::endl);
  }

  // Run binary creator
  {
    std::string ifwCmd = this->BinCreator;
    ifwCmd += " -c " + this->toplevel + "/config/config.xml";

    if (!this->Installer.Resources.empty()) {
      ifwCmd += " -r ";
      std::vector<std::string>::iterator it =
        this->Installer.Resources.begin();
      std::string path = this->toplevel + "/resources/";
      ifwCmd += path + *it;
      ++it;
      while (it != this->Installer.Resources.end()) {
        ifwCmd += "," + path + *it;
        ++it;
      }
    }

    ifwCmd += " -p " + this->toplevel + "/packages";

    if (!this->PkgsDirsVector.empty()) {
      for (std::vector<std::string>::iterator it =
             this->PkgsDirsVector.begin();
           it != this->PkgsDirsVector.end(); ++it) {
        ifwCmd += " -p " + *it;
      }
    }

    if (this->OnlineOnly) {
      ifwCmd += " --online-only";
    } else if (!this->DownloadedPackages.empty() &&
               !this->Installer.RemoteRepositories.empty()) {
      ifwCmd += " -e ";
      std::set<cmCPackIFWPackage*>::iterator it =
        this->DownloadedPackages.begin();
      ifwCmd += (*it)->Name;
      ++it;
      while (it != this->DownloadedPackages.end()) {
        ifwCmd += "," + (*it)->Name;
        ++it;
      }
    } else if (!this->DependentPackages.empty()) {
      ifwCmd += " -i ";
      // Binary
      std::set<cmCPackIFWPackage*>::iterator bit =
        this->BinaryPackages.begin();
      while (bit != this->BinaryPackages.end()) {
        ifwCmd += (*bit)->Name + ",";
        ++bit;
      }
      // Depend
      DependenceMap::iterator it = this->DependentPackages.begin();
      ifwCmd += it->second.Name;
      ++it;
      while (it != this->DependentPackages.end()) {
        ifwCmd += "," + it->second.Name;
        ++it;
      }
    }
    // TODO: set correct name for multipackages
    if (!this->packageFileNames.empty()) {
      ifwCmd += " " + this->packageFileNames[0];
    } else {
      ifwCmd += " installer";
    }
    cmCPackIFWLogger(VERBOSE, "Execute: " << ifwCmd << std::endl);
    std::string output;
    int retVal = 1;
    cmCPackIFWLogger(OUTPUT, "- Generate package" << std::endl);
    bool res = cmSystemTools::RunSingleCommand(ifwCmd.c_str(), &output,
                                               &output, &retVal, CM_NULLPTR,
                                               this->GeneratorVerbose, 0);
    if (!res || retVal) {
      cmGeneratedFileStream ofs(ifwTmpFile.c_str());
      ofs << "# Run command: " << ifwCmd << std::endl
          << "# Output:" << std::endl
          << output << std::endl;
      cmCPackIFWLogger(ERROR, "Problem running IFW command: "
                         << ifwCmd << std::endl
                         << "Please check " << ifwTmpFile << " for errors"
                         << std::endl);
      return 0;
    }
  }

  return 1;
}

const char* cmCPackIFWGenerator::GetPackagingInstallPrefix()
{
  const char* defPrefix = this->cmCPackGenerator::GetPackagingInstallPrefix();

  std::string tmpPref = defPrefix ? defPrefix : "";

  if (this->Components.empty()) {
    tmpPref += "packages/" + this->GetRootPackageName() + "/data";
  }

  this->SetOption("CPACK_IFW_PACKAGING_INSTALL_PREFIX", tmpPref.c_str());

  return this->GetOption("CPACK_IFW_PACKAGING_INSTALL_PREFIX");
}

const char* cmCPackIFWGenerator::GetOutputExtension()
{
  return this->ExecutableSuffix.c_str();
}

int cmCPackIFWGenerator::InitializeInternal()
{
  // Search Qt Installer Framework tools

  const std::string BinCreatorOpt = "CPACK_IFW_BINARYCREATOR_EXECUTABLE";
  const std::string RepoGenOpt = "CPACK_IFW_REPOGEN_EXECUTABLE";
  const std::string FrameworkVersionOpt = "CPACK_IFW_FRAMEWORK_VERSION";

  if (!this->IsSet(BinCreatorOpt) || !this->IsSet(RepoGenOpt) ||
      !this->IsSet(FrameworkVersionOpt)) {
    this->ReadListFile("CPackIFW.cmake");
  }

  // Look 'binarycreator' executable (needs)

  const char* BinCreatorStr = this->GetOption(BinCreatorOpt);
  if (!BinCreatorStr || cmSystemTools::IsNOTFOUND(BinCreatorStr)) {
    this->BinCreator = "";
  } else {
    this->BinCreator = BinCreatorStr;
  }

  if (this->BinCreator.empty()) {
    cmCPackIFWLogger(ERROR, "Cannot find QtIFW compiler \"binarycreator\": "
                            "likely it is not installed, or not in your PATH"
                       << std::endl);
    return 0;
  }

  // Look 'repogen' executable (optional)

  const char* RepoGenStr = this->GetOption(RepoGenOpt);
  if (!RepoGenStr || cmSystemTools::IsNOTFOUND(RepoGenStr)) {
    this->RepoGen = "";
  } else {
    this->RepoGen = RepoGenStr;
  }

  // Framework version
  if (const char* FrameworkVersionSrt = this->GetOption(FrameworkVersionOpt)) {
    this->FrameworkVersion = FrameworkVersionSrt;
  } else {
    this->FrameworkVersion = "1.9.9";
  }

  // Variables that Change Behavior

  // Resolve duplicate names
  this->ResolveDuplicateNames =
    this->IsOn("CPACK_IFW_RESOLVE_DUPLICATE_NAMES");

  // Additional packages dirs
  this->PkgsDirsVector.clear();
  if (const char* dirs = this->GetOption("CPACK_IFW_PACKAGES_DIRECTORIES")) {
    cmSystemTools::ExpandListArgument(dirs, this->PkgsDirsVector);
  }

  // Installer
  this->Installer.Generator = this;
  this->Installer.ConfigureFromOptions();

  // Repository
  this->Repository.Generator = this;
  this->Repository.Name = "Unspecified";
  if (const char* site = this->GetOption("CPACK_DOWNLOAD_SITE")) {
    this->Repository.Url = site;
    this->Installer.RemoteRepositories.push_back(&this->Repository);
  }

  // Repositories
  if (const char* RepoAllStr = this->GetOption("CPACK_IFW_REPOSITORIES_ALL")) {
    std::vector<std::string> RepoAllVector;
    cmSystemTools::ExpandListArgument(RepoAllStr, RepoAllVector);
    for (std::vector<std::string>::iterator rit = RepoAllVector.begin();
         rit != RepoAllVector.end(); ++rit) {
      this->GetRepository(*rit);
    }
  }

  if (const char* ifwDownloadAll = this->GetOption("CPACK_IFW_DOWNLOAD_ALL")) {
    this->OnlineOnly = cmSystemTools::IsOn(ifwDownloadAll);
  } else if (const char* cpackDownloadAll =
               this->GetOption("CPACK_DOWNLOAD_ALL")) {
    this->OnlineOnly = cmSystemTools::IsOn(cpackDownloadAll);
  } else {
    this->OnlineOnly = false;
  }

  if (!this->Installer.RemoteRepositories.empty() && this->RepoGen.empty()) {
    cmCPackIFWLogger(ERROR,
                     "Cannot find QtIFW repository generator \"repogen\": "
                     "likely it is not installed, or not in your PATH"
                       << std::endl);
    return 0;
  }

  // Executable suffix
  if (const char* optExeSuffix = this->GetOption("CMAKE_EXECUTABLE_SUFFIX")) {
    this->ExecutableSuffix = optExeSuffix;
    if (this->ExecutableSuffix.empty()) {
      std::string sysName(this->GetOption("CMAKE_SYSTEM_NAME"));
      if (sysName == "Linux") {
        this->ExecutableSuffix = ".run";
      }
    }
  } else {
    this->ExecutableSuffix = this->cmCPackGenerator::GetOutputExtension();
  }

  return this->Superclass::InitializeInternal();
}

std::string cmCPackIFWGenerator::GetComponentInstallDirNameSuffix(
  const std::string& componentName)
{
  const std::string prefix = "packages/";
  const std::string suffix = "/data";

  if (this->componentPackageMethod == this->ONE_PACKAGE) {
    return std::string(prefix + this->GetRootPackageName() + suffix);
  }

  return prefix +
    this->GetComponentPackageName(&this->Components[componentName]) + suffix;
}

cmCPackComponent* cmCPackIFWGenerator::GetComponent(
  const std::string& projectName, const std::string& componentName)
{
  ComponentsMap::iterator cit = this->Components.find(componentName);
  if (cit != this->Components.end()) {
    return &(cit->second);
  }

  cmCPackComponent* component =
    this->cmCPackGenerator::GetComponent(projectName, componentName);
  if (!component) {
    return component;
  }

  std::string name = this->GetComponentPackageName(component);
  PackagesMap::iterator pit = this->Packages.find(name);
  if (pit != this->Packages.end()) {
    return component;
  }

  cmCPackIFWPackage* package = &this->Packages[name];
  package->Name = name;
  package->Generator = this;
  if (package->ConfigureFromComponent(component)) {
    package->Installer = &this->Installer;
    this->Installer.Packages.insert(
      std::pair<std::string, cmCPackIFWPackage*>(name, package));
    this->ComponentPackages.insert(
      std::pair<cmCPackComponent*, cmCPackIFWPackage*>(component, package));
    if (component->IsDownloaded) {
      this->DownloadedPackages.insert(package);
    } else {
      this->BinaryPackages.insert(package);
    }
  } else {
    this->Packages.erase(name);
    cmCPackIFWLogger(ERROR, "Cannot configure package \""
                       << name << "\" for component \"" << component->Name
                       << "\"" << std::endl);
  }

  return component;
}

cmCPackComponentGroup* cmCPackIFWGenerator::GetComponentGroup(
  const std::string& projectName, const std::string& groupName)
{
  cmCPackComponentGroup* group =
    this->cmCPackGenerator::GetComponentGroup(projectName, groupName);
  if (!group) {
    return group;
  }

  std::string name = this->GetGroupPackageName(group);
  PackagesMap::iterator pit = this->Packages.find(name);
  if (pit != this->Packages.end()) {
    return group;
  }

  cmCPackIFWPackage* package = &this->Packages[name];
  package->Name = name;
  package->Generator = this;
  if (package->ConfigureFromGroup(group)) {
    package->Installer = &this->Installer;
    this->Installer.Packages.insert(
      std::pair<std::string, cmCPackIFWPackage*>(name, package));
    this->GroupPackages.insert(
      std::pair<cmCPackComponentGroup*, cmCPackIFWPackage*>(group, package));
    this->BinaryPackages.insert(package);
  } else {
    this->Packages.erase(name);
    cmCPackIFWLogger(ERROR, "Cannot configure package \""
                       << name << "\" for component group \"" << group->Name
                       << "\"" << std::endl);
  }
  return group;
}

enum cmCPackGenerator::CPackSetDestdirSupport
cmCPackIFWGenerator::SupportsSetDestdir() const
{
  return cmCPackGenerator::SETDESTDIR_SHOULD_NOT_BE_USED;
}

bool cmCPackIFWGenerator::SupportsAbsoluteDestination() const
{
  return false;
}

bool cmCPackIFWGenerator::SupportsComponentInstallation() const
{
  return true;
}

bool cmCPackIFWGenerator::IsOnePackage() const
{
  return this->componentPackageMethod == cmCPackGenerator::ONE_PACKAGE;
}

std::string cmCPackIFWGenerator::GetRootPackageName()
{
  // Default value
  std::string name = "root";
  if (const char* optIFW_PACKAGE_GROUP =
        this->GetOption("CPACK_IFW_PACKAGE_GROUP")) {
    // Configure from root group
    cmCPackIFWPackage package;
    package.Generator = this;
    package.ConfigureFromGroup(optIFW_PACKAGE_GROUP);
    name = package.Name;
  } else if (const char* optIFW_PACKAGE_NAME =
               this->GetOption("CPACK_IFW_PACKAGE_NAME")) {
    // Configure from root package name
    name = optIFW_PACKAGE_NAME;
  } else if (const char* optPACKAGE_NAME =
               this->GetOption("CPACK_PACKAGE_NAME")) {
    // Configure from package name
    name = optPACKAGE_NAME;
  }
  return name;
}

std::string cmCPackIFWGenerator::GetGroupPackageName(
  cmCPackComponentGroup* group) const
{
  std::string name;
  if (!group) {
    return name;
  }
  if (cmCPackIFWPackage* package = this->GetGroupPackage(group)) {
    return package->Name;
  }
  const char* option =
    this->GetOption("CPACK_IFW_COMPONENT_GROUP_" +
                    cmsys::SystemTools::UpperCase(group->Name) + "_NAME");
  name = option ? option : group->Name;
  if (group->ParentGroup) {
    cmCPackIFWPackage* package = this->GetGroupPackage(group->ParentGroup);
    bool dot = !this->ResolveDuplicateNames;
    if (dot && name.substr(0, package->Name.size()) == package->Name) {
      dot = false;
    }
    if (dot) {
      name = package->Name + "." + name;
    }
  }
  return name;
}

std::string cmCPackIFWGenerator::GetComponentPackageName(
  cmCPackComponent* component) const
{
  std::string name;
  if (!component) {
    return name;
  }
  if (cmCPackIFWPackage* package = this->GetComponentPackage(component)) {
    return package->Name;
  }
  std::string prefix = "CPACK_IFW_COMPONENT_" +
    cmsys::SystemTools::UpperCase(component->Name) + "_";
  const char* option = this->GetOption(prefix + "NAME");
  name = option ? option : component->Name;
  if (component->Group) {
    cmCPackIFWPackage* package = this->GetGroupPackage(component->Group);
    if ((this->componentPackageMethod ==
         cmCPackGenerator::ONE_PACKAGE_PER_GROUP) ||
        this->IsOn(prefix + "COMMON")) {
      return package->Name;
    }
    bool dot = !this->ResolveDuplicateNames;
    if (dot && name.substr(0, package->Name.size()) == package->Name) {
      dot = false;
    }
    if (dot) {
      name = package->Name + "." + name;
    }
  }
  return name;
}

cmCPackIFWPackage* cmCPackIFWGenerator::GetGroupPackage(
  cmCPackComponentGroup* group) const
{
  std::map<cmCPackComponentGroup*, cmCPackIFWPackage*>::const_iterator pit =
    this->GroupPackages.find(group);
  return pit != this->GroupPackages.end() ? pit->second : CM_NULLPTR;
}

cmCPackIFWPackage* cmCPackIFWGenerator::GetComponentPackage(
  cmCPackComponent* component) const
{
  std::map<cmCPackComponent*, cmCPackIFWPackage*>::const_iterator pit =
    this->ComponentPackages.find(component);
  return pit != this->ComponentPackages.end() ? pit->second : CM_NULLPTR;
}

cmCPackIFWRepository* cmCPackIFWGenerator::GetRepository(
  const std::string& repositoryName)
{
  RepositoriesMap::iterator rit = this->Repositories.find(repositoryName);
  if (rit != this->Repositories.end()) {
    return &(rit->second);
  }

  cmCPackIFWRepository* repository = &this->Repositories[repositoryName];
  repository->Name = repositoryName;
  repository->Generator = this;
  if (repository->ConfigureFromOptions()) {
    if (repository->Update == cmCPackIFWRepository::None) {
      this->Installer.RemoteRepositories.push_back(repository);
    } else {
      this->Repository.RepositoryUpdate.push_back(repository);
    }
  } else {
    this->Repositories.erase(repositoryName);
    repository = CM_NULLPTR;
    cmCPackIFWLogger(WARNING, "Invalid repository \""
                       << repositoryName << "\""
                       << " configuration. Repository will be skipped."
                       << std::endl);
  }
  return repository;
}
