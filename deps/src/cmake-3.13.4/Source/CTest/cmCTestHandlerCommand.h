/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestHandlerCommand_h
#define cmCTestHandlerCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestCommand.h"

#include <stddef.h>
#include <string>
#include <vector>

class cmCTestGenericHandler;
class cmExecutionStatus;

/** \class cmCTestHandler
 * \brief Run a ctest script
 *
 * cmCTestHandlerCommand defineds the command to test the project.
 */
class cmCTestHandlerCommand : public cmCTestCommand
{
public:
  cmCTestHandlerCommand();

  /**
   * The name of the command as specified in CMakeList.txt.
   */
  virtual std::string GetName() const = 0;

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

  enum
  {
    ct_NONE,
    ct_RETURN_VALUE,
    ct_CAPTURE_CMAKE_ERROR,
    ct_BUILD,
    ct_SOURCE,
    ct_SUBMIT_INDEX,
    ct_LAST
  };

protected:
  virtual cmCTestGenericHandler* InitializeHandler() = 0;

  virtual void ProcessAdditionalValues(cmCTestGenericHandler* handler);

  // Command argument handling.
  virtual bool CheckArgumentKeyword(std::string const& arg);
  virtual bool CheckArgumentValue(std::string const& arg);
  enum
  {
    ArgumentDoingNone,
    ArgumentDoingError,
    ArgumentDoingKeyword,
    ArgumentDoingLast1
  };
  int ArgumentDoing;
  unsigned int ArgumentIndex;

  bool AppendXML;
  bool Quiet;

  std::string ReturnVariable;
  std::vector<const char*> Arguments;
  std::vector<const char*> Values;
  size_t Last;
};

#define CTEST_COMMAND_APPEND_OPTION_DOCS                                      \
  "The APPEND option marks results for append to those previously "           \
  "submitted to a dashboard server since the last ctest_start.  "             \
  "Append semantics are defined by the dashboard server in use."

#endif
