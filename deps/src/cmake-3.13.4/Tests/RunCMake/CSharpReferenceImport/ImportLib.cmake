enable_language(CXX CSharp)

if(NOT DEFINED exportFileName OR
   NOT DEFINED exportNameSpace OR
   NOT DEFINED exportTargetName)
  message(FATAL_ERROR "export information missing")
endif()

add_library(${exportTargetName}CSharp SHARED
  ImportLib.cs)

# native c++ dll
add_library(${exportTargetName}Native SHARED
  ImportLibNative.h
  ImportLibNative.cxx)

# mixed c++ dll
add_library(${exportTargetName}Mixed SHARED
  ImportLibMixed.cxx
  ImportLibMixedNative.h
  ImportLibMixedNative.cxx)
set_target_properties(${exportTargetName}Mixed PROPERTIES
  COMMON_LANGUAGE_RUNTIME "")

# pure c++ dll
add_library(${exportTargetName}Pure SHARED
  ImportLibPure.cxx)
set_target_properties(${exportTargetName}Pure PROPERTIES
  COMMON_LANGUAGE_RUNTIME "pure")

# safe c++ dll
add_library(${exportTargetName}Safe SHARED
  ImportLibSafe.cxx)
set_target_properties(${exportTargetName}Safe PROPERTIES
  COMMON_LANGUAGE_RUNTIME "safe")

# generate export file
export(TARGETS
  ${exportTargetName}CSharp
  ${exportTargetName}Native
  ${exportTargetName}Mixed
  ${exportTargetName}Pure
  ${exportTargetName}Safe
  NAMESPACE "${exportNameSpace}:"
  FILE "${exportFileName}")
