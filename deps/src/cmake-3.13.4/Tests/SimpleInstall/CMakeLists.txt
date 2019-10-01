cmake_minimum_required (VERSION 2.6)
project (TestSimpleInstall)
set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
  "${TestSimpleInstall_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
  "${TestSimpleInstall_BINARY_DIR}/lib/static")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
  "${TestSimpleInstall_BINARY_DIR}/lib")

# Skip generating the rpath pointing at the build tree to make sure
# the executable is installed with the proper rpath in the install
# tree.
set(CMAKE_SKIP_BUILD_RPATH 1)

# Make sure the executable can run from the install tree.
set(CMAKE_INSTALL_NAME_DIR ${CMAKE_INSTALL_PREFIX}/MyTest/lib)

# Skip the dependency that causes a build when installing.  This
# avoids infinite loops when the post-build rule below installs.
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY 1)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY 1)

set(CMAKE_DEBUG_POSTFIX "_test_debug_postfix")

set(EXTRA_INSTALL_FLAGS)
message("Extra install: ${EXTRA_INSTALL_FLAGS}")

if(STAGE2)
  set(LIBPATHS
    ${CMAKE_INSTALL_PREFIX}/MyTest/lib/static
    ${CMAKE_INSTALL_PREFIX}/MyTest/lib
    )
  set(t1NAMES test1 test1${CMAKE_DEBUG_POSTFIX} test1rel)
  set(t2NAMES test2 test2${CMAKE_DEBUG_POSTFIX})
  set(t4NAMES test4out test4out${CMAKE_DEBUG_POSTFIX})

  # Make sure the install script ran.
  set(CMAKE_INSTALL_SCRIPT_DID_RUN 0)
  include(${CMAKE_INSTALL_PREFIX}/MyTest/InstallScriptOut.cmake OPTIONAL)
  if(CMAKE_INSTALL_SCRIPT_DID_RUN)
    message(STATUS "Stage 1 did run install script 2.")
  else()
    message(SEND_ERROR "Stage 1 did not run install script 2.")
  endif()

  if(CYGWIN OR MINGW)
    set(LIBPATHS ${LIBPATHS} "${CMAKE_INSTALL_PREFIX}/MyTest/bin")
  endif()
  message("Search for library in: ${LIBPATHS}")

  set(TEST1_LIBRARY "TEST1_LIBRARY-NOTFOUND" CACHE FILEPATH "Force find." FORCE)
  set(TEST2_LIBRARY "TEST2_LIBRARY-NOTFOUND" CACHE FILEPATH "Force find." FORCE)
  set(TEST4_LIBRARY "TEST4_LIBRARY-NOTFOUND" CACHE FILEPATH "Force find." FORCE)

  find_library(TEST1_LIBRARY
    NAMES ${t1NAMES}
    PATHS ${LIBPATHS}
    DOC "First library")
  find_library(TEST2_LIBRARY
    NAMES ${t2NAMES}
    PATHS ${LIBPATHS}
    DOC "Second library")
  find_library(TEST4_LIBRARY
    NAMES ${t4NAMES}
    PATHS ${LIBPATHS}
    DOC "Fourth library")

  # Test importing a library found on disk.
  add_library(lib_test4 UNKNOWN IMPORTED)
  set_property(TARGET lib_test4 PROPERTY IMPORTED_LOCATION ${TEST4_LIBRARY})

  include_directories(${CMAKE_INSTALL_PREFIX}/MyTest/include)
  add_executable (SimpleInstExeS2 inst2.cxx foo.c foo.h)
  target_link_libraries(SimpleInstExeS2 ${TEST1_LIBRARY} ${TEST2_LIBRARY} lib_test4)
  set(install_target SimpleInstExeS2)

  if("${TEST1_LIBRARY}" MATCHES "static")
    message(STATUS "test1 correctly found in lib/static")
  else()
    message(SEND_ERROR "test1 not found in lib/static!")
  endif()

  # Check for failure of configuration-specific installation.
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/include/Release/lib1debug.h")
    message(FATAL_ERROR "Debug-configuration file installed for Release!")
  endif()
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/include/Debug/lib1release.h")
    message(FATAL_ERROR "Release-configuration file installed for Debug!")
  endif()

  # Check for failure of directory installation.
  if(NOT EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/TestSubDir/TSD.h")
    message(FATAL_ERROR "Directory installation did not install TSD.h")
  endif()
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/TestSubDir/CVS")
    message(FATAL_ERROR "Directory installation installed CVS directory.")
  endif()
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/CVS")
    message(FATAL_ERROR "Directory installation installed CVS directory.")
  endif()
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/CMakeLists.txt")
    message(FATAL_ERROR "Directory installation installed CMakeLists.txt.")
  endif()
  if(NOT EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/alt/TestSubDir/TSD.h")
    message(FATAL_ERROR "Directory installation did not install alternate TSD.h")
  endif()
  if(EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/alt/TestSubDir/TSD.cxx")
    message(FATAL_ERROR "Directory installation installed alternate TSD.cxx")
  endif()

  # Check that scripts properly installed.
  if(WIN32 AND NOT CYGWIN)
    set(BAT .bat)
  else()
    set(BAT)
  endif()
  foreach(loc share share/old1 share/old2 share/old3 share/alt)
    set(CUR_SCRIPT "${CMAKE_INSTALL_PREFIX}/MyTest/${loc}/sample_script${BAT}")
    execute_process(
      COMMAND ${CUR_SCRIPT}
      RESULT_VARIABLE SAMPLE_SCRIPT_RESULT
      OUTPUT_VARIABLE SAMPLE_SCRIPT_OUTPUT
      )
    if(NOT "${SAMPLE_SCRIPT_RESULT}" STREQUAL "0")
      message(FATAL_ERROR
        "Sample script [${CUR_SCRIPT}] failed: [${SAMPLE_SCRIPT_RESULT}]")
    endif()
    if(NOT "${SAMPLE_SCRIPT_OUTPUT}" MATCHES "Sample Script Output")
      message(FATAL_ERROR
        "Bad sample script [${CUR_SCRIPT}] output: [${SAMPLE_SCRIPT_OUTPUT}]")
    endif()
  endforeach()

  # Check for failure of empty directory installation.
  if(NOT EXISTS "${CMAKE_INSTALL_PREFIX}/MyTest/share/empty")
    message(FATAL_ERROR "Empty directory installation did not install.")
  endif()
  file(GLOB EMPTY_FILES "${CMAKE_INSTALL_PREFIX}/MyTest/share/empty/*")
  if(EMPTY_FILES)
    message(FATAL_ERROR "Empty directory installed [${EMPTY_FILES}].")
  endif()

  # Make sure the test executable can run from the install tree.
  set_target_properties(SimpleInstExeS2 PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/MyTest/lib)

  install_targets(/MyTest/bin SimpleInstExeS2)

# try to import the exported targets again
  set(SimpleInstallS1_DIR ${CMAKE_INSTALL_PREFIX}/MyTest/lib)
  find_package(SimpleInstallS1 REQUIRED)
  get_target_property(simpleInstallImported S1_SimpleInstall IMPORTED)
  if(NOT simpleInstallImported)
    message(FATAL_ERROR "Target S1_SimpleInstall could not be imported")
  endif()

else()
  # Wipe out the install directory to do a fresh test.
  file(REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/MyTest)

  # this is stage 1, so create libraries and modules and install everything
  add_library(test1 STATIC lib1.cxx)
  add_library(test2 SHARED lib2.cxx)
  add_library(test3 MODULE lib3.cxx)
  add_library(test4 SHARED lib4.cxx)

  # Test <ARCHIVE|LIBRARY|RUNTIME>_OUTPUT_NAME properties.
  set_property(TARGET test4 PROPERTY ARCHIVE_OUTPUT_NAME test4out)
  set_property(TARGET test4 PROPERTY LIBRARY_OUTPUT_NAME test4out)

  add_executable (SimpleInstall inst.cxx foo.c foo.h)
  target_link_libraries(SimpleInstall test1 test2 test4)
  set(install_target SimpleInstall)

  set_target_properties(SimpleInstall PROPERTIES OUTPUT_NAME SimpleInstExe)
  # Disable VERSION test until it is implemented in the Xcode generator.
  if(NOT XCODE)
    set_target_properties(SimpleInstall PROPERTIES VERSION 1.2)
  endif()

  # Make sure the test executable can run from the install tree.
  set_target_properties(SimpleInstall PROPERTIES
    INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/MyTest/lib)

  # Test per-configuration output name.
  set_target_properties(test1 PROPERTIES RELEASE_OUTPUT_NAME test1rel)
  set_target_properties(test2 PROPERTIES PUBLIC_HEADER foo.h)

  if(CMAKE_GENERATOR MATCHES "Makefiles")
    add_subdirectory(TestSubDir)
    add_dependencies(SimpleInstall TSD)
  endif()

  add_dependencies(SimpleInstall test3)
  add_dependencies(test2 test3)
  add_dependencies(test4 test2)

  install(TARGETS SimpleInstall test1 test2 test3 EXPORT SimpleInstallS1
    RUNTIME DESTINATION MyTest/bin        COMPONENT Runtime     # .exe, .dll
    LIBRARY DESTINATION MyTest/lib        COMPONENT Runtime     # .so, mod.dll
    ARCHIVE DESTINATION MyTest/lib/static COMPONENT Development # .a, .lib
    PUBLIC_HEADER DESTINATION MyTest/include COMPONENT Development
    )

  install(TARGETS test4 PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    RUNTIME DESTINATION MyTest/bin
    LIBRARY DESTINATION MyTest/lib
    ARCHIVE DESTINATION MyTest/lib/static
    OPTIONAL # for coverage...target should always exist
    )
  install(FILES lib1.h DESTINATION MyTest/include/$<1:foo>$<0:/wrong>)
  install(FILES lib2.h
    DESTINATION $<1:MyTest/include/foo>$<0:/wrong>
    COMPONENT Development
    PERMISSIONS OWNER_READ OWNER_WRITE
    RENAME lib2renamed.h
    )

  # Test old-style install commands.
  install_files(/MyTest/include FILES lib3.h)
  install_files(/MyTest/include/old .h lib3)
  install_files(/MyTest/include/old "^lib2.h$")
  install_programs(/MyTest/share/old1 FILES
    scripts/sample_script scripts/sample_script.bat)
  install_programs(/MyTest/share/old2
    scripts/sample_script scripts/sample_script.bat)

# "export" the targets collected in "SimpleInstallS1"
  install(EXPORT SimpleInstallS1 FILE SimpleInstallS1Config.cmake
                                 DESTINATION MyTest/lib
                                 NAMESPACE S1_ )

  export(TARGETS SimpleInstall test1 test2 test3
         FILE "${CMAKE_CURRENT_BINARY_DIR}/SimpleInstallS1Config.cmake"
         NAMESPACE S2_ )

  add_subdirectory(scripts)

  # Test optional installation.
  install(FILES does_not_exist.h DESTINATION MyTest/include/foo OPTIONAL)

  # Test configuration-specific installation.
  install(FILES lib1.h RENAME lib1release.h CONFIGURATIONS Release
    DESTINATION MyTest/include/Release
    )
  install(FILES lib1.h RENAME lib1debug.h CONFIGURATIONS Debug
    DESTINATION MyTest/include/Debug
    )

  # Test directory installation.
  file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/MyTest/share/CVS")
  file(REMOVE_RECURSE "${CMAKE_INSTALL_PREFIX}/MyTest/share/TestSubDir/CVS")
  install(
    DIRECTORY TestSubDir $<1:scripts/>$<0:/wrong> DESTINATION $<1:MyTest/share>$<0:/wrong>
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE
    DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                          GROUP_READ GROUP_EXECUTE
    PATTERN "CVS" EXCLUDE
    REGEX "\\.txt$" EXCLUDE
    PATTERN "scripts/*" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
    )

  # Alternate directory installation for coverage.
  install(
    DIRECTORY $<1:scripts/>$<0:/wrong> DESTINATION $<1:MyTest/share/alt>$<0:/wrong>
    COMPONENT Development
    USE_SOURCE_PERMISSIONS
    PATTERN "CVS" EXCLUDE
    REGEX "\\.txt$" EXCLUDE
    )
  install(
    DIRECTORY TestSubDir DESTINATION $<1:MyTest/share/alt>$<0:/wrong>
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE
    DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                          GROUP_READ GROUP_EXECUTE
    FILES_MATCHING PATTERN "*.h"
    )

  # Test empty directory installation.
  install(DIRECTORY DESTINATION MyTest/share/empty)

  # Test optional directory installation.
  install(DIRECTORY does-not-exist DESTINATION MyTest/share OPTIONAL)

  # Test user-specified install scripts, with and without COMPONENT.
  install(
    SCRIPT InstallScript1.cmake
    CODE "set(INSTALL_CODE_DID_RUN 1)"
    SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/InstallScript2.cmake
    )
  install(
    SCRIPT InstallScript3.cmake
    CODE "set(INSTALL_CODE_WITH_COMPONENT_DID_RUN 1)"
    SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/InstallScript4.cmake
    COMPONENT Development
    )
  set_directory_properties(PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES
    "${CMAKE_INSTALL_PREFIX}/InstallScriptOut.cmake;${CMAKE_INSTALL_PREFIX}/InstallScript4Out.cmake")

  set_target_properties(SimpleInstall PROPERTIES PRE_INSTALL_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/PreInstall.cmake)
  set_target_properties(SimpleInstall PROPERTIES POST_INSTALL_SCRIPT
    ${CMAKE_CURRENT_SOURCE_DIR}/PostInstall.cmake)
  set_target_properties(test4 PROPERTIES VERSION 1.2 SOVERSION 3
        INSTALL_NAME_DIR @executable_path/../lib)
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(SI_CONFIG --config $<CONFIGURATION>)
else()
  set(SI_CONFIG)
endif()

# Dummy test of CPack
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Test of packaging with cpack")
set(CPACK_PACKAGE_VENDOR "Kitware")
set(CPACK_INSTALL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/PackageScript.cmake")

if(WIN32 AND NOT UNIX)
  find_program(NSIS_MAKENSIS NAMES makensis
    PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\NSIS]
    DOC "Where is makensis.exe located"
    )
  if(NOT NSIS_MAKENSIS)
    set(CPACK_GENERATOR TGZ)
  endif()
endif()
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "TGZ;STGZ;TZ")
  #  find_program(found_compress
  #    NAMES compress)
  #  if(found_compress)
  #    find_program(file_command NAMES file)
  #    if(NOT file_command)
  #      set(file_command file)
  #    endif()
  #    execute_process(COMMAND ${file_command} ${found_compress}
  #      OUTPUT_VARIABLE output)
  #    set(SKIP_TZ FALSE)
  #    if("${output}" MATCHES "script")
  #      set(SKIP_TZ TRUE)
  #    endif()
  #    if("${output}" MATCHES "dummy.sh")
  #      set(SKIP_TZ TRUE)
  #    endif()
  #    if(NOT SKIP_TZ)
  #      message("compress found and it was not a script")
  #      message("output from file command: [${output}]")
  #      list(APPEND CPACK_GENERATOR "TZ")
  #    else()
  #      message("compress found, but it was a script so dont use it")
  #      message("output from file command: [${output}]")
  #    endif()
  #  endif()
  find_program(found_bz2
    NAMES bzip2)
  if(found_bz2)
    list(APPEND CPACK_GENERATOR "TBZ2")
  endif()
endif()

set(CPACK_PACKAGE_EXECUTABLES "SimpleInstall" "Simple Install")
set(CMAKE_INSTALL_MFC_LIBRARIES 1)
set(CMAKE_INSTALL_DEBUG_LIBRARIES 1)
set(CMAKE_INSTALL_UCRT_LIBRARIES 1)
set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
include(InstallRequiredSystemLibraries)

if(CTEST_TEST_CPACK)
  set(package_command COMMAND
    ${CMAKE_COMMAND} --build . --target package ${SI_CONFIG}
    )

  # Avoid settings that require the .zip file command line tools...
  # (just build an NSIS installer without component support)
  #
  set(CPACK_BINARY_ZIP OFF)
  set(CPACK_MONOLITHIC_INSTALL ON)
else()
  set(package_command)
endif()

include(CPack)

set(install_command COMMAND
  ${CMAKE_COMMAND} --build . --target install ${SI_CONFIG}
  )

add_custom_command(
  TARGET ${install_target}
  POST_BUILD
  ${install_command}
  ${package_command}
  COMMENT "Install Project"
  )
