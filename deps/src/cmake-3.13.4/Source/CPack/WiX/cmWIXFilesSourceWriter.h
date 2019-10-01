/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXFilesSourceWriter_h
#define cmWIXFilesSourceWriter_h

#include "cmWIXSourceWriter.h"

#include "cmWIXPatch.h"
#include "cmWIXShortcut.h"

#include "cmCPackGenerator.h"

/** \class cmWIXFilesSourceWriter
 * \brief Helper class to generate files.wxs
 */
class cmWIXFilesSourceWriter : public cmWIXSourceWriter
{
public:
  cmWIXFilesSourceWriter(cmCPackLog* logger, std::string const& filename,
                         GuidType componentGuidType);

  void EmitShortcut(std::string const& id, cmWIXShortcut const& shortcut,
                    std::string const& shortcutPrefix, size_t shortcutIndex);

  void EmitRemoveFolder(std::string const& id);

  void EmitInstallRegistryValue(std::string const& registryKey,
                                std::string const& cpackComponentName,
                                std::string const& suffix);

  void EmitUninstallShortcut(std::string const& packageName);

  std::string EmitComponentCreateFolder(std::string const& directoryId,
                                        std::string const& guid,
                                        cmInstalledFile const* installedFile);

  std::string EmitComponentFile(std::string const& directoryId,
                                std::string const& id,
                                std::string const& filePath, cmWIXPatch& patch,
                                cmInstalledFile const* installedFile);
};

#endif
