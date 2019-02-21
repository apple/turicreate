/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTargetLinkLibrariesCommand_h
#define cmTargetLinkLibrariesCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

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
 *
 * Additionally, it allows to propagate usage-requirements (including link
 * libraries) from one target into another.
 */
class cmTargetLinkLibrariesCommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() override { return new cmTargetLinkLibrariesCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

private:
  void LinkLibraryTypeSpecifierWarning(int left, int right);
  static const char* LinkLibraryTypeNames[3];

  cmTarget* Target = nullptr;
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

  ProcessingState CurrentProcessingState = ProcessingLinkLibraries;

  bool HandleLibrary(const std::string& lib, cmTargetLinkLibraryType llt);
};

#endif
