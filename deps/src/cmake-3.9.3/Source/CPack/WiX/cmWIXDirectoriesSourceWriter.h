/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXDirectoriesSourceWriter_h
#define cmWIXDirectoriesSourceWriter_h

#include "cmWIXSourceWriter.h"

#include "cmCPackGenerator.h"

#include <string>

/** \class cmWIXDirectoriesSourceWriter
 * \brief Helper class to generate directories.wxs
 */
class cmWIXDirectoriesSourceWriter : public cmWIXSourceWriter
{
public:
  cmWIXDirectoriesSourceWriter(cmCPackLog* logger, std::string const& filename,
                               GuidType componentGuidType);

  void EmitStartMenuFolder(std::string const& startMenuFolder);

  void EmitDesktopFolder();

  void EmitStartupFolder();

  size_t BeginInstallationPrefixDirectory(
    std::string const& programFilesFolderId,
    std::string const& installRootString);

  void EndInstallationPrefixDirectory(size_t size);
};

#endif
