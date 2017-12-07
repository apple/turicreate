/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVariableWatchCommand_h
#define cmVariableWatchCommand_h

#include "cmConfigure.h"

#include <set>
#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmVariableWatchCommand
 * \brief Watch when the variable changes and invoke command
 *
 */
class cmVariableWatchCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmVariableWatchCommand; }

  //! Default constructor
  cmVariableWatchCommand();

  //! Destructor.
  ~cmVariableWatchCommand() CM_OVERRIDE;

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  /** This command does not really have a final pass but it needs to
      stay alive since it owns variable watch callback information. */
  bool HasFinalPass() const CM_OVERRIDE { return true; }

protected:
  std::set<std::string> WatchedVariables;
};

#endif
