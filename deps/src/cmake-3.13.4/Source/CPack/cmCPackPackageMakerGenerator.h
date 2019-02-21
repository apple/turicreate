/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackPackageMakerGenerator_h
#define cmCPackPackageMakerGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackGenerator.h"
#include "cmCPackPKGGenerator.h"

class cmCPackComponent;

/** \class cmCPackPackageMakerGenerator
 * \brief A generator for PackageMaker files
 *
 * http://developer.apple.com/documentation/Darwin
 * /Reference/ManPages/man1/packagemaker.1.html
 */
class cmCPackPackageMakerGenerator : public cmCPackPKGGenerator
{
public:
  cmCPackTypeMacro(cmCPackPackageMakerGenerator, cmCPackPKGGenerator);

  /**
   * Construct generator
   */
  cmCPackPackageMakerGenerator();
  ~cmCPackPackageMakerGenerator() override;
  bool SupportsComponentInstallation() const override;

protected:
  int InitializeInternal() override;
  int PackageFiles() override;
  const char* GetOutputExtension() override { return ".dmg"; }

  // Run PackageMaker with the given command line, which will (if
  // successful) produce the given package file. Returns true if
  // PackageMaker succeeds, false otherwise.
  bool RunPackageMaker(const char* command, const char* packageFile);

  // Generate a package in the file packageFile for the given
  // component.  All of the files within this component are stored in
  // the directory packageDir. Returns true if successful, false
  // otherwise.
  bool GenerateComponentPackage(const char* packageFile,
                                const char* packageDir,
                                const cmCPackComponent& component);

  double PackageMakerVersion;
  unsigned int PackageCompatibilityVersion;
};

#endif
