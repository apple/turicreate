/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetLinkLibrariesCommand_h
#define cmTargetLinkLibrariesCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"
#include "cmTargetLinkLibraryType.h"

class cmExecutionStatus;
class cmTarget;

/** \class cmTargetLinkLibrariesCommand
 * \brief Specify a list of libraries to link into executables.
 *
 * cmTargetLinkLibrariesCommand is used to specify a list of libraries to link
 * into executable(s) or shared objects. The names of the libraries
 * should be those defined by the LIBRARY(library) command(s).
 */
class cmTargetLinkLibrariesCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmTargetLinkLibrariesCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

private:
  void LinkLibraryTypeSpecifierWarning(int left, int right);
  static const char* LinkLibraryTypeNames[3];

  cmTarget* Target;
  enum ProcessingState
  {
    ProcessingLinkLibraries,
    ProcessingPlainLinkInterface,
    ProcessingKeywordLinkInterface,
    ProcessingPlainPublicInterface,
    ProcessingKeywordPublicInterface,
    ProcessingPlainPrivateInterface,
    ProcessingKeywordPrivateInterface
  };

  ProcessingState CurrentProcessingState;

  bool HandleLibrary(const std::string& lib, cmTargetLinkLibraryType llt);
};

#endif
