# this test creates a static library and an executable
# the source to the library is then changed
# and the build is done on the executable and if things
# are working the executable should relink with the new
# value.  The subdir Project contains the CMakelists.txt
# and source files for the test project.
cmake_minimum_required (VERSION 2.6)
project(BuildDepends)

# This entire test takes place during the initial
# configure step.  It should not run again when the
# project is built.
set(CMAKE_SUPPRESS_REGENERATION 1)

# Xcode needs some help with the fancy dependencies in this test.
if(XCODE AND XCODE_VERSION VERSION_LESS 5)
  set(HELP_XCODE 1)
endif()
function(help_xcode_depends)
  if(HELP_XCODE)
    file(GLOB_RECURSE MACRO_OBJS
      ${BuildDepends_BINARY_DIR}/Project/zot_macro_*.o*
      )
    if(MACRO_OBJS)
      message("Helping Xcode by removing objects [${MACRO_OBJS}]")
      file(REMOVE ${MACRO_OBJS})
    endif()
  endif()
endfunction()

# The Intel compiler causes the MSVC linker to crash during
# incremental linking, so avoid the /INCREMENTAL:YES flag.
if(WIN32 AND "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
  set(_cmake_options "-DCMAKE_EXE_LINKER_FLAGS=")
endif()

if("${CMAKE_GENERATOR}" MATCHES "Make|Ninja")
  set(TEST_LINK_DEPENDS ${BuildDepends_BINARY_DIR}/Project/linkdep.txt)
  file(WRITE ${TEST_LINK_DEPENDS} "1")
endif()
list(APPEND _cmake_options "-DTEST_LINK_DEPENDS=${TEST_LINK_DEPENDS}")

list(APPEND _cmake_options "-DCMAKE_FORCE_DEPFILES=1")

if(NOT CMAKE_GENERATOR MATCHES "Visual Studio ([^89]|[89][0-9])")
  set(TEST_MULTI3 1)
  list(APPEND _cmake_options "-DTEST_MULTI3=1")
endif()

file(MAKE_DIRECTORY ${BuildDepends_BINARY_DIR}/Project)
message("Creating Project/foo.cxx")
write_file(${BuildDepends_BINARY_DIR}/Project/foo.cxx
  "const char* foo() { return \"foo\";}" )

file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot.hxx.in
  "static const char* zot = \"zot\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/dir/header.txt
  "#define HEADER_STRING \"ninja\"\n" )
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_custom.hxx.in
  "static const char* zot_custom = \"zot_custom\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_macro_dir.hxx
  "static const char* zot_macro_dir = \"zot_macro_dir\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_macro_tgt.hxx
  "static const char* zot_macro_tgt = \"zot_macro_tgt\";\n")

file(WRITE ${BuildDepends_BINARY_DIR}/Project/link_depends_no_shared_lib.h
  "#define link_depends_no_shared_lib_value 1\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/link_depends_no_shared_exe.h
  "#define link_depends_no_shared_exe_value 0\n")
set(link_depends_no_shared_check_txt ${BuildDepends_BINARY_DIR}/Project/link_depends_no_shared_check.txt)

file(WRITE ${BuildDepends_BINARY_DIR}/Project/object_depends.txt "0\n")
set(object_depends_check_txt ${BuildDepends_BINARY_DIR}/Project/object_depends_check.txt)

