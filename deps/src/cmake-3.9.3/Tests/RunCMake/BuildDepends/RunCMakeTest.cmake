include(RunCMake)

if(RunCMake_GENERATOR STREQUAL "Borland Makefiles" OR
   RunCMake_GENERATOR STREQUAL "Watcom WMake")
  set(fs_delay 3)
else()
  set(fs_delay 1.125)
endif()

function(run_BuildDepends CASE)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${CASE}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  if(RunCMake_GENERATOR MATCHES "Make|Ninja")
    set(RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=Debug)
  endif()
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  include(${RunCMake_SOURCE_DIR}/${CASE}.step1.cmake OPTIONAL)
  run_cmake(${CASE})
  set(RunCMake-check-file check.cmake)
  set(check_step 1)
  run_cmake_command(${CASE}-build1 ${CMAKE_COMMAND} --build . --config Debug)
  if(run_BuildDepends_skip_step_2)
    return()
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${fs_delay}) # handle 1s resolution
  include(${RunCMake_SOURCE_DIR}/${CASE}.step2.cmake OPTIONAL)
  set(check_step 2)
  run_cmake_command(${CASE}-build2 ${CMAKE_COMMAND} --build . --config Debug)
endfunction()

run_BuildDepends(C-Exe)
if(NOT RunCMake_GENERATOR STREQUAL "Xcode")
  if(RunCMake_GENERATOR MATCHES "Visual Studio 10")
    # VS 10 forgets to re-link when a manifest changes
    set(run_BuildDepends_skip_step_2 1)
  endif()
  run_BuildDepends(C-Exe-Manifest)
  unset(run_BuildDepends_skip_step_2)
endif()

run_BuildDepends(Custom-Symbolic-and-Byproduct)
run_BuildDepends(Custom-Always)

if(RunCMake_GENERATOR MATCHES "Make")
  run_BuildDepends(MakeCustomIncludes)
  if(NOT "${RunCMake_BINARY_DIR}" STREQUAL "${RunCMake_SOURCE_DIR}")
    run_BuildDepends(MakeInProjectOnly)
  endif()
endif()

function(run_ReGeneration)
  # test re-generation of project even if CMakeLists.txt files disappeared

  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/regenerate-project-build)
  set(RunCMake_TEST_SOURCE_DIR ${RunCMake_BINARY_DIR}/regenerate-project-source)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}")
  set(ProjectHeader [=[
    cmake_minimum_required(VERSION 3.5)
    project(Regenerate-Project NONE)
  ]=])

  # create project with subdirectory
  file(WRITE "${RunCMake_TEST_SOURCE_DIR}/CMakeLists.txt" "${ProjectHeader}"
    "add_subdirectory(mysubdir)")
  file(MAKE_DIRECTORY "${RunCMake_TEST_SOURCE_DIR}/mysubdir")
  file(WRITE "${RunCMake_TEST_SOURCE_DIR}/mysubdir/CMakeLists.txt" "# empty")

  run_cmake(Regenerate-Project)
  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep ${fs_delay})

  # now we delete the subdirectory and adjust the CMakeLists.txt
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}/mysubdir")
  file(WRITE "${RunCMake_TEST_SOURCE_DIR}/CMakeLists.txt" "${ProjectHeader}")

  run_cmake_command(Regenerate-Project-Directory-Removed
    ${CMAKE_COMMAND} --build "${RunCMake_TEST_BINARY_DIR}")

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_SOURCE_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
endfunction()

if(RunCMake_GENERATOR STREQUAL "Xcode")
  run_ReGeneration(regenerate-project)
endif()
