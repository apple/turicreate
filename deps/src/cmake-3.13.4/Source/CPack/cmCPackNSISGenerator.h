/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackNSISGenerator_h
#define cmCPackNSISGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCPackGenerator.h"

#include <iosfwd>
#include <set>
#include <string>
#include <vector>

class cmCPackComponent;
class cmCPackComponentGroup;

/** \class cmCPackNSISGenerator
 * \brief A generator for NSIS files
 *
 * http://people.freebsd.org/~kientzle/libarchive/
 */
class cmCPackNSISGenerator : public cmCPackGenerator
{
public:
  cmCPackTypeMacro(cmCPackNSISGenerator, cmCPackGenerator);

  static cmCPackGenerator* CreateGenerator64()
  {
    return new cmCPackNSISGenerator(true);
  }

  /**
   * Construct generator
   */
  cmCPackNSISGenerator(bool nsis64 = false);
  ~cmCPackNSISGenerator() override;

protected:
  int InitializeInternal() override;
  void CreateMenuLinks(std::ostream& str, std::ostream& deleteStr);
  int PackageFiles() override;
  const char* GetOutputExtension() override { return ".exe"; }
  const char* GetOutputPostfix() override { return "win32"; }

  bool GetListOfSubdirectories(const char* dir,
                               std::vector<std::string>& dirs);

  enum cmCPackGenerator::CPackSetDestdirSupport SupportsSetDestdir()
    const override;
  bool SupportsAbsoluteDestination() const override;
  bool SupportsComponentInstallation() const override;

  /// Produce a string that contains the NSIS code to describe a
  /// particular component. Any added macros will be emitted via
  /// macrosOut.
  std::string CreateComponentDescription(cmCPackComponent* component,
                                         std::ostream& macrosOut);

  /// Produce NSIS code that selects all of the components that this component
  /// depends on, recursively.
  std::string CreateSelectionDependenciesDescription(
    cmCPackComponent* component, std::set<cmCPackComponent*>& visited);

  /// Produce NSIS code that de-selects all of the components that are
  /// dependent on this component, recursively.
  std::string CreateDeselectionDependenciesDescription(
    cmCPackComponent* component, std::set<cmCPackComponent*>& visited);

  /// Produce a string that contains the NSIS code to describe a
  /// particular component group, including its components. Any
  /// added macros will be emitted via macrosOut.
  std::string CreateComponentGroupDescription(cmCPackComponentGroup* group,
                                              std::ostream& macrosOut);

  /// Returns the custom install directory if available for the specified
  /// component, otherwise $INSTDIR is returned.
  std::string CustomComponentInstallDirectory(
    const std::string& componentName);

  /// Translations any newlines found in the string into \\r\\n, so that the
  /// resulting string can be used within NSIS.
  static std::string TranslateNewlines(std::string str);

  bool Nsis64;
};

#endif
