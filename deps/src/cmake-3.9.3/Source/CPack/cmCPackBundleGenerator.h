/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackBundleGenerator_h
#define cmCPackBundleGenerator_h

#include "cmConfigure.h"

#include <string>

#include "cmCPackDragNDropGenerator.h"
#include "cmCPackGenerator.h"

/** \class cmCPackBundleGenerator
 * \brief A generator for OSX bundles
 *
 * Based on Gimp.app
 */
class cmCPackBundleGenerator : public cmCPackDragNDropGenerator
{
public:
  cmCPackTypeMacro(cmCPackBundleGenerator, cmCPackDragNDropGenerator);

  cmCPackBundleGenerator();
  virtual ~cmCPackBundleGenerator();

protected:
  int InitializeInternal() CM_OVERRIDE;
  const char* GetPackagingInstallPrefix() CM_OVERRIDE;
  int ConstructBundle();
  int SignBundle(const std::string& src_dir);
  int PackageFiles() CM_OVERRIDE;
  bool SupportsComponentInstallation() const CM_OVERRIDE;

  std::string InstallPrefix;
};

#endif
