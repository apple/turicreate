/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackIFWInstaller_h
#define cmCPackIFWInstaller_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackIFWCommon.h"

#include <map>
#include <string>
#include <vector>

class cmCPackIFWPackage;
class cmCPackIFWRepository;

/** \class cmCPackIFWInstaller
 * \brief A binary installer to be created CPack IFW generator
 */
class cmCPackIFWInstaller : public cmCPackIFWCommon
{
public:
  // Types

  typedef std::map<std::string, cmCPackIFWPackage*> PackagesMap;
  typedef std::vector<cmCPackIFWRepository*> RepositoriesVector;

public:
  // Constructor

  /**
   * Construct installer
   */
  cmCPackIFWInstaller();

public:
  // Configuration

  /// Name of the product being installed
  std::string Name;

  /// Version number of the product being installed
  std::string Version;

  /// Name of the installer as displayed on the title bar
  std::string Title;

  /// Publisher of the software (as shown in the Windows Control Panel)
  std::string Publisher;

  /// URL to a page that contains product information on your web site
  std::string ProductUrl;

  /// Filename for a custom installer icon
  std::string InstallerApplicationIcon;

  /// Filename for a custom window icon
  std::string InstallerWindowIcon;

  /// Filename for a logo
  std::string Logo;

  /// Filename for a watermark
  std::string Watermark;

  /// Filename for a banner
  std::string Banner;

  /// Filename for a background
  std::string Background;

  /// Wizard style name
  std::string WizardStyle;

  /// Wizard width
  std::string WizardDefaultWidth;

  /// Wizard height
  std::string WizardDefaultHeight;

  /// Title color
  std::string TitleColor;

  /// Name of the default program group in the Windows Start menu
  std::string StartMenuDir;

  /// Default target directory for installation
  std::string TargetDir;

  /// Default target directory for installation with administrator rights
  std::string AdminTargetDir;

  /// Filename of the generated maintenance tool
  std::string MaintenanceToolName;

  /// Filename for the configuration of the generated maintenance tool
  std::string MaintenanceToolIniFile;

  /// Set to true if the installation path can contain non-ASCII characters
  std::string AllowNonAsciiCharacters;

  /// Set to false if the installation path cannot contain space characters
  std::string AllowSpaceInPath;

  /// Filename for a custom installer control script
  std::string ControlScript;

  /// List of resources to include in the installer binary
  std::vector<std::string> Resources;

public:
  // Internal implementation

  void ConfigureFromOptions();

  void GenerateInstallerFile();

  void GeneratePackageFiles();

  PackagesMap Packages;
  RepositoriesVector RemoteRepositories;
  std::string Directory;

protected:
  void printSkippedOptionWarning(const std::string& optionName,
                                 const std::string& optionValue);
};

#endif // cmCPackIFWInstaller_h
