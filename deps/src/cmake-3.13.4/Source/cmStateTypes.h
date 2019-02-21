/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmStateTypes_h
#define cmStateTypes_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmLinkedTree.h"

namespace cmStateDetail {
struct SnapshotDataType;
typedef cmLinkedTree<cmStateDetail::SnapshotDataType>::iterator PositionType;
}

namespace cmStateEnums {

enum SnapshotType
{
  BaseType,
  BuildsystemDirectoryType,
  FunctionCallType,
  MacroCallType,
  IncludeFileType,
  InlineListFileType,
  PolicyScopeType,
  VariableScopeType
};

// There are multiple overlapping ranges represented here. Be aware that adding
// a value to this enumeration may cause failures in numerous places which
// assume details about the ordering.
enum TargetType
{
  EXECUTABLE,
  STATIC_LIBRARY,
  SHARED_LIBRARY,
  MODULE_LIBRARY,
  OBJECT_LIBRARY,
  UTILITY,
  GLOBAL_TARGET,
  INTERFACE_LIBRARY,
  UNKNOWN_LIBRARY
};

enum CacheEntryType
{
  BOOL = 0,
  PATH,
  FILEPATH,
  STRING,
  INTERNAL,
  STATIC,
  UNINITIALIZED
};

enum ArtifactType
{
  RuntimeBinaryArtifact,
  ImportLibraryArtifact
};
}

#endif
