/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackProductBuildGenerator_h
#define cmCPackProductBuildGenerator_h

#include "cmConfigure.h"

#include <string>

#include "cmCPackGenerator.h"
#include "cmCPackPKGGenerator.h"

class cmCPackComponent;

/** \class cmCPackProductBuildGenerator
 * \brief A generator for ProductBuild files
 *
 */
class cmCPackProductBuildGenerator : public cmCPackPKGGenerator
{
public:
  cmCPackTypeMacro(cmCPackProductBuildGenerator, cmCPackPKGGenerator);

  /**
   * Construct generator
   */
  cmCPackProductBuildGenerator();
  virtual ~cmCPackProductBuildGenerator();

protected:
  int InitializeInternal() CM_OVERRIDE;
  int PackageFiles() CM_OVERRIDE;
  const char* GetOutputExtension() CM_OVERRIDE { return ".pkg"; }

  // Run ProductBuild with the given command line, which will (if
  // successful) produce the given package file. Returns true if
  // ProductBuild succeeds, false otherwise.
  bool RunProductBuild(const std::string& command);

  // Generate a package in the file packageFile for the given
  // component.  All of the files within this component are stored in
  // the directory packageDir. Returns true if successful, false
  // otherwise.
  bool GenerateComponentPackage(const std::string& packageFileDir,
                                const std::string& packageFileName,
                                const std::string& packageDir,
                                const cmCPackComponent* component);

  const char* GetComponentScript(const char* script,
                                 const char* script_component);
};

#endif
