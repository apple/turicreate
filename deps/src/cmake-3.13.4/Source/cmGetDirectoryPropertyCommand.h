/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGetDirectoryPropertyCommand_h
#define cmGetDirectoryPropertyCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmGetDirectoryPropertyCommand : public cmCommand
{
public:
  cmCommand* Clone() override { return new cmGetDirectoryPropertyCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  void StoreResult(const std::string& variable, const char* prop);
};

#endif
