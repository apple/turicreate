/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackOSXX11Generator_h
#define cmCPackOSXX11Generator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

#include "cmCPackGenerator.h"

/** \class cmCPackOSXX11Generator
 * \brief A generator for OSX X11 modules
 *
 * Based on Gimp.app
 */
class cmCPackOSXX11Generator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackOSXX11Generator, cmCPackGenerator);

  /**
   * Construct generator
   */
  cmCPackOSXX11Generator();
  ~cmCPackOSXX11Generator() override;

protected:
  virtual int InitializeInternal() override;
  int PackageFiles() override;
  const char* GetPackagingInstallPrefix() override;
  const char* GetOutputExtension() override { return ".dmg"; }

  // bool CopyCreateResourceFile(const std::string& name,
  //                            const std::string& dir);
  bool CopyResourcePlistFile(const std::string& name, const std::string& dir,
                             const char* outputFileName = 0,
                             bool copyOnly = false);
  std::string InstallPrefix;
};

#endif
