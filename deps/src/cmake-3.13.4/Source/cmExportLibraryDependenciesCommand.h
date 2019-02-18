/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExportLibraryDependenciesCommand_h
#define cmExportLibraryDependenciesCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmExportLibraryDependenciesCommand : public cmCommand
{
public:
  cmCommand* Clone() override
  {
    return new cmExportLibraryDependenciesCommand;
  }
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

  void FinalPass() override;
  bool HasFinalPass() const override { return true; }

private:
  std::string Filename;
  bool Append = false;
  void ConstFinalPass() const;
};

#endif
