/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmEnableLanguageCommand_h
#define cmEnableLanguageCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmEnableLanguageCommand
 * \brief Specify the name for this build project.
 *
 * cmEnableLanguageCommand is used to specify a name for this build project.
 * It is defined once per set of CMakeList.txt files (including
 * all subdirectories). Currently it just sets the name of the workspace
 * file for Microsoft Visual C++
 */
class cmEnableLanguageCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmEnableLanguageCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
