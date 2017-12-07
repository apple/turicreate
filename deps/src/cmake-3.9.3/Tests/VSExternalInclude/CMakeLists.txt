cmake_minimum_required (VERSION 2.6)
project(VSExternalInclude)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio 1[01245]")
  set(PROJECT_EXT vcxproj)
else()
  set(PROJECT_EXT vcproj)
endif()

# make sure directories exists
set(LIB1_BINARY_DIR ${VSExternalInclude_BINARY_DIR}/Lib1)
make_directory("${LIB1_BINARY_DIR}")

set(LIB2_BINARY_DIR ${VSExternalInclude_BINARY_DIR}/Lib2)
make_directory("${LIB2_BINARY_DIR}")

# generate lib1
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -T "${CMAKE_GENERATOR_TOOLSET}" "${VSExternalInclude_SOURCE_DIR}/Lib1"
  WORKING_DIRECTORY ${LIB1_BINARY_DIR}
  OUTPUT_VARIABLE OUT
  ERROR_VARIABLE OUT
  )
message("CMAKE Ran with the following output:\n\"${OUT}\"")

# generate lib2
execute_process(
  COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}"
    -A "${CMAKE_GENERATOR_PLATFORM}"
    -T "${CMAKE_GENERATOR_TOOLSET}" "${VSExternalInclude_SOURCE_DIR}/Lib2"
  WORKING_DIRECTORY ${LIB2_BINARY_DIR}
  OUTPUT_VARIABLE OUT
  ERROR_VARIABLE OUT
  )
message("CMAKE Ran with the following output:\n\"${OUT}\"")


include_external_msproject(lib1 ${VSExternalInclude_BINARY_DIR}/Lib1/LIB1.${PROJECT_EXT})
# lib2 depends on lib1
include_external_msproject(lib2 ${VSExternalInclude_BINARY_DIR}/Lib2/LIB2.${PROJECT_EXT} lib1)

include_directories(${VSExternalInclude_SOURCE_DIR}/Lib2 ${VSExternalInclude_SOURCE_DIR}/Lib1)

set(SOURCES main.cpp)

add_executable(VSExternalInclude ${SOURCES})

# target depends on lib2
add_dependencies(VSExternalInclude lib2)
# VS 10 vcxproj files have depends in them
# Since lib1 and lib2 do not depend on each other
# then the vcxproj files do not depend on each other
# and the sln file can no longer be the only source
# of that depend.  So, for VS 10 make the executable
# depend on lib1 and lib2
if(${CMAKE_GENERATOR} MATCHES "Visual Studio 1[01245]")
  add_dependencies(VSExternalInclude lib1)
endif()

# Interaction testing between the FOLDER target property and
# INCLUDE_EXTERNAL_MSPROJECT targets:
set_target_properties(VSExternalInclude PROPERTIES FOLDER folder1/folder2)
set_target_properties(lib1 PROPERTIES FOLDER folder1/folder2)
set_target_properties(lib2 PROPERTIES FOLDER folder1/folder2)
add_custom_target(EmptyCustomTarget)
set_target_properties(EmptyCustomTarget PROPERTIES FOLDER folder1/folder2)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
