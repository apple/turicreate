/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallDirectoryGenerator_h
#define cmInstallDirectoryGenerator_h

#include "cmInstallGenerator.h"
#include "cmScriptGenerator.h"

#include "cmConfigure.h"

#include <iosfwd>
#include <string>
#include <vector>

class cmLocalGenerator;

/** \class cmInstallDirectoryGenerator
 * \brief Generate directory installation rules.
 */
class cmInstallDirectoryGenerator : public cmInstallGenerator
{
public:
  cmInstallDirectoryGenerator(std::vector<std::string> const& dirs,
                              const char* dest, const char* file_permissions,
                              const char* dir_permissions,
                              std::vector<std::string> const& configurations,
                              const char* component, MessageLevel message,
                              bool exclude_from_all, const char* literal_args,
                              bool optional = false);
  ~cmInstallDirectoryGenerator() CM_OVERRIDE;

  void Compute(cmLocalGenerator* lg) CM_OVERRIDE;

  std::string GetDestination(std::string const& config) const;

protected:
  void GenerateScriptActions(std::ostream& os, Indent indent) CM_OVERRIDE;
  void GenerateScriptForConfig(std::ostream& os, const std::string& config,
                               Indent indent) CM_OVERRIDE;
  void AddDirectoryInstallRule(std::ostream& os, const std::string& config,
                               Indent indent,
                               std::vector<std::string> const& dirs);
  cmLocalGenerator* LocalGenerator;
  std::vector<std::string> Directories;
  std::string FilePermissions;
  std::string DirPermissions;
  std::string LiteralArguments;
  bool Optional;
};

#endif
