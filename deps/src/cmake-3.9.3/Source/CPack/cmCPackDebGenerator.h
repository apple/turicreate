/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackDebGenerator_h
#define cmCPackDebGenerator_h

#include "cmConfigure.h"

#include "cmCPackGenerator.h"

#include <string>
#include <vector>

/** \class cmCPackDebGenerator
 * \brief A generator for Debian packages
 *
 */
class cmCPackDebGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackDebGenerator, cmCPackGenerator);

  /**
   * Construct generator
   */
  cmCPackDebGenerator();
  ~cmCPackDebGenerator() CM_OVERRIDE;

  static bool CanGenerate()
  {
#ifdef __APPLE__
    // on MacOS enable CPackDeb iff dpkg is found
    std::vector<std::string> locations;
    locations.push_back("/sw/bin");        // Fink
    locations.push_back("/opt/local/bin"); // MacPorts
    return cmSystemTools::FindProgram("dpkg", locations) != "" ? true : false;
#else
    // legacy behavior on other systems
    return true;
#endif
  }

protected:
  int InitializeInternal() CM_OVERRIDE;
  /**
   * This method factors out the work done in component packaging case.
   */
  int PackageOnePack(std::string const& initialToplevel,
                     std::string const& packageName);
  /**
   * The method used to package files when component
   * install is used. This will create one
   * archive for each component group.
   */
  int PackageComponents(bool ignoreGroup);
  /**
   * Special case of component install where all
   * components will be put in a single installer.
   */
  int PackageComponentsAllInOne(const std::string& compInstDirName);
  int PackageFiles() CM_OVERRIDE;
  const char* GetOutputExtension() CM_OVERRIDE { return ".deb"; }
  bool SupportsComponentInstallation() const CM_OVERRIDE;
  std::string GetComponentInstallDirNameSuffix(
    const std::string& componentName) CM_OVERRIDE;

private:
  int createDeb();
  std::vector<std::string> packageFiles;
};

#endif
