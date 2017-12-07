/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallGenerator_h
#define cmInstallGenerator_h

#include "cmConfigure.h"

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
  ~cmInstallGenerator() CM_OVERRIDE;

  void AddInstallRule(
    std::ostream& os, std::string const& dest, cmInstallType type,
    std::vector<std::string> const& files, bool optional = false,
    const char* permissions_file = CM_NULLPTR,
    const char* permissions_dir = CM_NULLPTR, const char* rename = CM_NULLPTR,
    const char* literal_args = CM_NULLPTR, Indent indent = Indent());

  /** Get the install destination as it should appear in the
      installation script.  */
  std::string ConvertToAbsoluteDestination(std::string const& dest) const;

  /** Test if this generator installs something for a given configuration.  */
  bool InstallsForConfig(const std::string& config);

  /** Select message level from CMAKE_INSTALL_MESSAGE or 'never'.  */
  static MessageLevel SelectMessageLevel(cmMakefile* mf, bool never = false);

  virtual void Compute(cmLocalGenerator*) {}

protected:
  void GenerateScript(std::ostream& os) CM_OVERRIDE;

  std::string CreateComponentTest(const char* component,
                                  bool exclude_from_all);

  // Information shared by most generator types.
  std::string Destination;
  std::string Component;
  MessageLevel Message;
  bool ExcludeFromAll;
};

#endif
