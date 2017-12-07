cmake_minimum_required(VERSION 2.6)
project(testRebuild)

function(test_for_xcode4 result_var)
  set(${result_var} 0 PARENT_SCOPE)
  if(APPLE)
    execute_process(COMMAND xcodebuild -version
      OUTPUT_VARIABLE ov RESULT_VARIABLE rv
      )
    if("${rv}" STREQUAL "0" AND ov MATCHES "^Xcode ([0-9]+)\\.")
      if(NOT CMAKE_MATCH_1 VERSION_LESS 4)
        set(${result_var} 1 PARENT_SCOPE)
      endif()
    endif()
  endif()
endfunction()

if(APPLE)
  # only use multi-arch if the sysroot exists on this machine
  # Ninja needs -M which could not be used with multiple -arch flags
  if(EXISTS "${CMAKE_OSX_SYSROOT}" AND NOT "${CMAKE_GENERATOR}" MATCHES "Ninja")
    set(CMAKE_OSX_ARCHITECTURES "ppc;i386")
    test_for_xcode4(is_xcode4)
    if(is_xcode4)
      # Xcode 4, use modern architectures as defaults
      # Arch 'ppc' no longer works: tools no longer available starting with Xcode 4
      set(CMAKE_OSX_ARCHITECTURES i386 x86_64)
    endif()
  endif()
endif()

add_library(foo STATIC ${testRebuild_BINARY_DIR}/foo.cxx)
set_target_properties(foo PROPERTIES OUTPUT_NAME "foolib")
# Add a generated header that regenerates when the generator is
# rebuilt.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/regen.h
  COMMAND generator ${CMAKE_CURRENT_BINARY_DIR}/regen.h regen
  DEPENDS generator # adds file-level dependency to re-run rule
  )

# Add a generated header that does NOT regenerate when the generator
# is rebuilt.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/noregen.h
  COMMAND generator ${CMAKE_CURRENT_BINARY_DIR}/noregen.h noregen
  )

# Test that the generator rebuilds when the static library source file
# changes.  This should cause regen.h to be recreated also.
add_executable(generator generator.cxx)
target_link_libraries(generator foo)
set_target_properties(generator PROPERTIES OUTPUT_NAME "gen")

# Build an executable to drive the build and rebuild.
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(bar bar.cxx
  ${CMAKE_CURRENT_BINARY_DIR}/regen.h
  ${CMAKE_CURRENT_BINARY_DIR}/noregen.h
  )

#-----------------------------------------------------------------------------
if("${CMAKE_GENERATOR}" MATCHES "Make")
  # Test the IMPLICIT_DEPENDS feature.
  set(ZOT_DEPENDS IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/dep.cxx)
  set(ZOT_CUSTOM_DEP
    IMPLICIT_DEPENDS CXX ${CMAKE_CURRENT_SOURCE_DIR}/dep_custom.cxx
                     CXX ${CMAKE_CURRENT_SOURCE_DIR}/dep_custom2.cxx )
else()
  # No IMPLICIT_DEPENDS...just depend directly.
  set(ZOT_DEPENDS DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zot.hxx.in)
  set(ZOT_CUSTOM_DEP DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/zot_custom.hxx.in)
endif()
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zot.hxx
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/zot.hxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/zot.hxx
  ${ZOT_DEPENDS}
  )

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/zot_custom.hxx
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_CURRENT_BINARY_DIR}/zot_custom.hxx.in
  ${CMAKE_CURRENT_BINARY_DIR}/zot_custom.hxx
  ${ZOT_CUSTOM_DEP}
  )
add_custom_target(zot_custom ALL DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/zot_custom.hxx)

add_executable(zot zot.cxx ${CMAKE_CURRENT_BINARY_DIR}/zot.hxx
  zot_macro_dir.cxx zot_macro_tgt.cxx)
add_dependencies(zot zot_custom)

# Test the #include line macro transformation rule support.
set_property(
  TARGET zot
  PROPERTY IMPLICIT_DEPENDS_INCLUDE_TRANSFORM "ZOT_TGT(%)=<zot_%_tgt.hxx>"
  )

set_property(
  DIRECTORY
  PROPERTY IMPLICIT_DEPENDS_INCLUDE_TRANSFORM "ZOT_DIR(%)=<zot_%_dir.hxx>"
  )

