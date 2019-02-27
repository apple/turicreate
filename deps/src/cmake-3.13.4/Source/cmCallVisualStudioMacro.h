/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCallVisualStudioMacro_h
#define cmCallVisualStudioMacro_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

/** \class cmCallVisualStudioMacro
 * \brief Control class for communicating with CMake's Visual Studio macros
 *
 * Find running instances of Visual Studio by full path solution name.
 * Call a Visual Studio IDE macro in any of those instances.
 */
class cmCallVisualStudioMacro
{
public:
  ///! Call the named macro in instances of Visual Studio with the
  ///! given solution file open. Pass "ALL" for slnFile to call the
  ///! macro in each Visual Studio instance.
  static int CallMacro(const std::string& slnFile, const std::string& macro,
                       const std::string& args,
                       const bool logErrorsAsMessages);

  ///! Count the number of running instances of Visual Studio with the
  ///! given solution file open. Pass "ALL" for slnFile to count all
  ///! running Visual Studio instances.
  static int GetNumberOfRunningVisualStudioInstances(
    const std::string& slnFile);

protected:
private:
};

#endif
