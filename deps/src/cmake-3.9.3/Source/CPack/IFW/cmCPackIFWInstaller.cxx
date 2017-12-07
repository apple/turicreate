/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackIFWInstaller.h"

#include "cmCPackIFWCommon.h"
#include "cmCPackIFWGenerator.h"
#include "cmCPackIFWPackage.h"
#include "cmCPackIFWRepository.h"
#include "cmCPackLog.h" // IWYU pragma: keep
#include "cmGeneratedFileStream.h"
#include "cmSystemTools.h"
#include "cmXMLParser.h"
#include "cmXMLWriter.h"

#include <sstream>
#include <stddef.h>
#include <utility>

cmCPackIFWInstaller::cmCPackIFWInstaller()
{
}

void cmCPackIFWInstaller::printSkippedOptionWarning(
  const std::string& optionName, const std::string& optionValue)
{
  cmCPackIFWLogger(
    WARNING, "Option "
      << optionName << " is set to \"" << optionValue
      << "\" but will be skipped because the specified file does not exist."
      << std::endl);
}

void cmCPackIFWInstaller::ConfigureFromOptions()
{
  // Name;
  if (const char* optIFW_PACKAGE_NAME =
        this->GetOption("CPACK_IFW_PACKAGE_NAME")) {
    this->Name = optIFW_PACKAGE_NAME;
  } else if (const char* optPACKAGE_NAME =
               this->GetOption("CPACK_PACKAGE_NAME")) {
    this->Name = optPACKAGE_NAME;
  } else {
    this->Name = "Your package";
  }

  // Title;
  if (const char* optIFW_PACKAGE_TITLE =
        this->GetOption("CPACK_IFW_PACKAGE_TITLE")) {
    this->Title = optIFW_PACKAGE_TITLE;
  } else if (const char* optPACKAGE_DESCRIPTION_SUMMARY =
               this->GetOption("CPACK_PACKAGE_DESCRIPTION_SUMMARY")) {
    this->Title = optPACKAGE_DESCRIPTION_SUMMARY;
  } else {
    this->Title = "Your package description";
  }

  // Version;
  if (const char* option = this->GetOption("CPACK_PACKAGE_VERSION")) {
    this->Version = option;
  } else {
    this->Version = "1.0.0";
  }

  // Publisher
  if (const char* optIFW_PACKAGE_PUBLISHER =
        this->GetOption("CPACK_IFW_PACKAGE_PUBLISHER")) {
    this->Publisher = optIFW_PACKAGE_PUBLISHER;
  } else if (const char* optPACKAGE_VENDOR =
               GetOption("CPACK_PACKAGE_VENDOR")) {
    this->Publisher = optPACKAGE_VENDOR;
  }

  // ProductUrl
  if (const char* option = this->GetOption("CPACK_IFW_PRODUCT_URL")) {
    this->ProductUrl = option;
  }

  // ApplicationIcon
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_ICON")) {
    if (cmSystemTools::FileExists(option)) {
      this->InstallerApplicationIcon = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_ICON", option);
    }
  }

  // WindowIcon
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_WINDOW_ICON")) {
    if (cmSystemTools::FileExists(option)) {
      this->InstallerWindowIcon = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_WINDOW_ICON", option);
    }
  }

  // Logo
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_LOGO")) {
    if (cmSystemTools::FileExists(option)) {
      this->Logo = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_LOGO", option);
    }
  }

  // Watermark
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_WATERMARK")) {
    if (cmSystemTools::FileExists(option)) {
      this->Watermark = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_WATERMARK", option);
    }
  }

  // Banner
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_BANNER")) {
    if (cmSystemTools::FileExists(option)) {
      this->Banner = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_BANNER", option);
    }
  }

  // Background
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_BACKGROUND")) {
    if (cmSystemTools::FileExists(option)) {
      this->Background = option;
    } else {
      this->printSkippedOptionWarning("CPACK_IFW_PACKAGE_BACKGROUND", option);
    }
  }

  // WizardStyle
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_WIZARD_STYLE")) {
    // Setting the user value in any case
    this->WizardStyle = option;
    // Check known values
    if (this->WizardStyle != "Modern" && this->WizardStyle != "Aero" &&
        this->WizardStyle != "Mac" && this->WizardStyle != "Classic") {
      cmCPackIFWLogger(
        WARNING, "Option CPACK_IFW_PACKAGE_WIZARD_STYLE has unknown value \""
          << option << "\". Expected values are: Modern, Aero, Mac, Classic."
          << std::endl);
    }
  }

  // WizardDefaultWidth
  if (const char* option =
        this->GetOption("CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH")) {
    this->WizardDefaultWidth = option;
  }

  // WizardDefaultHeight
  if (const char* option =
        this->GetOption("CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT")) {
    this->WizardDefaultHeight = option;
  }

  // TitleColor
  if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_TITLE_COLOR")) {
    this->TitleColor = option;
  }

  // Start menu
  if (const char* optIFW_START_MENU_DIR =
        this->GetOption("CPACK_IFW_PACKAGE_START_MENU_DIRECTORY")) {
    this->StartMenuDir = optIFW_START_MENU_DIR;
  } else {
    this->StartMenuDir = Name;
  }

  // Default target directory for installation
  if (const char* optIFW_TARGET_DIRECTORY =
        this->GetOption("CPACK_IFW_TARGET_DIRECTORY")) {
    this->TargetDir = optIFW_TARGET_DIRECTORY;
  } else if (const char* optPACKAGE_INSTALL_DIRECTORY =
               this->GetOption("CPACK_PACKAGE_INSTALL_DIRECTORY")) {
    this->TargetDir = "@ApplicationsDir@/";
    this->TargetDir += optPACKAGE_INSTALL_DIRECTORY;
  } else {
    this->TargetDir = "@RootDir@/usr/local";
  }

  // Default target directory for installation with administrator rights
  if (const char* option =
        this->GetOption("CPACK_IFW_ADMIN_TARGET_DIRECTORY")) {
    this->AdminTargetDir = option;
  }

  // Maintenance tool
  if (const char* optIFW_MAINTENANCE_TOOL =
        this->GetOption("CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME")) {
    this->MaintenanceToolName = optIFW_MAINTENANCE_TOOL;
  }

  // Maintenance tool ini file
  if (const char* optIFW_MAINTENANCE_TOOL_INI =
        this->GetOption("CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE")) {
    this->MaintenanceToolIniFile = optIFW_MAINTENANCE_TOOL_INI;
  }

  // Allow non-ASCII characters
  if (this->GetOption("CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS")) {
    if (this->IsOn("CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS")) {
      this->AllowNonAsciiCharacters = "true";
    } else {
      this->AllowNonAsciiCharacters = "false";
    }
  }

  // Space in path
  if (this->GetOption("CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH")) {
    if (this->IsOn("CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH")) {
      this->AllowSpaceInPath = "true";
    } else {
      this->AllowSpaceInPath = "false";
    }
  }

  // Control script
  if (const char* optIFW_CONTROL_SCRIPT =
        this->GetOption("CPACK_IFW_PACKAGE_CONTROL_SCRIPT")) {
    this->ControlScript = optIFW_CONTROL_SCRIPT;
  }

  // Resources
  if (const char* optIFW_PACKAGE_RESOURCES =
        this->GetOption("CPACK_IFW_PACKAGE_RESOURCES")) {
    this->Resources.clear();
    cmSystemTools::ExpandListArgument(optIFW_PACKAGE_RESOURCES,
                                      this->Resources);
  }
}

