/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFSPermissions_h
#define cmFSPermissions_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cm_sys_stat.h"

#include <string>

namespace cmFSPermissions {

// Table of permissions flags.
#if defined(_WIN32) && !defined(__CYGWIN__)
static const mode_t mode_owner_read = S_IREAD;
static const mode_t mode_owner_write = S_IWRITE;
static const mode_t mode_owner_execute = S_IEXEC;
static const mode_t mode_group_read = 040;
static const mode_t mode_group_write = 020;
static const mode_t mode_group_execute = 010;
static const mode_t mode_world_read = 04;
static const mode_t mode_world_write = 02;
static const mode_t mode_world_execute = 01;
static const mode_t mode_setuid = 04000;
static const mode_t mode_setgid = 02000;
#else
static const mode_t mode_owner_read = S_IRUSR;
static const mode_t mode_owner_write = S_IWUSR;
static const mode_t mode_owner_execute = S_IXUSR;
static const mode_t mode_group_read = S_IRGRP;
static const mode_t mode_group_write = S_IWGRP;
static const mode_t mode_group_execute = S_IXGRP;
static const mode_t mode_world_read = S_IROTH;
static const mode_t mode_world_write = S_IWOTH;
static const mode_t mode_world_execute = S_IXOTH;
static const mode_t mode_setuid = S_ISUID;
static const mode_t mode_setgid = S_ISGID;
#endif

bool stringToModeT(std::string const& arg, mode_t& permissions);

} // ns

#endif