file(WRITE ${BuildDepends_BINARY_DIR}/Project/external.in "external original\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi1-in.txt "multi1-in original\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi2-stamp.txt "multi2-stamp original\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi3-stamp.txt "multi3-stamp original\n")

help_xcode_depends()

message("Building project first time")
try_compile(RESULT
  ${BuildDepends_BINARY_DIR}/Project
  ${BuildDepends_SOURCE_DIR}/Project
  testRebuild
  CMAKE_FLAGS ${_cmake_options}
  OUTPUT_VARIABLE OUTPUT)
if(HELP_XCODE)
  try_compile(RESULT
    ${BuildDepends_BINARY_DIR}/Project
    ${BuildDepends_SOURCE_DIR}/Project
    testRebuild
    OUTPUT_VARIABLE OUTPUT)
  try_compile(RESULT
    ${BuildDepends_BINARY_DIR}/Project
    ${BuildDepends_SOURCE_DIR}/Project
    testRebuild
    OUTPUT_VARIABLE OUTPUT)
endif()

message("Output from first build:\n${OUTPUT}")
if(NOT RESULT)
  message(SEND_ERROR "Could not build test project (1)!")
endif()

# find and save the ninjadep executable
set(ninjadep ${BuildDepends_BINARY_DIR}/Project/ninjadep${CMAKE_EXECUTABLE_SUFFIX})
if(EXISTS
    "${BuildDepends_BINARY_DIR}/Project/Debug/ninjadep${CMAKE_EXECUTABLE_SUFFIX}" )
  message("found debug")
  set(ninjadep
    "${BuildDepends_BINARY_DIR}/Project/Debug/ninjadep${CMAKE_EXECUTABLE_SUFFIX}")
endif()
message("Running ${ninjadep}  ")
execute_process(COMMAND ${ninjadep} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

if("${out}" STREQUAL "HEADER_STRING: ninja ")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not rebuild properly. Output[${out}]\n"
    " expected [HEADER_STRING: ninja]")
endif()

set(bar ${BuildDepends_BINARY_DIR}/Project/bar${CMAKE_EXECUTABLE_SUFFIX})
if(EXISTS
    "${BuildDepends_BINARY_DIR}/Project/Debug/bar${CMAKE_EXECUTABLE_SUFFIX}" )
  message("found debug")
  set(bar
    "${BuildDepends_BINARY_DIR}/Project/Debug/bar${CMAKE_EXECUTABLE_SUFFIX}")
endif()
set(zot ${BuildDepends_BINARY_DIR}/Project/zot${CMAKE_EXECUTABLE_SUFFIX})
if(EXISTS
    "${BuildDepends_BINARY_DIR}/Project/Debug/zot${CMAKE_EXECUTABLE_SUFFIX}" )
  message("found debug")
  set(zot
    "${BuildDepends_BINARY_DIR}/Project/Debug/zot${CMAKE_EXECUTABLE_SUFFIX}")
endif()

message("Running ${bar}  ")
execute_process(COMMAND ${bar} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

if("${out}" STREQUAL "foo ")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not initially build properly: ${out}")
endif()

message("Running ${zot}  ")
execute_process(COMMAND ${zot} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

set(VALUE_UNCHANGED "[zot] [zot_custom] [zot_macro_dir] [zot_macro_tgt] ")
if("${out}" STREQUAL "${VALUE_UNCHANGED}")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not initially build properly: ${out}")
endif()

if(EXISTS "${link_depends_no_shared_check_txt}")
  file(STRINGS "${link_depends_no_shared_check_txt}" link_depends_no_shared_check LIMIT_COUNT 1)
  if("${link_depends_no_shared_check}" STREQUAL "1")
    message(STATUS "link_depends_no_shared_exe is newer than link_depends_no_shared_lib as expected.")
  else()
    message(SEND_ERROR "Project did not initially build properly: "
      "link_depends_no_shared_exe is older than link_depends_no_shared_lib.")
  endif()
else()
  message(SEND_ERROR "Project did not initially build properly: "
    "Targets link_depends_no_shared_lib and link_depends_no_shared_exe not both built.")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/external.out)
  file(STRINGS ${BuildDepends_BINARY_DIR}/Project/external.out external_out)
  if("${external_out}" STREQUAL "external original")
    message(STATUS "external.out contains '${external_out}'")
  else()
    message(SEND_ERROR "Project did not initially build properly: "
      "external.out contains '${external_out}'")
  endif()
else()
  message(SEND_ERROR "Project did not initially build properly: "
    "external.out is missing")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi1-out2-copy.txt)
  file(STRINGS ${BuildDepends_BINARY_DIR}/Project/multi1-out2-copy.txt multi1_out)
  if("${multi1_out}" STREQUAL "multi1-in original")
    message(STATUS "multi1-out2-copy.txt contains '${multi1_out}'")
  else()
    message(SEND_ERROR "Project did not initially build properly: "
      "multi1-out2-copy.txt contains '${multi1_out}'")
  endif()
else()
  message(SEND_ERROR "Project did not initially build properly: "
    "multi1-out2-copy.txt is missing")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi2-real.txt)
  if(${BuildDepends_BINARY_DIR}/Project/multi2-real.txt
      IS_NEWER_THAN ${BuildDepends_BINARY_DIR}/Project/multi2-stamp.txt)
    message(STATUS "multi2-real.txt is newer than multi2-stamp.txt")
  else()
    message(SEND_ERROR "Project did not initially build properly: "
      "multi2-real.txt is not newer than multi2-stamp.txt")
  endif()
else()
  message(SEND_ERROR "Project did not initially build properly: "
    "multi2-real.txt is missing")
endif()