/** \class cmCPackIFWResourcesParser
 * \brief Helper class that parse resources form .qrc (Qt)
 */
class cmCPackIFWResourcesParser : public cmXMLParser
{
public:
  cmCPackIFWResourcesParser(cmCPackIFWInstaller* i)
    : installer(i)
    , file(false)
  {
    this->path = i->Directory + "/resources";
  }

  bool ParseResource(size_t r)
  {
    this->hasFiles = false;
    this->hasErrors = false;

    this->basePath =
      cmSystemTools::GetFilenamePath(this->installer->Resources[r]);

    this->ParseFile(this->installer->Resources[r].data());

    return this->hasFiles && !this->hasErrors;
  }

  cmCPackIFWInstaller* installer;
  bool file, hasFiles, hasErrors;
  std::string path, basePath;

protected:
  void StartElement(const std::string& name, const char** /*atts*/) CM_OVERRIDE
  {
    this->file = name == "file";
    if (file) {
      this->hasFiles = true;
    }
  }

  void CharacterDataHandler(const char* data, int length) CM_OVERRIDE
  {
    if (this->file) {
      std::string content(data, data + length);
      content = cmSystemTools::TrimWhitespace(content);
      std::string source = this->basePath + "/" + content;
      std::string destination = this->path + "/" + content;
      if (!cmSystemTools::CopyFileIfDifferent(source.data(),
                                              destination.data())) {
        this->hasErrors = true;
      }
    }
  }

