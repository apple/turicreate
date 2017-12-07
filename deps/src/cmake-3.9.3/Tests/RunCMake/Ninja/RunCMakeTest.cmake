include(RunCMake)

# Detect ninja version so we know what tests can be supported.
execute_process(
  COMMAND "${RunCMake_MAKE_PROGRAM}" --version
  OUTPUT_VARIABLE ninja_out
  ERROR_VARIABLE ninja_out
  RESULT_VARIABLE ninja_res
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )
if(ninja_res EQUAL 0 AND "x${ninja_out}" MATCHES "^x[0-9]+\\.[0-9]+")
  set(ninja_version "${ninja_out}")
  message(STATUS "ninja version: ${ninja_version}")
else()
  message(FATAL_ERROR "'ninja --version' reported:\n${ninja_out}")
endif()

function(run_NinjaToolMissing)
  set(RunCMake_MAKE_PROGRAM ninja-tool-missing)
  run_cmake(NinjaToolMissing)
endfunction()
run_NinjaToolMissing()

function(run_CMP0058 case)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CMP0058-${case}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(CMP0058-${case})
  run_cmake_command(CMP0058-${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

run_CMP0058(OLD-no)
run_CMP0058(OLD-by)
run_CMP0058(WARN-no)
run_CMP0058(WARN-by)
run_CMP0058(NEW-no)
run_CMP0058(NEW-by)

run_cmake(CustomCommandDepfile)

function(run_CommandConcat)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CommandConcat-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(CommandConcat)
  run_cmake_command(CommandConcat-build ${CMAKE_COMMAND} --build .)
endfunction()
run_CommandConcat()

function(run_SubDir)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/SubDir-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(SubDir)
  if(WIN32)
    set(SubDir_all [[SubDir\all]])
    set(SubDir_test [[SubDir\test]])
    set(SubDir_install [[SubDir\install]])
    set(SubDirBinary_test [[SubDirBinary\test]])
    set(SubDirBinary_all [[SubDirBinary\all]])
    set(SubDirBinary_install [[SubDirBinary\install]])
  else()
    set(SubDir_all [[SubDir/all]])
    set(SubDir_test [[SubDir/test]])
    set(SubDir_install [[SubDir/install]])
    set(SubDirBinary_all [[SubDirBinary/all]])
    set(SubDirBinary_test [[SubDirBinary/test]])
    set(SubDirBinary_install [[SubDirBinary/install]])
  endif()
  run_cmake_command(SubDir-build ${CMAKE_COMMAND} --build . --target ${SubDir_all})
  run_cmake_command(SubDir-test ${CMAKE_COMMAND} --build . --target ${SubDir_test})
  run_cmake_command(SubDir-install ${CMAKE_COMMAND} --build . --target ${SubDir_install})
  run_cmake_command(SubDirBinary-build ${CMAKE_COMMAND} --build . --target ${SubDirBinary_all})
  run_cmake_command(SubDirBinary-test ${CMAKE_COMMAND} --build . --target ${SubDirBinary_test})
  run_cmake_command(SubDirBinary-install ${CMAKE_COMMAND} --build . --target ${SubDirBinary_install})
endfunction()
run_SubDir()

function(run_ninja dir)
  execute_process(
    COMMAND "${RunCMake_MAKE_PROGRAM}" ${ARGN}
    WORKING_DIRECTORY "${dir}"
    OUTPUT_VARIABLE ninja_stdout
    ERROR_VARIABLE ninja_stderr
    RESULT_VARIABLE ninja_result
    )
  if(NOT ninja_result EQUAL 0)
    message(STATUS "
============ beginning of ninja's stdout ============
${ninja_stdout}
=============== end of ninja's stdout ===============
")
    message(STATUS "
============ beginning of ninja's stderr ============
${ninja_stderr}
=============== end of ninja's stderr ===============
")
    message(FATAL_ERROR
      "top ninja build failed exited with status ${ninja_result}")
  endif()
endfunction(run_ninja)

function (run_LooseObjectDepends)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/LooseObjectDepends-build)
  run_cmake(LooseObjectDepends)
  run_ninja("${RunCMake_TEST_BINARY_DIR}" "CMakeFiles/top.dir/top.c${CMAKE_C_OUTPUT_EXTENSION}")
  if (EXISTS "${RunCMake_TEST_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}dep${CMAKE_SHARED_LIBRARY_SUFFIX}")
    message(FATAL_ERROR
      "The `dep` library was created when requesting an object file to be "
      "built; this should no longer be necessary.")
  endif ()
  if (EXISTS "${RunCMake_TEST_BINARY_DIR}/CMakeFiles/dep.dir/dep.c${CMAKE_C_OUTPUT_EXTENSION}")
    message(FATAL_ERROR
      "The `dep.c` object file was created when requesting an object file to "
      "be built; this should no longer be necessary.")
  endif ()
endfunction ()
run_LooseObjectDepends()

function (run_AssumedSources)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/AssumedSources-build)
  run_cmake(AssumedSources)
  run_ninja("${RunCMake_TEST_BINARY_DIR}" "target.c")
  if (NOT EXISTS "${RunCMake_TEST_BINARY_DIR}/target.c")
    message(FATAL_ERROR
      "Dependencies for an assumed source did not hook up properly for 'target.c'.")
  endif ()
  run_ninja("${RunCMake_TEST_BINARY_DIR}" "target-no-depends.c")
  if (EXISTS "${RunCMake_TEST_BINARY_DIR}/target-no-depends.c")
    message(FATAL_ERROR
      "Dependencies for an assumed source were magically hooked up for 'target-no-depends.c'.")
  endif ()
endfunction ()
run_AssumedSources()

function(sleep delay)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E sleep ${delay}
    RESULT_VARIABLE result
    )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "failed to sleep for ${delay} second.")
  endif()
