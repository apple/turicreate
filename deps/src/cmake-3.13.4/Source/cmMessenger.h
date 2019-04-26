/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmMessenger_h
#define cmMessenger_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmListFileCache.h"
#include "cmake.h"

#include <string>

class cmState;

class cmMessenger
{
public:
  cmMessenger(cmState* state);

  void IssueMessage(
    cmake::MessageType t, std::string const& text,
    cmListFileBacktrace const& backtrace = cmListFileBacktrace()) const;

  void DisplayMessage(cmake::MessageType t, std::string const& text,
                      cmListFileBacktrace const& backtrace) const;

  bool GetSuppressDevWarnings() const;
  bool GetSuppressDeprecatedWarnings() const;
  bool GetDevWarningsAsErrors() const;
  bool GetDeprecatedWarningsAsErrors() const;

private:
  bool IsMessageTypeVisible(cmake::MessageType t) const;
  cmake::MessageType ConvertMessageType(cmake::MessageType t) const;

  cmState* State;
};

#endif