  void EndElement(const std::string& /*name*/) CM_OVERRIDE {}
};

void cmCPackIFWInstaller::GenerateInstallerFile()
{
  // Lazy directory initialization
  if (this->Directory.empty() && this->Generator) {
    this->Directory = this->Generator->toplevel;
  }

  // Output stream
  cmGeneratedFileStream fout((this->Directory + "/config/config.xml").data());
  cmXMLWriter xout(fout);

  xout.StartDocument();

  WriteGeneratedByToStrim(xout);

  xout.StartElement("Installer");

  xout.Element("Name", this->Name);
  xout.Element("Version", this->Version);
  xout.Element("Title", this->Title);

  if (!this->Publisher.empty()) {
    xout.Element("Publisher", this->Publisher);
  }

  if (!this->ProductUrl.empty()) {
    xout.Element("ProductUrl", this->ProductUrl);
  }

  // ApplicationIcon
  if (!this->InstallerApplicationIcon.empty()) {
    std::string name =
      cmSystemTools::GetFilenameName(this->InstallerApplicationIcon);
    std::string path = this->Directory + "/config/" + name;
    name = cmSystemTools::GetFilenameWithoutExtension(name);
    cmsys::SystemTools::CopyFileIfDifferent(this->InstallerApplicationIcon,
                                            path);
    xout.Element("InstallerApplicationIcon", name);
  }

  // WindowIcon
  if (!this->InstallerWindowIcon.empty()) {
    std::string name =
      cmSystemTools::GetFilenameName(this->InstallerWindowIcon);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->InstallerWindowIcon, path);
    xout.Element("InstallerWindowIcon", name);
  }

  // Logo
  if (!this->Logo.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->Logo);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->Logo, path);
    xout.Element("Logo", name);
  }

  // Banner
  if (!this->Banner.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->Banner);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->Banner, path);
    xout.Element("Banner", name);
  }

  // Watermark
  if (!this->Watermark.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->Watermark);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->Watermark, path);
    xout.Element("Watermark", name);
  }

  // Background
  if (!this->Background.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->Background);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->Background, path);
    xout.Element("Background", name);
  }

  // WizardStyle
  if (!this->WizardStyle.empty()) {
    xout.Element("WizardStyle", this->WizardStyle);
  }

  // WizardDefaultWidth
  if (!this->WizardDefaultWidth.empty()) {
    xout.Element("WizardDefaultWidth", this->WizardDefaultWidth);
  }

  // WizardDefaultHeight
  if (!this->WizardDefaultHeight.empty()) {
    xout.Element("WizardDefaultHeight", this->WizardDefaultHeight);
  }

  // TitleColor
  if (!this->TitleColor.empty()) {
    xout.Element("TitleColor", this->TitleColor);
  }

  // Start menu
  if (!this->IsVersionLess("2.0")) {
    xout.Element("StartMenuDir", this->StartMenuDir);
  }

  // Target dir
  if (!this->TargetDir.empty()) {
    xout.Element("TargetDir", this->TargetDir);
  }

  // Admin target dir
  if (!this->AdminTargetDir.empty()) {
    xout.Element("AdminTargetDir", this->AdminTargetDir);
  }

  // Remote repositories
  if (!this->RemoteRepositories.empty()) {
    xout.StartElement("RemoteRepositories");
    for (RepositoriesVector::iterator rit = this->RemoteRepositories.begin();
         rit != this->RemoteRepositories.end(); ++rit) {
      (*rit)->WriteRepositoryConfig(xout);
    }
    xout.EndElement();
  }

  // Maintenance tool
  if (!this->IsVersionLess("2.0") && !this->MaintenanceToolName.empty()) {
    xout.Element("MaintenanceToolName", this->MaintenanceToolName);
  }

  // Maintenance tool ini file
  if (!this->IsVersionLess("2.0") && !this->MaintenanceToolIniFile.empty()) {
    xout.Element("MaintenanceToolIniFile", this->MaintenanceToolIniFile);
  }

  // Different allows
  if (this->IsVersionLess("2.0")) {
    // CPack IFW default policy
    xout.Comment("CPack IFW default policy for QtIFW less 2.0");
    xout.Element("AllowNonAsciiCharacters", "true");
    xout.Element("AllowSpaceInPath", "true");
  } else {
    if (!this->AllowNonAsciiCharacters.empty()) {
      xout.Element("AllowNonAsciiCharacters", this->AllowNonAsciiCharacters);
    }
    if (!this->AllowSpaceInPath.empty()) {
      xout.Element("AllowSpaceInPath", this->AllowSpaceInPath);
    }
  }

  // Control script (copy to config dir)
  if (!this->IsVersionLess("2.0") && !this->ControlScript.empty()) {
    std::string name = cmSystemTools::GetFilenameName(this->ControlScript);
    std::string path = this->Directory + "/config/" + name;
    cmsys::SystemTools::CopyFileIfDifferent(this->ControlScript, path);
    xout.Element("ControlScript", name);
  }

  // Resources (copy to resources dir)
  if (!this->Resources.empty()) {
    std::vector<std::string> resources;
    cmCPackIFWResourcesParser parser(this);
    for (size_t i = 0; i < this->Resources.size(); i++) {
      if (parser.ParseResource(i)) {
        std::string name = cmSystemTools::GetFilenameName(this->Resources[i]);
        std::string path = this->Directory + "/resources/" + name;
        cmsys::SystemTools::CopyFileIfDifferent(this->Resources[i], path);
        resources.push_back(name);
      } else {
        cmCPackIFWLogger(WARNING, "Can't copy resources from \""
                           << this->Resources[i]
                           << "\". Resource will be skipped." << std::endl);
      }
    }
    this->Resources = resources;
  }

  xout.EndElement();
  xout.EndDocument();
}

void cmCPackIFWInstaller::GeneratePackageFiles()
{
  if (this->Packages.empty() || this->Generator->IsOnePackage()) {
    // Generate default package
    cmCPackIFWPackage package;
    package.Generator = this->Generator;
    package.Installer = this;
    // Check package group
    if (const char* option = this->GetOption("CPACK_IFW_PACKAGE_GROUP")) {
      package.ConfigureFromGroup(option);
      std::string forcedOption = "CPACK_IFW_COMPONENT_GROUP_" +
        cmsys::SystemTools::UpperCase(option) + "_FORCED_INSTALLATION";
      if (!GetOption(forcedOption)) {
        package.ForcedInstallation = "true";
      }
    } else {
      package.ConfigureFromOptions();
    }
    package.GeneratePackageFile();
    return;
  }

  // Generate packages meta information
  for (PackagesMap::iterator pit = this->Packages.begin();
       pit != this->Packages.end(); ++pit) {
    cmCPackIFWPackage* package = pit->second;
    package->GeneratePackageFile();
  }
}
