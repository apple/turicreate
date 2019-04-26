/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackNuGetGenerator_h
#define cmCPackNuGetGenerator_h

#include "cmCPackGenerator.h"

/** \class cmCPackNuGetGenerator
 * \brief A generator for RPM packages
 */
class cmCPackNuGetGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackNuGetGenerator, cmCPackGenerator);

  // NOTE In fact, it is possible to have NuGet not only for Windows...
  // https://docs.microsoft.com/en-us/nuget/install-nuget-client-tools
  static bool CanGenerate() { return true; }

protected:
  bool SupportsComponentInstallation() const override;
  int PackageFiles() override;

  const char* GetOutputExtension() override { return ".nupkg"; }
  bool SupportsAbsoluteDestination() const override { return false; }
  /**
   * The method used to prepare variables when component
   * install is used.
   */
  void SetupGroupComponentVariables(bool ignoreGroup);
  /**
   * Populate \c packageFileNames vector of built packages.
   */
  void AddGeneratedPackageNames();
};

#endif
