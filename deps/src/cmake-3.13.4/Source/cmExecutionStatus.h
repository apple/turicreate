/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmExecutionStatus_h
#define cmExecutionStatus_h

/** \class cmExecutionStatus
 * \brief Superclass for all command status classes
 *
 * when a command is involked it may set values on a command status instance
 */
class cmExecutionStatus
{
public:
  cmExecutionStatus()
    : ReturnInvoked(false)
    , BreakInvoked(false)
    , ContinueInvoked(false)
    , NestedError(false)
  {
  }

  void Clear()
  {
    this->ReturnInvoked = false;
    this->BreakInvoked = false;
    this->ContinueInvoked = false;
    this->NestedError = false;
  }

  void SetReturnInvoked() { this->ReturnInvoked = true; }
  bool GetReturnInvoked() const { return this->ReturnInvoked; }

  void SetBreakInvoked() { this->BreakInvoked = true; }
  bool GetBreakInvoked() const { return this->BreakInvoked; }

  void SetContinueInvoked() { this->ContinueInvoked = true; }
  bool GetContinueInvoked() const { return this->ContinueInvoked; }

  void SetNestedError() { this->NestedError = true; }
  bool GetNestedError() const { return this->NestedError; }

private:
  bool ReturnInvoked;
  bool BreakInvoked;
  bool ContinueInvoked;
  bool NestedError;
};

#endif
