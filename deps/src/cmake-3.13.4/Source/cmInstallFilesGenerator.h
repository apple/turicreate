/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallFilesGenerator_h
#define cmInstallFilesGenerator_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmInstallGenerator.h"
#include "cmScriptGenerator.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmLocalGenerator;

/** \class cmInstallFilesGenerator
 * \brief Generate file installation rules.
 */
class cmInstallFilesGenerator : public cmInstallGenerator
{
public:
  cmInstallFilesGenerator(std::vector<std::string> const& files,
                          const char* dest, bool programs,
                          const char* file_permissions,
                          std::vector<std::string> const& configurations,
                          const char* component, MessageLevel message,
                          bool exclude_from_all, const char* rename,
                          bool optional = false);
  ~cmInstallFilesGenerator() override;

  void Compute(cmLocalGenerator* lg) override;

  std::string GetDestination(std::string const& config) const;

protected:
  void GenerateScriptActions(std::ostream& os, Indent indent) override;
  void GenerateScriptForConfig(std::ostream& os, const std::string& config,
                               Indent indent) override;
  void AddFilesInstallRule(std::ostream& os, std::string const& config,
                           Indent indent,
                           std::vector<std::string> const& files);

  cmLocalGenerator* LocalGenerator;
  std::vector<std::string> Files;
  std::string FilePermissions;
  std::string Rename;
  bool Programs;
  bool Optional;
};

#endif
