/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmStringCommand_h
#define cmStringCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <cstddef>
#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

/** \class cmStringCommand
 * \brief Common string operations
 *
 */
class cmStringCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmStringCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

protected:
  bool HandleConfigureCommand(std::vector<std::string> const& args);
  bool HandleAsciiCommand(std::vector<std::string> const& args);
  bool HandleRegexCommand(std::vector<std::string> const& args);
  bool RegexMatch(std::vector<std::string> const& args);
  bool RegexMatchAll(std::vector<std::string> const& args);
  bool RegexReplace(std::vector<std::string> const& args);
  bool HandleHashCommand(std::vector<std::string> const& args);
  bool HandleToUpperLowerCommand(std::vector<std::string> const& args,
                                 bool toUpper);
  bool HandleCompareCommand(std::vector<std::string> const& args);
  bool HandleReplaceCommand(std::vector<std::string> const& args);
  bool HandleLengthCommand(std::vector<std::string> const& args);
  bool HandleSubstringCommand(std::vector<std::string> const& args);
  bool HandleAppendCommand(std::vector<std::string> const& args);
  bool HandlePrependCommand(std::vector<std::string> const& args);
  bool HandleConcatCommand(std::vector<std::string> const& args);
  bool HandleJoinCommand(std::vector<std::string> const& args);
  bool HandleStripCommand(std::vector<std::string> const& args);
  bool HandleRandomCommand(std::vector<std::string> const& args);
  bool HandleFindCommand(std::vector<std::string> const& args);
  bool HandleTimestampCommand(std::vector<std::string> const& args);
  bool HandleMakeCIdentifierCommand(std::vector<std::string> const& args);
  bool HandleGenexStripCommand(std::vector<std::string> const& args);
  bool HandleUuidCommand(std::vector<std::string> const& args);

  bool joinImpl(std::vector<std::string> const& args, std::string const& glue,
                size_t varIdx);
};

#endif