if(TEST_MULTI3)
  if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi3-real.txt)
    if(${BuildDepends_BINARY_DIR}/Project/multi3-real.txt
        IS_NEWER_THAN ${BuildDepends_BINARY_DIR}/Project/multi3-stamp.txt)
      message(STATUS "multi3-real.txt is newer than multi3-stamp.txt")
    else()
      message(SEND_ERROR "Project did not initially build properly: "
        "multi3-real.txt is not newer than multi3-stamp.txt")
    endif()
  else()
    message(SEND_ERROR "Project did not initially build properly: "
      "multi3-real.txt is missing")
  endif()
endif()

message("Waiting 3 seconds...")
execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 3)

message("Modifying Project/foo.cxx")
write_file(${BuildDepends_BINARY_DIR}/Project/foo.cxx
  "const char* foo() { return \"foo changed\";}" )
file(WRITE "${BuildDepends_BINARY_DIR}/Project/dir/header.txt"
  "#define HEADER_STRING \"ninja changed\"\n" )
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot.hxx.in
  "static const char* zot = \"zot changed\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_custom.hxx.in
  "static const char* zot_custom = \"zot_custom changed\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_macro_dir.hxx
  "static const char* zot_macro_dir = \"zot_macro_dir changed\";\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/zot_macro_tgt.hxx
  "static const char* zot_macro_tgt = \"zot_macro_tgt changed\";\n")

file(WRITE ${BuildDepends_BINARY_DIR}/Project/link_depends_no_shared_lib.h
  "#define link_depends_no_shared_lib_value 0\n")

file(WRITE ${BuildDepends_BINARY_DIR}/Project/object_depends.txt "1\n")

if(TEST_LINK_DEPENDS)
  file(WRITE ${TEST_LINK_DEPENDS} "2")
endif()

file(WRITE ${BuildDepends_BINARY_DIR}/Project/external.in "external changed\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi1-in.txt "multi1-in changed\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi2-stamp.txt "multi2-stamp changed\n")
file(WRITE ${BuildDepends_BINARY_DIR}/Project/multi3-stamp.txt "multi3-stamp changed\n")

help_xcode_depends()

message("Building project second time")
try_compile(RESULT
  ${BuildDepends_BINARY_DIR}/Project
  ${BuildDepends_SOURCE_DIR}/Project
  testRebuild
  CMAKE_FLAGS ${_cmake_options}
  OUTPUT_VARIABLE OUTPUT)

# Xcode is in serious need of help here
if(HELP_XCODE)
  try_compile(RESULT
    ${BuildDepends_BINARY_DIR}/Project
    ${BuildDepends_SOURCE_DIR}/Project
    testRebuild
    OUTPUT_VARIABLE OUTPUT)
  try_compile(RESULT
    ${BuildDepends_BINARY_DIR}/Project
    ${BuildDepends_SOURCE_DIR}/Project
    testRebuild
    OUTPUT_VARIABLE OUTPUT)
endif()

message("Output from second build:\n${OUTPUT}")
if(NOT RESULT)
  message(SEND_ERROR "Could not build test project (2)!")
endif()
if(EXISTS
    "${BuildDepends_BINARY_DIR}/Project/Debug/bar${CMAKE_EXECUTABLE_SUFFIX}" )
  message("found debug")
endif()
if(EXISTS
    "${BuildDepends_BINARY_DIR}/Project/Debug/zot${CMAKE_EXECUTABLE_SUFFIX}" )
  message("found debug")
endif()

message("Running ${ninjadep}  ")
execute_process(COMMAND ${ninjadep} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

if("${out}" STREQUAL "HEADER_STRING: ninja changed ")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not rebuild properly. Output[${out}]\n"
    " expected [HEADER_STRING: ninja changed]")
endif()

