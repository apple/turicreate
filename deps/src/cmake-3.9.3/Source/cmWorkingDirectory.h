/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWorkingDirectory_h
#define cmWorkingDirectory_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

/** \class cmWorkingDirectory
 * \brief An RAII class to manipulate the working directory.
 */
class cmWorkingDirectory
{
public:
  cmWorkingDirectory(std::string const& newdir);
  ~cmWorkingDirectory();

  void Pop();

private:
  std::string OldDir;
};

#endif
