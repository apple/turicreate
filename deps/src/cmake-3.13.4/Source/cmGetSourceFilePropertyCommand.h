/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGetSourceFilePropertyCommand_h
#define cmGetSourceFilePropertyCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmGetSourceFilePropertyCommand : public cmCommand
{
public:
  cmCommand* Clone() override { return new cmGetSourceFilePropertyCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
