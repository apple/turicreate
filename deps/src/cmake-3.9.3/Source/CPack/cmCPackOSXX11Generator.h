/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackOSXX11Generator_h
#define cmCPackOSXX11Generator_h

#include "cmConfigure.h"

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
  virtual ~cmCPackOSXX11Generator();

protected:
  virtual int InitializeInternal() CM_OVERRIDE;
  int PackageFiles() CM_OVERRIDE;
  const char* GetPackagingInstallPrefix() CM_OVERRIDE;
  const char* GetOutputExtension() CM_OVERRIDE { return ".dmg"; }

  // bool CopyCreateResourceFile(const std::string& name,
  //                            const std::string& dir);
  bool CopyResourcePlistFile(const std::string& name, const std::string& dir,
                             const char* outputFileName = 0,
                             bool copyOnly = false);
  std::string InstallPrefix;
};

#endif
