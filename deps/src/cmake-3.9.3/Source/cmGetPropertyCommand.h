/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGetPropertyCommand_h
#define cmGetPropertyCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmGetPropertyCommand : public cmCommand
{
public:
  cmGetPropertyCommand();

  cmCommand* Clone() CM_OVERRIDE { return new cmGetPropertyCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  enum OutType
  {
    OutValue,
    OutDefined,
    OutBriefDoc,
    OutFullDoc,
    OutSet
  };
  std::string Variable;
  std::string Name;
  std::string PropertyName;
  OutType InfoType;

  // Implementation of result storage.
  bool StoreResult(const char* value);

  // Implementation of each property type.
  bool HandleGlobalMode();
  bool HandleDirectoryMode();
  bool HandleTargetMode();
  bool HandleSourceMode();
  bool HandleTestMode();
  bool HandleVariableMode();
  bool HandleCacheMode();
  bool HandleInstallMode();
};

#endif
