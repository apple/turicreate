/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQTWrapUICommand_h
#define cmQTWrapUICommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmQTWrapUICommand
 * \brief Create .h and .cxx files rules for Qt user interfaces files
 *
 * cmQTWrapUICommand is used to create wrappers for Qt classes into normal C++
 */
class cmQTWrapUICommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmQTWrapUICommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;
};

#endif
