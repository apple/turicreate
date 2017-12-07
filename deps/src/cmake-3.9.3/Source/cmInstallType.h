/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmInstallType_h
#define cmInstallType_h

/**
 * Enumerate types known to file(INSTALL).
 */
enum cmInstallType
{
  cmInstallType_EXECUTABLE,
  cmInstallType_STATIC_LIBRARY,
  cmInstallType_SHARED_LIBRARY,
  cmInstallType_MODULE_LIBRARY,
  cmInstallType_FILES,
  cmInstallType_PROGRAMS,
  cmInstallType_DIRECTORY
};

#endif
