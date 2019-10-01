enable_language(CXX CSharp)

if(NOT DEFINED exportFileName OR
   NOT DEFINED exportNameSpace OR
   NOT DEFINED exportTargetName)
  message(FATAL_ERROR "export information missing")
endif()

# Include generated export file.
if(NOT EXISTS "${exportFileName}")
  message(FATAL_ERROR "exportFileNameCSharp does not exist: ${exportFileName}")
endif()
include(${exportFileName})

# Verify expected targets are imported
set(linkNames linkNameCSharp linkNameNative linkNameMixed
  linkNamePure linkNameSafe)
set(linkNameCSharp "${exportNameSpace}:${exportTargetName}CSharp")
set(linkNameNative "${exportNameSpace}:${exportTargetName}Native")
set(linkNameMixed "${exportNameSpace}:${exportTargetName}Mixed")
set(linkNamePure "${exportNameSpace}:${exportTargetName}Pure")
set(linkNameSafe "${exportNameSpace}:${exportTargetName}Safe")
foreach(l ${linkNames})
  if(NOT TARGET ${${l}})
    message(FATAL_ERROR "imported target not found (${${l}})")
  endif()
endforeach()

# Test referencing managed assemblies from C# executable.
add_executable(ReferenceImportCSharp ReferenceImport.cs)
target_link_libraries(ReferenceImportCSharp
  ${linkNameCSharp}
  ${linkNameNative} # ignored
  ${linkNameMixed}
  ${linkNamePure}
  ${linkNameSafe}
  )

# native C++ executable.
add_executable(ReferenceImportNative ReferenceImportNative.cxx)
target_link_libraries(ReferenceImportNative
  ${linkNameCSharp} # ignored
  ${linkNameNative}
  ${linkNameMixed}
  ${linkNamePure} # ignored
  ${linkNameSafe} # ignored
  )
add_custom_command(TARGET ReferenceImportNative POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${linkNameNative}>"
    "${CMAKE_BINARY_DIR}/$<CONFIG>"
  )

# mixed C++ executable.
add_executable(ReferenceImportMixed ReferenceImportMixed.cxx)
target_link_libraries(ReferenceImportMixed
  ${linkNameCSharp}
  ${linkNameNative}
  ${linkNameMixed}
  ${linkNamePure}
  ${linkNameSafe}
  )
set_target_properties(ReferenceImportMixed PROPERTIES
  COMMON_LANGUAGE_RUNTIME "")

# pure C++ executable.
add_executable(ReferenceImportPure ReferenceImportPure.cxx)
target_link_libraries(ReferenceImportPure
  ${linkNameCSharp}
  ${linkNameNative} # ignored
  ${linkNameMixed}
  ${linkNamePure}
  ${linkNameSafe}
  )
set_target_properties(ReferenceImportPure PROPERTIES
  COMMON_LANGUAGE_RUNTIME "pure")

# native C++ executable.
add_executable(ReferenceImportSafe ReferenceImportSafe.cxx)
target_link_libraries(ReferenceImportSafe
  ${linkNameCSharp}
  ${linkNameNative} # ignored
  ${linkNameMixed}
  ${linkNamePure}
  ${linkNameSafe}
  )
set_target_properties(ReferenceImportSafe PROPERTIES
  COMMON_LANGUAGE_RUNTIME "safe")
