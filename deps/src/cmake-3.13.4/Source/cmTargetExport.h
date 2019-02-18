/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetExport_h
#define cmTargetExport_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

class cmGeneratorTarget;
class cmInstallFilesGenerator;
class cmInstallTargetGenerator;

/** \brief A member of an ExportSet
 *
 * This struct holds pointers to target and all relevant generators.
 */
class cmTargetExport
{
public:
  std::string TargetName;
  cmGeneratorTarget* Target;

  ///@name Generators
  ///@{
  cmInstallTargetGenerator* ArchiveGenerator;
  cmInstallTargetGenerator* RuntimeGenerator;
  cmInstallTargetGenerator* LibraryGenerator;
  cmInstallTargetGenerator* ObjectsGenerator;
  cmInstallTargetGenerator* FrameworkGenerator;
  cmInstallTargetGenerator* BundleGenerator;
  cmInstallFilesGenerator* HeaderGenerator;
  std::string InterfaceIncludeDirectories;
  ///@}
};

#endif
