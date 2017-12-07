/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSetsPropertiesCommand_h
#define cmSetsPropertiesCommand_h

#include "cmConfigure.h"

#include <set>
#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;
class cmInstalledFile;
class cmSourceFile;
class cmTarget;
class cmTest;

class cmSetPropertyCommand : public cmCommand
{
public:
  cmSetPropertyCommand();

  cmCommand* Clone() CM_OVERRIDE { return new cmSetPropertyCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  std::set<std::string> Names;
  std::string PropertyName;
  std::string PropertyValue;
  bool Remove;
  bool AppendMode;
  bool AppendAsString;

  // Implementation of each property type.
  bool HandleGlobalMode();
  bool HandleDirectoryMode();
  bool HandleTargetMode();
  bool HandleTarget(cmTarget* target);
  bool HandleSourceMode();
  bool HandleSource(cmSourceFile* sf);
  bool HandleTestMode();
  bool HandleTest(cmTest* test);
  bool HandleCacheMode();
  bool HandleCacheEntry(std::string const&);
  bool HandleInstallMode();
  bool HandleInstall(cmInstalledFile* file);
};

#endif
