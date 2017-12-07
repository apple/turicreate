/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFindBase_h
#define cmFindBase_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmFindCommon.h"

/** \class cmFindBase
 * \brief Base class for most FIND_XXX commands.
 *
 * cmFindBase is a parent class for cmFindProgramCommand, cmFindPathCommand,
 * and cmFindLibraryCommand, cmFindFileCommand
 */
class cmFindBase : public cmFindCommon
{
public:
  cmFindBase();
  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  virtual bool ParseArguments(std::vector<std::string> const& args);

protected:
  void PrintFindStuff();
  void ExpandPaths();

  // see if the VariableName is already set in the cache,
  // also copy the documentation from the cache to VariableDocumentation
  // if it has documentation in the cache
  bool CheckForVariableInCache();

  // use by command during find
  std::string VariableDocumentation;
  std::string VariableName;
  std::vector<std::string> Names;
  bool NamesPerDir;
  bool NamesPerDirAllowed;

  // CMAKE_*_PATH CMAKE_SYSTEM_*_PATH FRAMEWORK|LIBRARY|INCLUDE|PROGRAM
  std::string EnvironmentPath; // LIB,INCLUDE

  bool AlreadyInCache;
  bool AlreadyInCacheWithoutMetaInfo;

private:
  // Add pieces of the search.
  void FillPackageRootPath();
  void FillCMakeVariablePath();
  void FillCMakeEnvironmentPath();
  void FillUserHintsPath();
  void FillSystemEnvironmentPath();
  void FillCMakeSystemVariablePath();
  void FillUserGuessPath();
};

#endif