message("Running ${bar}  ")
execute_process(COMMAND ${bar} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

if("${out}" STREQUAL "foo changed ")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not rebuild properly!")
endif()

message("Running ${zot}  ")
execute_process(COMMAND ${zot} OUTPUT_VARIABLE out RESULT_VARIABLE runResult)
string(REGEX REPLACE "[\r\n]" " " out "${out}")
message("Run result: ${runResult} Output: \"${out}\"")

set(VALUE_CHANGED
  "[zot changed] [zot_custom changed] [zot_macro_dir changed] [zot_macro_tgt changed] "
  )
if("${out}" STREQUAL "${VALUE_CHANGED}")
  message("Worked!")
else()
  message(SEND_ERROR "Project did not rebuild properly!")
endif()

if(TEST_LINK_DEPENDS)
  set(linkdep ${BuildDepends_BINARY_DIR}/Project/linkdep${CMAKE_EXECUTABLE_SUFFIX})
  if(${linkdep} IS_NEWER_THAN ${TEST_LINK_DEPENDS})
    message("LINK_DEPENDS worked")
  else()
    message(SEND_ERROR "LINK_DEPENDS failed.  Executable
  ${linkdep}
is not newer than dependency
  ${TEST_LINK_DEPENDS}
")
  endif()
endif()

if(EXISTS "${link_depends_no_shared_check_txt}")
  file(STRINGS "${link_depends_no_shared_check_txt}" link_depends_no_shared_check LIMIT_COUNT 1)
  if("${link_depends_no_shared_check}" STREQUAL "0")
    message(STATUS "link_depends_no_shared_exe is older than link_depends_no_shared_lib as expected.")
  elseif(XCODE AND NOT XCODE_VERSION VERSION_LESS 5)
    message(STATUS "Known limitation: link_depends_no_shared_exe is newer than link_depends_no_shared_lib but we cannot stop Xcode ${XCODE_VERSION} from enforcing this dependency.")
  else()
    message(SEND_ERROR "Project did not rebuild properly: link_depends_no_shared_exe is newer than link_depends_no_shared_lib.")
  endif()
else()
  message(SEND_ERROR "Project did not rebuild properly.  "
    "Targets link_depends_no_shared_lib and link_depends_no_shared_exe not both built.")
endif()

if(EXISTS "${object_depends_check_txt}")
  file(STRINGS "${object_depends_check_txt}" object_depends_check LIMIT_COUNT 1)
  if("${object_depends_check}" STREQUAL "1")
    message(STATUS "object_depends exe is newer than object_depends.txt as expected.")
  elseif(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
    message(STATUS "Known limitation: OBJECT_DEPENDS does not work on ${CMAKE_GENERATOR}")
  else()
    message(SEND_ERROR "Project did not rebuild properly: object_depends exe is not newer than object_depends.txt.")
  endif()
else()
  message(SEND_ERROR "Project did not rebuild properly.  "
    "object_depends exe and object_depends.txt are not both present.")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/external.out)
  file(STRINGS ${BuildDepends_BINARY_DIR}/Project/external.out external_out)
  if("${external_out}" STREQUAL "external changed")
    message(STATUS "external.out contains '${external_out}'")
  else()
    message(SEND_ERROR "Project did not rebuild properly: "
      "external.out contains '${external_out}'")
  endif()
else()
  message(SEND_ERROR "Project did not rebuild properly: "
    "external.out is missing")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi1-out2-copy.txt)
  file(STRINGS ${BuildDepends_BINARY_DIR}/Project/multi1-out2-copy.txt multi1_out)
  if("${multi1_out}" STREQUAL "multi1-in changed")
    message(STATUS "multi1-out2-copy.txt contains '${multi1_out}'")
  else()
    message(SEND_ERROR "Project did not rebuild properly: "
      "multi1-out2-copy.txt contains '${multi1_out}'")
  endif()
else()
  message(SEND_ERROR "Project did not rebuild properly: "
    "multi1-out2-copy.txt is missing")
endif()

if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi2-real.txt)
  if(${BuildDepends_BINARY_DIR}/Project/multi2-real.txt
      IS_NEWER_THAN ${BuildDepends_BINARY_DIR}/Project/multi2-stamp.txt)
    message(STATUS "multi2-real.txt is newer than multi2-stamp.txt")
  else()
    message(SEND_ERROR "Project did not rebuild properly: "
      "multi2-real.txt is not newer than multi2-stamp.txt")
  endif()
else()
  message(SEND_ERROR "Project did not rebuild properly: "
    "multi2-real.txt is missing")
endif()

if(TEST_MULTI3)
  if(EXISTS ${BuildDepends_BINARY_DIR}/Project/multi3-real.txt)
    if(${BuildDepends_BINARY_DIR}/Project/multi3-real.txt
        IS_NEWER_THAN ${BuildDepends_BINARY_DIR}/Project/multi3-stamp.txt)
      message(STATUS "multi3-real.txt is newer than multi3-stamp.txt")
    else()
      message(SEND_ERROR "Project did not rebuild properly: "
        "multi3-real.txt is not newer than multi3-stamp.txt")
    endif()
  else()
    message(SEND_ERROR "Project did not rebuild properly: "
      "multi3-real.txt is missing")
  endif()
endif()
