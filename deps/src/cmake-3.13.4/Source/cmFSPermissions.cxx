/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFSPermissions.h"

bool cmFSPermissions::stringToModeT(std::string const& arg,
                                    mode_t& permissions)
{
  if (arg == "OWNER_READ") {
    permissions |= mode_owner_read;
  } else if (arg == "OWNER_WRITE") {
    permissions |= mode_owner_write;
  } else if (arg == "OWNER_EXECUTE") {
    permissions |= mode_owner_execute;
  } else if (arg == "GROUP_READ") {
    permissions |= mode_group_read;
  } else if (arg == "GROUP_WRITE") {
    permissions |= mode_group_write;
  } else if (arg == "GROUP_EXECUTE") {
    permissions |= mode_group_execute;
  } else if (arg == "WORLD_READ") {
    permissions |= mode_world_read;
  } else if (arg == "WORLD_WRITE") {
    permissions |= mode_world_write;
  } else if (arg == "WORLD_EXECUTE") {
    permissions |= mode_world_execute;
  } else if (arg == "SETUID") {
    permissions |= mode_setuid;
  } else if (arg == "SETGID") {
    permissions |= mode_setgid;
  } else {
    return false;
  }
  return true;
}
