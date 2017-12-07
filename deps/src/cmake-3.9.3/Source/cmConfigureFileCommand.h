/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmConfigureFileCommand_h
#define cmConfigureFileCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmNewLineStyle.h"

class cmExecutionStatus;

class cmConfigureFileCommand : public cmCommand
{
public:
  cmCommand* Clone() CM_OVERRIDE { return new cmConfigureFileCommand; }

  /**
   * This is called when the command is first encountered in
   * the input file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  int ConfigureFile();

  cmNewLineStyle NewLineStyle;

  std::string InputFile;
  std::string OutputFile;
  bool CopyOnly;
  bool EscapeQuotes;
  bool AtOnly;
};

#endif