if(TEST_LINK_DEPENDS)
  add_executable(linkdep linkdep.cxx)
  set_property(TARGET linkdep PROPERTY LINK_DEPENDS ${TEST_LINK_DEPENDS})
endif()

add_library(link_depends_no_shared_lib SHARED link_depends_no_shared_lib.c
  ${CMAKE_CURRENT_BINARY_DIR}/link_depends_no_shared_lib.h)
add_executable(link_depends_no_shared_exe link_depends_no_shared_exe.c
  ${CMAKE_CURRENT_BINARY_DIR}/link_depends_no_shared_exe.h)
target_link_libraries(link_depends_no_shared_exe link_depends_no_shared_lib)
set_property(TARGET link_depends_no_shared_exe PROPERTY LINK_DEPENDS_NO_SHARED 1)
add_custom_target(link_depends_no_shared_check ALL
  COMMAND ${CMAKE_COMMAND}
   -Dlib=$<TARGET_FILE:link_depends_no_shared_lib>
   -Dexe=$<TARGET_FILE:link_depends_no_shared_exe>
   -Dout=${CMAKE_CURRENT_BINARY_DIR}/link_depends_no_shared_check.txt
   -P ${CMAKE_CURRENT_SOURCE_DIR}/link_depends_no_shared_check.cmake
  )
add_dependencies(link_depends_no_shared_check link_depends_no_shared_exe)

add_custom_command(
  OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/dir/header.h
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dir/header.txt
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_CURRENT_BINARY_DIR}/dir/header.txt
  ${CMAKE_CURRENT_BINARY_DIR}/dir/header.h
  )

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/dir/header.h
  PROPERTIES GENERATED 1)

add_custom_target(header_tgt DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dir/header.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(ninjadep ninjadep.cpp)
add_dependencies(ninjadep header_tgt)

include(ExternalProject)
ExternalProject_Add(ExternalBuild
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/External
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/External/Stamp
  BUILD_ALWAYS 1
  CMAKE_ARGS
    -Dexternal_in=${CMAKE_CURRENT_BINARY_DIR}/external.in
    -Dexternal_out=${CMAKE_CURRENT_BINARY_DIR}/external.out
  INSTALL_COMMAND ""
  )

add_custom_command(
  OUTPUT multi1-out1.txt multi1-out2.txt
  COMMAND ${CMAKE_COMMAND} -E copy multi1-in.txt multi1-out1.txt
  COMMAND ${CMAKE_COMMAND} -E copy multi1-in.txt multi1-out2.txt
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/multi1-in.txt
  )
add_custom_command(
  OUTPUT multi1-out2-copy.txt
  COMMAND ${CMAKE_COMMAND} -E copy multi1-out2.txt multi1-out2-copy.txt
  DEPENDS multi1-out2.txt
  )
add_custom_target(multi1 ALL DEPENDS multi1-out2-copy.txt)

# Test having the first output never created.
add_custom_command(
  OUTPUT multi2-dummy.txt multi2-real.txt
  COMMAND ${CMAKE_COMMAND} -E touch multi2-real.txt
  )
set_property(SOURCE multi2-real.txt multi2-dummy.txt PROPERTY SYMBOLIC 1)
add_custom_target(multi2 ALL DEPENDS multi2-real.txt)

if(TEST_MULTI3)
  # Test having the second output never created.  Does not work with msbuild.
  add_custom_command(
    OUTPUT multi3-real.txt multi3-dummy.txt
    COMMAND ${CMAKE_COMMAND} -E touch multi3-real.txt
    )
  set_property(SOURCE multi3-real.txt multi3-dummy.txt PROPERTY SYMBOLIC 1)
  add_custom_target(multi3 ALL DEPENDS multi3-real.txt)
endif()

add_executable(object_depends object_depends.cxx)
set_property(SOURCE object_depends.cxx PROPERTY OBJECT_DEPENDS
  ${CMAKE_CURRENT_BINARY_DIR}/object_depends.txt)
add_custom_target(object_depends_check ALL
  COMMAND ${CMAKE_COMMAND}
   -Dexe=$<TARGET_FILE:object_depends>
   -Dout=${CMAKE_CURRENT_BINARY_DIR}/object_depends_check.txt
   -Dtxt=${CMAKE_CURRENT_BINARY_DIR}/object_depends.txt
   -P ${CMAKE_CURRENT_SOURCE_DIR}/object_depends_check.cmake
  )
add_dependencies(object_depends_check object_depends)
