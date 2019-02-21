# Check the CMake source tree and report anything suspicious...
#
message("=============================================================================")
message("CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message("")
message("CMake_BINARY_DIR='${CMake_BINARY_DIR}'")
message("CMake_SOURCE_DIR='${CMake_SOURCE_DIR}'")
message("GIT_EXECUTABLE='${GIT_EXECUTABLE}'")
message("HOME='${HOME}'")
message("ENV{DASHBOARD_TEST_FROM_CTEST}='$ENV{DASHBOARD_TEST_FROM_CTEST}'")
message("")
string(REPLACE "\\" "\\\\" HOME "${HOME}")


# Is the build directory the same as or underneath the source directory?
# (i.e. - is it an "in source" build?)
#
set(in_source_build 0)
set(build_under_source 0)

string(FIND "${CMake_BINARY_DIR}" "${CMake_SOURCE_DIR}/" pos)
if(pos EQUAL 0)
  message("build dir is *inside* source dir")
  set(build_under_source 1)
elseif(CMake_SOURCE_DIR STREQUAL "${CMake_BINARY_DIR}")
  message("build dir *is* source dir")
  set(in_source_build 1)
else()
  string(LENGTH "${CMake_SOURCE_DIR}" src_len)
  string(LENGTH "${CMake_BINARY_DIR}" bin_len)

  if(bin_len GREATER src_len)
    math(EXPR substr_len "${src_len}+1")
    string(SUBSTRING "${CMake_BINARY_DIR}" 0 ${substr_len} bin_dir)
    if(bin_dir STREQUAL "${CMake_SOURCE_DIR}/")
      message("build dir is under source dir")
      set(in_source_build 1)
    endif()
  endif()
endif()

message("src_len='${src_len}'")
message("bin_len='${bin_len}'")
message("substr_len='${substr_len}'")
message("bin_dir='${bin_dir}'")
message("in_source_build='${in_source_build}'")
message("build_under_source='${build_under_source}'")
message("")

if(build_under_source)
  message(STATUS "Skipping rest of test because build tree is under source tree")
  return()
endif()

# If this does not appear to be a git checkout, just pass the test here
# and now. (Do not let the test fail if it is run in a tree *exported* from a
# repository or unpacked from a .zip file source installer...)
#
set(is_git_checkout 0)
if(EXISTS "${CMake_SOURCE_DIR}/.git")
  set(is_git_checkout 1)
endif()

message("is_git_checkout='${is_git_checkout}'")
message("")

if(NOT is_git_checkout)
  message("source tree is not a git checkout... test passes by early return...")
  return()
endif()

# This test looks for the following types of changes in the source tree:
#
set(additions 0)
set(conflicts 0)
set(modifications 0)
set(nonadditions 0)

# ov == output variable... conditionally filled in by either git below:
#
set(cmd "")
set(ov "")
set(ev "")
set(rv "")

# If no GIT_EXECUTABLE, see if we can figure out which git was used
# for the ctest_update step on this dashboard...
#
if(is_git_checkout AND NOT GIT_EXECUTABLE)
  set(ctest_ini_file "")
  set(exe "")

  # Use the old name:
  if(EXISTS "${CMake_BINARY_DIR}/DartConfiguration.tcl")
    set(ctest_ini_file "${CMake_BINARY_DIR}/DartConfiguration.tcl")
  endif()

  # But if it exists, prefer the new name:
  if(EXISTS "${CMake_BINARY_DIR}/CTestConfiguration.ini")
    set(ctest_ini_file "${CMake_BINARY_DIR}/CTestConfiguration.ini")
  endif()

  # If there is a ctest ini file, read the update command or git command
  # from it:
  #
  if(ctest_ini_file)
    file(STRINGS "${ctest_ini_file}" line REGEX "^GITCommand: (.*)$")
    string(REGEX REPLACE "^GITCommand: (.*)$" "\\1" line "${line}")
    if("${line}" MATCHES "^\"")
      string(REGEX REPLACE "^\"([^\"]+)\" *.*$" "\\1" line "${line}")
    else()
      string(REGEX REPLACE "^([^ ]+) *.*$" "\\1" line "${line}")
    endif()
    set(exe "${line}")
    if("${exe}" STREQUAL "GITCOMMAND-NOTFOUND")
      set(exe "")
    endif()
    if(exe)
      message("info: GIT_EXECUTABLE set by 'GITCommand:' from '${ctest_ini_file}'")
    endif()

    if(NOT exe)
      file(STRINGS "${ctest_ini_file}" line REGEX "^UpdateCommand: (.*)$")
      string(REGEX REPLACE "^UpdateCommand: (.*)$" "\\1" line "${line}")
      if("${line}" MATCHES "^\"")
        string(REGEX REPLACE "^\"([^\"]+)\" *.*$" "\\1" line "${line}")
      else()
        string(REGEX REPLACE "^([^ ]+) *.*$" "\\1" line "${line}")
      endif()
      set(exe "${line}")
      if("${exe}" STREQUAL "GITCOMMAND-NOTFOUND")
        set(exe "")
      endif()
      if(exe)
        message("info: GIT_EXECUTABLE set by 'UpdateCommand:' from '${ctest_ini_file}'")
      endif()
    endif()
  else()
    message("info: no DartConfiguration.tcl or CTestConfiguration.ini file...")
  endif()

  # If we have still not grokked the exe, look in the Update.xml file to see
  # if we can parse it from there...
  #
  if(NOT exe)
    file(GLOB_RECURSE update_xml_file "${CMake_BINARY_DIR}/Testing/Update.xml")
    if(update_xml_file)
      file(STRINGS "${update_xml_file}" line
        REGEX "^.*<UpdateCommand>(.*)</UpdateCommand>$" LIMIT_COUNT 1)
      string(REPLACE "&quot\;" "\"" line "${line}")
      string(REGEX REPLACE "^.*<UpdateCommand>(.*)</UpdateCommand>$" "\\1" line "${line}")
      if("${line}" MATCHES "^\"")
        string(REGEX REPLACE "^\"([^\"]+)\" *.*$" "\\1" line "${line}")
      else()
        string(REGEX REPLACE "^([^ ]+) *.*$" "\\1" line "${line}")
      endif()
      if(line)
        set(exe "${line}")
      endif()
      if(exe)
        message("info: GIT_EXECUTABLE set by '<UpdateCommand>' from '${update_xml_file}'")
      endif()
    else()
      message("info: no Update.xml file...")
    endif()
  endif()

  if(exe)
    set(GIT_EXECUTABLE "${exe}")
    message("GIT_EXECUTABLE='${GIT_EXECUTABLE}'")
    message("")

    if(NOT EXISTS "${GIT_EXECUTABLE}")
      message(FATAL_ERROR "GIT_EXECUTABLE does not exist...")
    endif()
  else()
    message(FATAL_ERROR "could not determine GIT_EXECUTABLE...")
  endif()
endif()


if(is_git_checkout AND GIT_EXECUTABLE)
  # Check with "git status" if there are any local modifications to the
  # CMake source tree:
  #
  message("=============================================================================")
  message("This is a git checkout, using git to verify source tree....")
  message("")

  execute_process(COMMAND ${GIT_EXECUTABLE} --version
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}
    OUTPUT_VARIABLE version_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message("=== output of 'git --version' ===")
  message("${version_output}")
  message("=== end output ===")
  message("")

  execute_process(COMMAND ${GIT_EXECUTABLE} branch -a
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}
    OUTPUT_VARIABLE git_branch_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message("=== output of 'git branch -a' ===")
  message("${git_branch_output}")
  message("=== end output ===")
  message("")

  execute_process(COMMAND ${GIT_EXECUTABLE} log -1
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}
    OUTPUT_VARIABLE git_log_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message("=== output of 'git log -1' ===")
  message("${git_log_output}")
  message("=== end output ===")
  message("")

  message("Copy/paste this command to reproduce:")
  message("cd \"${CMake_SOURCE_DIR}\" && \"${GIT_EXECUTABLE}\" status")
  message("")

  set(cmd ${GIT_EXECUTABLE} status)
endif()


if(cmd)
  # Use the HOME value passed in to the script for calling git so it can
  # find its user/global config settings...
  #
  set(original_ENV_HOME "$ENV{HOME}")
  set(ENV{HOME} "${HOME}")

  execute_process(COMMAND ${cmd}
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}
    OUTPUT_VARIABLE ov
    ERROR_VARIABLE ev
    RESULT_VARIABLE rv)

  set(ENV{HOME} "${original_ENV_HOME}")

  message("Results of running ${cmd}")
  message("rv='${rv}'")
  message("ov='${ov}'")
  message("ev='${ev}'")
  message("")

  if(NOT rv STREQUAL 0)
    if(is_git_checkout AND (rv STREQUAL "1"))
      # Many builds of git return "1" from a "nothing is changed" git status call...
      # Do not fail with an error for rv==1 with git...
    else()
      message(FATAL_ERROR "error: ${cmd} attempt failed... (see output above)")
    endif()
  endif()
