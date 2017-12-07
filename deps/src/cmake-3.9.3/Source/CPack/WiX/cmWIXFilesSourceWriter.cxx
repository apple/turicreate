/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXFilesSourceWriter.h"

#include "cmWIXAccessControlList.h"

#include "cmInstalledFile.h"

#include "cmSystemTools.h"
#include "cmUuid.h"

#include "cm_sys_stat.h"

cmWIXFilesSourceWriter::cmWIXFilesSourceWriter(cmCPackLog* logger,
                                               std::string const& filename,
                                               GuidType componentGuidType)
  : cmWIXSourceWriter(logger, filename, componentGuidType)
{
}

void cmWIXFilesSourceWriter::EmitShortcut(std::string const& id,
                                          cmWIXShortcut const& shortcut,
                                          std::string const& shortcutPrefix,
                                          size_t shortcutIndex)
{
  std::ostringstream shortcutId;
  shortcutId << shortcutPrefix << id;

  if (shortcutIndex > 0) {
    shortcutId << "_" << shortcutIndex;
  }

  std::string fileId = std::string("CM_F") + id;

  BeginElement("Shortcut");
  AddAttribute("Id", shortcutId.str());
  AddAttribute("Name", shortcut.label);
  std::string target = "[#" + fileId + "]";
  AddAttribute("Target", target);
  AddAttribute("WorkingDirectory", shortcut.workingDirectoryId);
  EndElement("Shortcut");
}

void cmWIXFilesSourceWriter::EmitRemoveFolder(std::string const& id)
{
  BeginElement("RemoveFolder");
  AddAttribute("Id", id);
  AddAttribute("On", "uninstall");
  EndElement("RemoveFolder");
}

void cmWIXFilesSourceWriter::EmitInstallRegistryValue(
  std::string const& registryKey, std::string const& cpackComponentName,
  std::string const& suffix)
{
  std::string valueName;
  if (!cpackComponentName.empty()) {
    valueName = cpackComponentName + "_";
  }

  valueName += "installed";
  valueName += suffix;

  BeginElement("RegistryValue");
  AddAttribute("Root", "HKCU");
  AddAttribute("Key", registryKey);
  AddAttribute("Name", valueName);
  AddAttribute("Type", "integer");
  AddAttribute("Value", "1");
  AddAttribute("KeyPath", "yes");
  EndElement("RegistryValue");
}

void cmWIXFilesSourceWriter::EmitUninstallShortcut(
  std::string const& packageName)
{
  BeginElement("Shortcut");
  AddAttribute("Id", "UNINSTALL");
  AddAttribute("Name", "Uninstall " + packageName);
  AddAttribute("Description", "Uninstalls " + packageName);
  AddAttribute("Target", "[SystemFolder]msiexec.exe");
  AddAttribute("Arguments", "/x [ProductCode]");
  EndElement("Shortcut");
}

std::string cmWIXFilesSourceWriter::EmitComponentCreateFolder(
  std::string const& directoryId, std::string const& guid,
  cmInstalledFile const* installedFile)
{
  std::string componentId = std::string("CM_C_EMPTY_") + directoryId;

  BeginElement("DirectoryRef");
  AddAttribute("Id", directoryId);

  BeginElement("Component");
  AddAttribute("Id", componentId);
  AddAttribute("Guid", guid);

  BeginElement("CreateFolder");

  if (installedFile) {
    cmWIXAccessControlList acl(Logger, *installedFile, *this);
    acl.Apply();
  }

  EndElement("CreateFolder");
  EndElement("Component");
  EndElement("DirectoryRef");

  return componentId;
}

std::string cmWIXFilesSourceWriter::EmitComponentFile(
  std::string const& directoryId, std::string const& id,
  std::string const& filePath, cmWIXPatch& patch,
  cmInstalledFile const* installedFile)
{
  std::string componentId = std::string("CM_C") + id;
  std::string fileId = std::string("CM_F") + id;

  std::string guid = CreateGuidFromComponentId(componentId);

  BeginElement("DirectoryRef");
  AddAttribute("Id", directoryId);

  BeginElement("Component");
  AddAttribute("Id", componentId);
  AddAttribute("Guid", guid);

  if (installedFile) {
    if (installedFile->GetPropertyAsBool("CPACK_NEVER_OVERWRITE")) {
      AddAttribute("NeverOverwrite", "yes");
    }
    if (installedFile->GetPropertyAsBool("CPACK_PERMANENT")) {
      AddAttribute("Permanent", "yes");
    }
  }

  patch.ApplyFragment(componentId, *this);
  BeginElement("File");
  AddAttribute("Id", fileId);
  AddAttribute("Source", filePath);
  AddAttribute("KeyPath", "yes");

  mode_t fileMode = 0;
  cmSystemTools::GetPermissions(filePath.c_str(), fileMode);

  if (!(fileMode & S_IWRITE)) {
    AddAttribute("ReadOnly", "yes");
  }
  patch.ApplyFragment(fileId, *this);

  if (installedFile) {
    cmWIXAccessControlList acl(Logger, *installedFile, *this);
    acl.Apply();
  }

  EndElement("File");

  EndElement("Component");
  EndElement("DirectoryRef");

  return componentId;
}
