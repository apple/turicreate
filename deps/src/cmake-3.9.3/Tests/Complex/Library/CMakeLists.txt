remove_definitions(-DCMAKE_IS_REALLY_FUN)

#
# Small utility used to create file
# UTILITY_SOURCE is used for coverage and for getting the exact name
# of the executable.
#
utility_source(CREATE_FILE_EXE create_file "." create_file.cxx)
add_executable(create_file create_file.cxx)
set_target_properties(create_file PROPERTIES RUNTIME_OUTPUT_DIRECTORY ".")

#
# Create static library
# SOURCE_FILES_REMOVE is used for Coverage. empty.h is included for coverage
#
aux_source_directory(ExtraSources LibrarySources)
set(LibrarySources ${LibrarySources}
  file2
  empty
  create_file.cxx
  GENERATED
  nonexisting_file)
remove(LibrarySources create_file.cxx GENERATED nonexisting_file)
add_library(CMakeTestLibrary ${LibrarySources})

if(WIN32)
  if(NOT CYGWIN)
    if(NOT BORLAND)
      if(NOT MINGW)
        target_link_libraries(CMakeTestLibrary
          debug
          user32.lib)
        target_link_libraries(CMakeTestLibrary
          optimized
          kernel32.lib)
      endif()
    endif()
  endif()
endif()

#
# Create shared library
#
set(SharedLibrarySources sharedFile)
add_library(CMakeTestLibraryShared SHARED ${SharedLibrarySources})
string(APPEND CMAKE_C_FLAGS " -DTEST_C_FLAGS")
add_library(CMakeTestCLibraryShared SHARED testConly.c)
define_property(
  TARGET PROPERTY FOO
  BRIEF_DOCS "a test property"
  FULL_DOCS "A simple etst proerty that means nothign and is used for nothing"
  )
set_target_properties(CMakeTestCLibraryShared PROPERTIES FOO BAR)
if(NOT BEOS AND NOT WIN32 AND NOT HAIKU)  # No libm on BeOS.
  set_target_properties(CMakeTestCLibraryShared PROPERTIES LINK_FLAGS "-lm")
endif()
get_target_property(FOO_BAR_VAR CMakeTestCLibraryShared FOO)
if(${FOO_BAR_VAR} MATCHES "BAR")
else()
  message(SEND_ERROR "SET_TARGET_PROPERTIES or GET_TARGET_PROPERTY failed, FOO_BAR_VAR should be BAR, but is ${FOO_BAR_VAR}")
endif()

# Create static and shared lib of same name.
if(CMAKE_EXE_LINK_STATIC_CXX_FLAGS)
  add_library(CMakeTestLinkStatic STATIC TestLink.c)
  add_library(CMakeTestLinkShared SHARED TestLink.c)
  set_target_properties(CMakeTestLinkStatic CMakeTestLinkShared
    PROPERTIES OUTPUT_NAME CMakeTestLink)
endif()

#
# Attach pre-build/pre-link/post-build custom-commands to the lib.
# Each runs ${CREATE_FILE_EXE} which will create a file.
# The 'complex' executable will then test if this file exists and remove it.
#
add_dependencies(CMakeTestLibraryShared create_file)
message("complex bin dir is ${Complex_BINARY_DIR}")
add_custom_command(TARGET CMakeTestLibraryShared PRE_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "${Complex_BINARY_DIR}/Library/prebuild.txt")
add_custom_command(TARGET CMakeTestLibraryShared PRE_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "${Complex_BINARY_DIR}/Library/prelink.txt")
add_custom_command(TARGET CMakeTestLibraryShared POST_BUILD
                   COMMAND ${CREATE_FILE_EXE}
                   ARGS "${Complex_BINARY_DIR}/Library/postbuild.txt")
add_custom_command(TARGET CMakeTestLibraryShared POST_BUILD
                   COMMAND ${CMAKE_COMMAND}
                   ARGS -E copy
                        "${Complex_BINARY_DIR}/Library/postbuild.txt"
                        "${Complex_BINARY_DIR}/Library/postbuild2.txt")

#
# Add a custom target.
# It runs ${CREATE_FILE_EXE} which will create a file.
# The 'complex' executable will then test if this file exists and remove it.
#
add_custom_target(custom_target1
                  ALL
                  ${CREATE_FILE_EXE}
                  "${Complex_BINARY_DIR}/Library/custom_target1.txt")

add_dependencies(custom_target1 create_file)

#
# Extra coverage
#
set_source_files_properties(file2 PROPERTIES ABSTRACT 1)

install_files(/tmp .h ${Complex_BINARY_DIR}/cmTestConfigure.h)
install_files(/tmp .cxx ${Complex_BINARY_DIR}/cmTestConfigure.h)

# Test creating a library that is not built by default.
add_library(notInAllLib EXCLUDE_FROM_ALL notInAllLib.cxx)

# Create an imported target for if(TARGET) test in Executable dir.
# That test should not see this target.
add_library(LibImportedTarget UNKNOWN IMPORTED)

# Test generation of preprocessed sources.
if("${CMAKE_GENERATOR}" MATCHES "Makefile" AND CMAKE_MAKE_PROGRAM)
  if(CMAKE_CXX_CREATE_PREPROCESSED_SOURCE)
    # Skip running this part of the test on certain platforms
    # until they are fixed.
    set(MAYBE_ALL ALL)
    list(LENGTH CMAKE_OSX_ARCHITECTURES ARCH_COUNT)
    if(ARCH_COUNT GREATER 1)
      # OSX does not support preprocessing more than one architecture.
      set(MAYBE_ALL)
    endif()

    # Custom target to try preprocessing invocation.
    add_custom_target(test_preprocess ${MAYBE_ALL}
      COMMAND ${CMAKE_COMMAND} -E remove CMakeFiles/create_file.dir/create_file.i
      COMMAND ${CMAKE_MAKE_PROGRAM} create_file.i
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/test_preprocess.cmake
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )
  endif()
endif()
