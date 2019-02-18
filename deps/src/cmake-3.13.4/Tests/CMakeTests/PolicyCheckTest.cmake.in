# Check the CMake source tree for suspicious policy introdcutions...
#
message("=============================================================================")
message("CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message("")
message("CMake_BINARY_DIR='${CMake_BINARY_DIR}'")
message("CMake_SOURCE_DIR='${CMake_SOURCE_DIR}'")
message("GIT_EXECUTABLE='${GIT_EXECUTABLE}'")
message("")


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
  # Check with "git grep" if there are any unacceptable cmPolicies additions
  #
  message("=============================================================================")
  message("This is a git checkout, using git grep to verify no unacceptable policies")
  message("are being introduced....")
  message("")

  execute_process(COMMAND ${GIT_EXECUTABLE} grep -En "[0-9][0-9][0-9][0-9][0-9].*cmPolicies"
    WORKING_DIRECTORY ${CMake_SOURCE_DIR}
    OUTPUT_VARIABLE grep_output
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message("=== output of 'git grep -En \"[0-9][0-9][0-9][0-9][0-9].*cmPolicies\"' ===")
  message("${grep_output}")
  message("=== end output ===")
  message("")

  if(NOT "${grep_output}" STREQUAL "")
    message(FATAL_ERROR "git grep output is non-empty...
New CMake policies must be introduced in a non-date-based version number.
Send email to the cmake-developers list to figure out what the target
version number for this policy should be...")
  endif()
endif()


# Still here? Good then...
#
message("test passes")
message("")
