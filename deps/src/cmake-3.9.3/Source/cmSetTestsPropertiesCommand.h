/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmSetTestsPropertiesCommand_h
#define cmSetTestsPropertiesCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;
class cmMakefile;

class cmSetTestsPropertiesCommand : public cmCommand
{
public:
  cmCommand* Clone() CM_OVERRIDE { return new cmSetTestsPropertiesCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  static bool SetOneTest(const std::string& tname,
                         std::vector<std::string>& propertyPairs,
                         cmMakefile* mf, std::string& errors);
};

#endif
