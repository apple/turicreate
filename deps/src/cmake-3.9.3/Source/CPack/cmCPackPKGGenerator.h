/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackPKGGenerator_h
#define cmCPackPKGGenerator_h

#include "cmConfigure.h"

#include <set>
#include <sstream>
#include <string>

#include "cmCPackComponentGroup.h"
#include "cmCPackGenerator.h"

class cmXMLWriter;

/** \class cmCPackPKGGenerator
 * \brief A generator for pkg files
 *
 */
class cmCPackPKGGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackPKGGenerator, cmCPackGenerator);

  /**
   * Construct generator
   */
  cmCPackPKGGenerator();
  virtual ~cmCPackPKGGenerator();

  bool SupportsComponentInstallation() const CM_OVERRIDE;

protected:
  int InitializeInternal() CM_OVERRIDE;
  const char* GetOutputPostfix() CM_OVERRIDE { return "darwin"; }

  // Copies or creates the resource file with the given name to the
  // package or package staging directory dirName. The variable
  // CPACK_RESOURCE_FILE_${NAME} (where ${NAME} is the uppercased
  // version of name) specifies the input file to use for this file,
  // which will be configured via ConfigureFile.
  bool CopyCreateResourceFile(const std::string& name,
                              const std::string& dirName);
  bool CopyResourcePlistFile(const std::string& name, const char* outName = 0);

  int CopyInstallScript(const std::string& resdir, const std::string& script,
                        const std::string& name);

  // Retrieve the name of package file that will be generated for this
  // component. The name is just the file name with extension, and
  // does not include the subdirectory.
  std::string GetPackageName(const cmCPackComponent& component);

  // Writes a distribution.dist file, which turns a metapackage into a
  // full-fledged distribution. This file is used to describe
  // inter-component dependencies. metapackageFile is the name of the
  // metapackage for the distribution. Only valid for a
  // component-based install.
  void WriteDistributionFile(const char* metapackageFile);

  // Subroutine of WriteDistributionFile that writes out the
  // dependency attributes for inter-component dependencies.
  void AddDependencyAttributes(const cmCPackComponent& component,
                               std::set<const cmCPackComponent*>& visited,
                               std::ostringstream& out);

  // Subroutine of WriteDistributionFile that writes out the
  // reverse dependency attributes for inter-component dependencies.
  void AddReverseDependencyAttributes(
    const cmCPackComponent& component,
    std::set<const cmCPackComponent*>& visited, std::ostringstream& out);

  // Generates XML that encodes the hierarchy of component groups and
  // their components in a form that can be used by distribution
  // metapackages.
  void CreateChoiceOutline(const cmCPackComponentGroup& group,
                           cmXMLWriter& xout);

  /// Create the "choice" XML element to describe a component group
  /// for the installer GUI.
  void CreateChoice(const cmCPackComponentGroup& group, cmXMLWriter& xout);

  /// Create the "choice" XML element to describe a component for the
  /// installer GUI.
  void CreateChoice(const cmCPackComponent& component, cmXMLWriter& xout);

  // The PostFlight component when creating a metapackage
  cmCPackComponent PostFlightComponent;
};

#endif