else()
  message(FATAL_ERROR "error: no COMMAND to run to analyze source tree...")
endif()


# Analyze output:
#
if(NOT ov STREQUAL "")
  string(REPLACE ";" "\\\\;" lines "${ov}")
  string(REPLACE "\n" "E;" lines "${lines}")

  foreach(line ${lines})
    message("'${line}'")

    # But do not consider files that exist just because some user poked around
    # the file system with Windows Explorer or with the Finder from a Mac...
    # ('Thumbs.db' and '.DS_Store' files...)
    #
    set(consider 1)
    set(ignore_files_regex "^(. |.*(/|\\\\))(\\.DS_Store|Thumbs.db)E$")
    if(line MATCHES "${ignore_files_regex}")
      message("   line matches '${ignore_files_regex}' -- not considered")
      set(consider 0)
    endif()

    if(consider)
      if(is_git_checkout)
        if(line MATCHES "^#?[ \t]*modified:")
          message("   locally modified file detected...")
          set(modifications 1)
        endif()

        if(line MATCHES "^(# )?Untracked")
          message("   locally non-added file/directory detected...")
          set(nonadditions 1)
        endif()
      endif()
    endif()
  endforeach()
endif()


message("=============================================================================")
message("additions='${additions}'")
message("conflicts='${conflicts}'")
message("modifications='${modifications}'")
message("nonadditions='${nonadditions}'")
message("")


