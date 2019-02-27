/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCommand_h
#define cmCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

class cmExecutionStatus;
class cmMakefile;
struct cmListFileArgument;

/** \class cmCommand
 * \brief Superclass for all commands in CMake.
 *
 * cmCommand is the base class for all commands in CMake. A command
 * manifests as an entry in CMakeLists.txt and produces one or
 * more makefile rules. Commands are associated with a particular
 * makefile. This base class cmCommand defines the API for commands
 * to support such features as enable/disable, inheritance,
 * documentation, and construction.
 */
class cmCommand
{
  CM_DISABLE_COPY(cmCommand)

public:
  /**
   * Construct the command. By default it has no makefile.
   */
  cmCommand()
    : Makefile(nullptr)
  {
  }

  /**
   * Need virtual destructor to destroy real command type.
   */
  virtual ~cmCommand() {}

  /**
   * Specify the makefile.
   */
  void SetMakefile(cmMakefile* m) { this->Makefile = m; }
  cmMakefile* GetMakefile() { return this->Makefile; }

  /**
   * This is called by the cmMakefile when the command is first
   * encountered in the CMakeLists.txt file.  It expands the command's
   * arguments and then invokes the InitialPass.
   */
  virtual bool InvokeInitialPass(const std::vector<cmListFileArgument>& args,
                                 cmExecutionStatus& status);

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  virtual bool InitialPass(std::vector<std::string> const& args,
                           cmExecutionStatus&) = 0;

  /**
   * This is called at the end after all the information
   * specified by the command is accumulated. Most commands do
   * not implement this method.  At this point, reading and
   * writing to the cache can be done.
   */
  virtual void FinalPass() {}

  /**
   * Does this command have a final pass?  Query after InitialPass.
   */
  virtual bool HasFinalPass() const { return false; }

  /**
   * This is a virtual constructor for the command.
   */
  virtual cmCommand* Clone() = 0;

  /**
   * Return the last error string.
   */
  const char* GetError();

  /**
   * Set the error message
   */
  void SetError(const std::string& e);

protected:
  cmMakefile* Makefile;

private:
  std::string Error;
};

#endif
