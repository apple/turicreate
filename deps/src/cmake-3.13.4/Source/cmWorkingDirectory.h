/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWorkingDirectory_h
#define cmWorkingDirectory_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>

/** \class cmWorkingDirectory
 * \brief An RAII class to manipulate the working directory.
 *
 * The current working directory is set to the location given to the
 * constructor. The working directory can be changed again as needed
 * by calling SetDirectory(). When the object is destroyed, the destructor
 * will restore the working directory to what it was when the object was
 * created, regardless of any calls to SetDirectory() in the meantime.
 */
class cmWorkingDirectory
{
public:
  cmWorkingDirectory(std::string const& newdir);
  ~cmWorkingDirectory();

  bool SetDirectory(std::string const& newdir);
  void Pop();
  bool Failed() const { return ResultCode != 0; }

  /** \return 0 if the last attempt to set the working directory was
   *          successful. If it failed, the value returned will be the
   *          \c errno value associated with the failure. A description
   *          of the error code can be obtained by passing the result
   *          to \c std::strerror().
   */
  int GetLastResult() const { return ResultCode; }

private:
  std::string OldDir;
  int ResultCode;
};

#endif