# Decide if the test passes or fails:
#
message("=============================================================================")

if("$ENV{DASHBOARD_TEST_FROM_CTEST}" STREQUAL "")

  # developers are allowed to have local additions and modifications...
  message("interactive test run")
  message("")

else()

  message("dashboard test run")
  message("")

  # but dashboard machines are not allowed to have local additions or modifications...
  if(additions)
    message(FATAL_ERROR "test fails: local source tree additions")
  endif()

  if(modifications)
    message(FATAL_ERROR "test fails: local source tree modifications")
  endif()

  #
  # It's a dashboard run if ctest was run with '-D ExperimentalTest' or some
  # other -D arg on its command line or if ctest is running a -S script to run
  # a dashboard... Running ctest like that sets the DASHBOARD_TEST_FROM_CTEST
  # env var.
  #

endif()


# ...and nobody is allowed to have local non-additions or conflicts...
# Not even developers.
#
if(nonadditions)
  if(in_source_build)
    message("
warning: test results confounded because this is an 'in-source' build - cannot
distinguish between non-added files that are in-source build products and
non-added source files that somebody forgot to 'git add'... - this is only ok
if this is intentionally an in-source dashboard build... Developers should
use out-of-source builds to verify a clean source tree with this test...

Allowing test to pass despite the warning message...
")
  else()
    message(FATAL_ERROR "test fails: local source tree non-additions: use git add before committing, or remove the files from the source tree")
  endif()
endif()

if(conflicts)
  message(FATAL_ERROR "test fails: local source tree conflicts: resolve before committing")
endif()


# Still here? Good then...
#
message("test passes")
message("")
