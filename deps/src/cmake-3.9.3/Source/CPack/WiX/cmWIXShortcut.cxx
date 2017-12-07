/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXShortcut.h"

#include "cmWIXFilesSourceWriter.h"

void cmWIXShortcuts::insert(Type type, std::string const& id,
                            cmWIXShortcut const& shortcut)
{
  this->Shortcuts[type][id].push_back(shortcut);
}

bool cmWIXShortcuts::empty(Type type) const
{
  return this->Shortcuts.find(type) == this->Shortcuts.end();
}

bool cmWIXShortcuts::EmitShortcuts(
  Type type, std::string const& registryKey,
  std::string const& cpackComponentName,
  cmWIXFilesSourceWriter& fileDefinitions) const
{
  shortcut_type_map_t::const_iterator i = this->Shortcuts.find(type);

  if (i == this->Shortcuts.end()) {
    return false;
  }

  shortcut_id_map_t const& id_map = i->second;

  std::string shortcutPrefix;
  std::string registrySuffix;

  switch (type) {
    case START_MENU:
      shortcutPrefix = "CM_S";
      break;
    case DESKTOP:
      shortcutPrefix = "CM_DS";
      registrySuffix = "_desktop";
      break;
    case STARTUP:
      shortcutPrefix = "CM_SS";
      registrySuffix = "_startup";
      break;
    default:
      return false;
  }

  for (shortcut_id_map_t::const_iterator j = id_map.begin(); j != id_map.end();
       ++j) {
    std::string const& id = j->first;
    shortcut_list_t const& shortcutList = j->second;

    for (size_t shortcutListIndex = 0; shortcutListIndex < shortcutList.size();
         ++shortcutListIndex) {
      cmWIXShortcut const& shortcut = shortcutList[shortcutListIndex];
      fileDefinitions.EmitShortcut(id, shortcut, shortcutPrefix,
                                   shortcutListIndex);
    }
  }

  fileDefinitions.EmitInstallRegistryValue(registryKey, cpackComponentName,
                                           registrySuffix);

  return true;
}

void cmWIXShortcuts::AddShortcutTypes(std::set<Type>& types)
{
  for (shortcut_type_map_t::const_iterator i = this->Shortcuts.begin();
       i != this->Shortcuts.end(); ++i) {
    types.insert(i->first);
  }
}

void cmWIXShortcuts::CreateFromProperties(std::string const& id,
                                          std::string const& directoryId,
                                          cmInstalledFile const& installedFile)
{
  CreateFromProperty("CPACK_START_MENU_SHORTCUTS", START_MENU, id, directoryId,
                     installedFile);

  CreateFromProperty("CPACK_DESKTOP_SHORTCUTS", DESKTOP, id, directoryId,
                     installedFile);

  CreateFromProperty("CPACK_STARTUP_SHORTCUTS", STARTUP, id, directoryId,
                     installedFile);
}

void cmWIXShortcuts::CreateFromProperty(std::string const& propertyName,
                                        Type type, std::string const& id,
                                        std::string const& directoryId,
                                        cmInstalledFile const& installedFile)
{
  std::vector<std::string> list;
  installedFile.GetPropertyAsList(propertyName, list);

  for (size_t i = 0; i < list.size(); ++i) {
    cmWIXShortcut shortcut;
    shortcut.label = list[i];
    shortcut.workingDirectoryId = directoryId;
    insert(type, id, shortcut);
  }
}
