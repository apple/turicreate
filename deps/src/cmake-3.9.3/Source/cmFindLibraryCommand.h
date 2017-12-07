/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindLibraryCommand_h
#define cmFindLibraryCommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmFindBase.h"

class cmCommand;
class cmExecutionStatus;

/** \class cmFindLibraryCommand
 * \brief Define a command to search for a library.
 *
 * cmFindLibraryCommand is used to define a CMake variable
 * that specifies a library. The command searches for a given
 * file in a list of directories.
 */
class cmFindLibraryCommand : public cmFindBase
{
public:
  cmFindLibraryCommand();
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmFindLibraryCommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

protected:
  void AddArchitecturePaths(const char* suffix);
  void AddArchitecturePath(std::string const& dir,
                           std::string::size_type start_pos,
                           const char* suffix, bool fresh = true);
  std::string FindLibrary();

private:
  std::string FindNormalLibrary();
  std::string FindNormalLibraryNamesPerDir();
  std::string FindNormalLibraryDirsPerName();
  std::string FindFrameworkLibrary();
  std::string FindFrameworkLibraryNamesPerDir();
  std::string FindFrameworkLibraryDirsPerName();
};

#endif
