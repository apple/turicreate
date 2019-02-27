/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallGenerator_h
#define cmInstallGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmInstallType.h"
#include "cmScriptGenerator.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmLocalGenerator;
class cmMakefile;

/** \class cmInstallGenerator
 * \brief Support class for generating install scripts.
 *
 */
class cmInstallGenerator : public cmScriptGenerator
{
  CM_DISABLE_COPY(cmInstallGenerator)

public:
  enum MessageLevel
  {
    MessageDefault,
    MessageAlways,
    MessageLazy,
    MessageNever
  };

  cmInstallGenerator(const char* destination,
                     std::vector<std::string> const& configurations,
                     const char* component, MessageLevel message,
                     bool exclude_from_all);
  ~cmInstallGenerator() override;

  void AddInstallRule(
    std::ostream& os, std::string const& dest, cmInstallType type,
    std::vector<std::string> const& files, bool optional = false,
    const char* permissions_file = nullptr,
    const char* permissions_dir = nullptr, const char* rename = nullptr,
    const char* literal_args = nullptr, Indent indent = Indent());

  /** Get the install destination as it should appear in the
      installation script.  */
  std::string ConvertToAbsoluteDestination(std::string const& dest) const;

  /** Test if this generator installs something for a given configuration.  */
  bool InstallsForConfig(const std::string& config);

  /** Select message level from CMAKE_INSTALL_MESSAGE or 'never'.  */
  static MessageLevel SelectMessageLevel(cmMakefile* mf, bool never = false);

  virtual void Compute(cmLocalGenerator*) {}

protected:
  void GenerateScript(std::ostream& os) override;

  std::string CreateComponentTest(const char* component,
                                  bool exclude_from_all);

  // Information shared by most generator types.
  std::string Destination;
  std::string Component;
  MessageLevel Message;
  bool ExcludeFromAll;
};

#endif
