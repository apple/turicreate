#
# Create exe.
#
string(APPEND CMAKE_CXX_FLAGS " -DTEST_CXX_FLAGS")
string(APPEND CMAKE_C_FLAGS " -DTEST_C_FLAGS")

# Create an imported target for if(TARGET) test below.
add_library(ExeImportedTarget UNKNOWN IMPORTED)

# Test if(TARGET) command.
if(NOT TARGET CMakeTestLibrary)
  message(FATAL_ERROR "if(NOT TARGET CMakeTestLibrary) returned true!")
endif()
if(NOT TARGET ExeImportedTarget)
  message(FATAL_ERROR "if(NOT TARGET ExeImportedTarget) returned true!")
endif()
if(TARGET LibImportedTarget)
  message(FATAL_ERROR "if(TARGET LibImportedTarget) returned true!")
endif()
if(TARGET NotATarget)
  message(FATAL_ERROR "if(TARGET NotATarget) returned true!")
endif()

# Use LINK_LIBRARIES instead of TARGET_LINK_LIBRARIES to
set(COMPLEX_LIBS CMakeTestLibrary;CMakeTestLibraryShared;CMakeTestCLibraryShared)
link_libraries(${COMPLEX_LIBS})

# Test forcing a .cxx file to not build.
set_source_files_properties(complex_nobuild.cxx PROPERTIES
  HEADER_FILE_ONLY 1)

# Test forcing a .c file to not build.
# This makes sure a mixed language library is created
# with header file only sources
set_source_files_properties(complex_nobuild.c PROPERTIES
  HEADER_FILE_ONLY 1)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(A A.cxx A.hh A.h A.txt)
add_custom_command(OUTPUT Aout.h COMMAND A > Aout.h VERBATIM)
add_executable(complex complex testcflags.c Aout.h)
#  Sub1/NameConflictTest.c Sub2/NameConflictTest.c)
add_executable(complex.file complex.file.cxx complex_nobuild.cxx
  complex_nobuild.c)

if (UNIX)
  target_link_libraries(complex ${CMAKE_DL_LIBS})
else()
  if (NOT BORLAND)
    if(NOT MINGW)
      target_link_libraries(complex rpcrt4.lib)
    endif()
  endif()
endif ()

# Test linking to static lib when a shared lib has the same name.
if(CMAKE_EXE_LINK_STATIC_CXX_FLAGS)
  add_definitions(-DCOMPLEX_TEST_LINK_STATIC)
  target_link_libraries(complex CMakeTestLinkStatic)
endif()

# can we get the path to a source file
get_source_file_property(A_LOCATION A.cxx LOCATION)
if ("${A_LOCATION}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/A.cxx")
  add_definitions(-DCMAKE_FOUND_ACXX)
endif ()

# get the directory parent
get_directory_property(P_VALUE PARENT_DIRECTORY)
if ("${P_VALUE}" STREQUAL "${CMAKE_SOURCE_DIR}")
  add_definitions(-DCMAKE_FOUND_PARENT)
endif ()

# get the stack of listfiles
include(Included.cmake)
if ("${LF_VALUE}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt;${CMAKE_CURRENT_SOURCE_DIR}/Included.cmake")
  add_definitions(-DCMAKE_FOUND_LISTFILE_STACK)
endif ()

# Test add/remove definitions.
add_definitions(
  -DCOMPLEX_DEFINED_PRE
  -DCOMPLEX_DEFINED
  -DCOMPLEX_DEFINED_POST
  -DCOMPLEX_DEFINED
  )
remove_definitions(-DCOMPLEX_DEFINED)

# Test pre-build/pre-link/post-build rules for an executable.
add_custom_command(TARGET complex PRE_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "Executable/prebuild.txt"
                   WORKING_DIRECTORY "${Complex_BINARY_DIR}")
add_custom_command(TARGET complex PRE_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "Executable/prelink.txt"
                   WORKING_DIRECTORY "${Complex_BINARY_DIR}")
add_custom_command(TARGET complex POST_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "Executable/postbuild.txt"
                   WORKING_DIRECTORY "${Complex_BINARY_DIR}")
add_custom_command(TARGET complex POST_BUILD
                   COMMAND ${CMAKE_COMMAND}
                   ARGS -E copy
                        "Executable/postbuild.txt"
                        "Executable/postbuild2.txt"
                   WORKING_DIRECTORY "${Complex_BINARY_DIR}")

set_source_files_properties(complex
  COMPILE_FLAGS
  "-DFILE_HAS_EXTRA_COMPILE_FLAGS"
  #" -DFILE_DEFINE_STRING=\\\"hello\\\""
  OBJECT_DEPENDS ${Complex_BINARY_DIR}/cmTestGeneratedHeader.h
)
set_target_properties(complex PROPERTIES COMPILE_FLAGS "-DCOMPLEX_TARGET_FLAG")
add_custom_command(
  TARGET complex
  SOURCE ${Complex_SOURCE_DIR}/cmTestGeneratedHeader.h.in
  COMMAND ${CMAKE_COMMAND}
  ARGS -E copy ${Complex_SOURCE_DIR}/cmTestGeneratedHeader.h.in
               ${Complex_BINARY_DIR}/cmTestGeneratedHeader.h
  OUTPUTS ${Complex_BINARY_DIR}/cmTestGeneratedHeader.h
  DEPENDS ${CMAKE_COMMAND}
)

# Test creating an executable that is not built by default.
add_executable(notInAllExe EXCLUDE_FROM_ALL notInAllExe.cxx)
target_link_libraries(notInAllExe notInAllLib)

# Test user-value flag mapping for the VS IDE.
if(MSVC)
  set_target_properties(notInAllExe PROPERTIES
    LINK_FLAGS "/NODEFAULTLIB:LIBC;LIBCMT;MSVCRT")
endif()

# Test creating a custom target that builds not-in-all targets.
add_custom_target(notInAllCustom)
add_dependencies(notInAllCustom notInAllExe)

#
# Output the files required by 'complex' to a file.
#
# This test has been moved to the 'required' subdir so that it
# has no side-effects on the current Makefile (duplicated source file
# due to source list expansion done twice).
#
add_subdirectory(Temp)

if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_INCLUDE_SYSTEM_FLAG_CXX
   AND NOT XCODE) # XCODE is excluded due to #15687
  add_executable(testSystemDir testSystemDir.cxx)
  set_target_properties(testSystemDir PROPERTIES COMPILE_FLAGS "-Werror")
endif()

#
# Extra coverage.Not used.
#
install_targets(/tmp complex)
install_programs(/tmp complex)

configure_file(
  ${Complex_SOURCE_DIR}/Executable/cmVersion.h.in
  ${Complex_BINARY_DIR}/cmVersion.h)

source_group(A_GROUP ".cxx")
source_group(B_GROUP REGULAR_EXPRESSION "cxx")
source_group(C_GROUP FILES complex.cxx)

file(WRITE ${Complex_BINARY_DIR}/A/libA.a "test")
file(WRITE ${Complex_BINARY_DIR}/A/libC.a "test")
file(WRITE ${Complex_BINARY_DIR}/B/libB.a "test")
file(WRITE ${Complex_BINARY_DIR}/B/libA.a "test")
file(WRITE ${Complex_BINARY_DIR}/C/libC.a "test")
file(WRITE ${Complex_BINARY_DIR}/C/libB.a "test")
