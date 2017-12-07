/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportLibraryDependenciesCommand_h
#define cmExportLibraryDependenciesCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmExportLibraryDependenciesCommand : public cmCommand
{
public:
  cmCommand* Clone() CM_OVERRIDE
  {
    return new cmExportLibraryDependenciesCommand;
  }
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  void FinalPass() CM_OVERRIDE;
  bool HasFinalPass() const CM_OVERRIDE { return true; }

private:
  std::string Filename;
  bool Append;
  void ConstFinalPass() const;
};

#endif