endfunction(sleep)

function(touch path)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E touch ${path}
    RESULT_VARIABLE result
    )
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "failed to touch main ${path} file.")
  endif()
endfunction(touch)

macro(ninja_escape_path path out)
  string(REPLACE "\$ " "\$\$" "${out}" "${path}")
  string(REPLACE " " "\$ " "${out}" "${${out}}")
  string(REPLACE ":" "\$:" "${out}" "${${out}}")
endmacro(ninja_escape_path)

macro(shell_escape string out)
  string(REPLACE "\"" "\\\"" "${out}" "${string}")
endmacro(shell_escape)

function(run_sub_cmake test ninja_output_path_prefix)
  set(top_build_dir "${RunCMake_BINARY_DIR}/${test}-build/")
  file(REMOVE_RECURSE "${top_build_dir}")
  file(MAKE_DIRECTORY "${top_build_dir}")

  ninja_escape_path("${ninja_output_path_prefix}"
    escaped_ninja_output_path_prefix)

  # Generate top build ninja file.
  set(top_build_ninja "${top_build_dir}/build.ninja")
  shell_escape("${top_build_ninja}" escaped_top_build_ninja)
  set(build_ninja_dep "${top_build_dir}/build_ninja_dep")
  ninja_escape_path("${build_ninja_dep}" escaped_build_ninja_dep)
  shell_escape("${CMAKE_COMMAND}" escaped_CMAKE_COMMAND)
  file(WRITE "${build_ninja_dep}" "fake dependency of top build.ninja file\n")
  if(WIN32)
    set(cmd_prefix "cmd.exe /C \"")
    set(cmd_suffix "\"")
  else()
    set(cmd_prefix "")
    set(cmd_suffix "")
  endif()
  file(WRITE "${top_build_ninja}" "\
subninja ${escaped_ninja_output_path_prefix}/build.ninja
default ${escaped_ninja_output_path_prefix}/all

# Sleep for 1 second before to regenerate to make sure the timestamp of
# the top build.ninja will be strictly greater than the timestamp of the
# sub/build.ninja file. We assume the system as 1 sec timestamp resolution.
rule RERUN
  command = ${cmd_prefix}\"${escaped_CMAKE_COMMAND}\" -E sleep 1 && \"${escaped_CMAKE_COMMAND}\" -E touch \"${escaped_top_build_ninja}\"${cmd_suffix}
  description = Testing regeneration
  generator = 1

build build.ninja: RERUN ${escaped_build_ninja_dep} || ${escaped_ninja_output_path_prefix}/build.ninja
  pool = console
")

  # Run sub cmake project.
  set(RunCMake_TEST_OPTIONS "-DCMAKE_NINJA_OUTPUT_PATH_PREFIX=${ninja_output_path_prefix}")
  set(RunCMake_TEST_BINARY_DIR "${top_build_dir}/${ninja_output_path_prefix}")
  run_cmake(${test})

  # Check there is no 'default' statement in Ninja file generated by CMake.
  set(sub_build_ninja "${RunCMake_TEST_BINARY_DIR}/build.ninja")
  file(READ "${sub_build_ninja}" sub_build_ninja_file)
  if(sub_build_ninja_file MATCHES "\ndefault [^\n][^\n]*all\n")
    message(FATAL_ERROR
      "unexpected 'default' statement found in '${sub_build_ninja}'")
  endif()

  # Run ninja from the top build directory.
  run_ninja("${top_build_dir}")

  # Test regeneration rules run in order.
  set(main_cmakelists "${RunCMake_SOURCE_DIR}/CMakeLists.txt")
  sleep(1) # Assume the system as 1 sec timestamp resolution.
  touch("${main_cmakelists}")
  touch("${build_ninja_dep}")
  run_ninja("${top_build_dir}")
  file(TIMESTAMP "${main_cmakelists}" mtime_main_cmakelists UTC)
  file(TIMESTAMP "${sub_build_ninja}" mtime_sub_build_ninja UTC)
  file(TIMESTAMP "${top_build_ninja}" mtime_top_build_ninja UTC)

  # Check sub build.ninja is regenerated.
  if(mtime_main_cmakelists STRGREATER mtime_sub_build_ninja)
    message(FATAL_ERROR
      "sub build.ninja not regenerated:
  CMakeLists.txt  = ${mtime_main_cmakelists}
  sub/build.ninja = ${mtime_sub_build_ninja}")
  endif()

  # Check top build.ninja is regenerated after sub build.ninja.
  if(NOT mtime_top_build_ninja STRGREATER mtime_sub_build_ninja)
    message(FATAL_ERROR
      "top build.ninja not regenerated strictly after sub build.ninja:
  sub/build.ninja = ${mtime_sub_build_ninja}
  build.ninja     = ${mtime_top_build_ninja}")
  endif()

endfunction(run_sub_cmake)

if("${ninja_version}" VERSION_LESS 1.6)
  message(WARNING "Ninja is too old; skipping rest of test.")
  return()
endif()

foreach(ninja_output_path_prefix "sub space" "sub")
  run_sub_cmake(Executable "${ninja_output_path_prefix}")
  run_sub_cmake(StaticLib  "${ninja_output_path_prefix}")
  run_sub_cmake(SharedLib "${ninja_output_path_prefix}")
  run_sub_cmake(TwoLibs "${ninja_output_path_prefix}")
  run_sub_cmake(SubDirPrefix "${ninja_output_path_prefix}")
  run_sub_cmake(CustomCommandWorkingDirectory "${ninja_output_path_prefix}")
endforeach(ninja_output_path_prefix)
